#include<sstream>
#include<iomanip>
#include<iostream>
#include<ctime>
#include<cstring>
#include<string>
#include<cstdlib>
#include<algorithm>
#include<cassert>
#include<libgen.h>
#include<unistd.h>
#include<grp.h>

#include"tar.h"
#include"crc32.h"

#define BUF_SIZE (4096*1024)
#define BLOCK_SIZE	(20*512) /* 20 Records */

extern "C"{
typedef struct header_posix_ustar {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char checksum[8];
  char typeflag[1];
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
  char pad[12];
}tar_header_t;
};


template<typename T>
static std::string pax_entry(const char *name, const T &val) {
	std::stringstream ss;

	ss << ' ' << name << '=' << val << '\n';
	std::string cur = ss.str();


	size_t length = cur.size();
	ss.str("");
	ss << length;
	std::string len_str = ss.str();

	size_t new_length = length + ss.str().size();
	ss.str("");
	ss << new_length;
	std::string new_len_str = ss.str();

	size_t total_length = 0;
	if(len_str.size() == new_len_str.size()) {
		total_length = len_str.size() + cur.size();
	}else{
		total_length = len_str.size() + 1 + cur.size();
	}

	ss.str("");
	ss << total_length << cur;

	return ss.str();
}

template<typename T>
static inline int to_oct(char *target, int len, T num){
	std::stringstream ss;
	ss << std::setbase(8) << std::setw(len - 1) << std::setfill('0') << num;
	const std::string &s = ss.str();
	if (s.size() >= len) {
		return -1;
	}
	strcpy(target, s.c_str());
	return 0;
}

static inline void build_checksum(tar_header_t *head){
	memset(head->checksum, ' ', sizeof(head->checksum));

	unsigned sum = 0;
	for(size_t i = 0; i < sizeof(*head); ++i) {
		sum += ((char*)head)[i];
	}

	assert(to_oct(head->checksum, sizeof(head->checksum) - 1, sum) != -1);
}

static inline void tar_header_init(tar_header_t *head){
	memset(head, 0, sizeof(*head));
}

static inline void tar_header_standard(tar_header_t *head, const char *name, time_t mtime){
	strcpy(head->magic, "ustar");
	head->version[0] = head->version[1] = '0';

	char *dir_name_i = strdup(name);
	char *base_name_i = strdup(name);

	const char *dir_name = dirname(dir_name_i);
	const char *base_name = basename(base_name_i);

	if(strcmp(dir_name,"/") == 0) {
		assert(0 && "/");
	}else if(strcmp(dir_name, ".") == 0){
		dir_name = "";
	}else if(strcmp(dir_name, "..") == 0){
		assert(0 && "..");
	}

	strncpy(head->prefix, dir_name, sizeof(head->prefix));
	strncpy(head->name, base_name, sizeof(head->name));

	head->prefix[ sizeof(head->prefix) - 1] = '\0';
	head->name[ sizeof(head->name) - 1] = '\0';

	free(dir_name_i);
	free(base_name_i);

	assert(to_oct(head->uid, sizeof(head->uid), getuid()) == 0);
	assert(to_oct(head->gid, sizeof(head->uid), getgid()) == 0);

	assert(to_oct(head->devmajor, sizeof(head->devmajor), 0) == 0);
	assert(to_oct(head->devminor, sizeof(head->devminor), 0) == 0);

	std::string user_name = getlogin();
	struct group *g = getgrgid(getgid());
	assert( g != NULL);
	std::string group_name = g->gr_name;

	assert(user_name.size() < sizeof(head->uname));
	assert(group_name.size() < sizeof(head->gname));

	strcpy(head->uname, user_name.c_str());
	strcpy(head->gname, group_name.c_str());

	assert(to_oct(head->mtime, sizeof(head->mtime), mtime) == 0);
}

static inline size_t output_pax_extends(FILE *target, const char *name, size_t size, time_t mtime){
	tar_header_t head;
	tar_header_init(&head);
	tar_header_standard(&head, name, mtime);

	std::stringstream ss;
	ss << "./PaxHeaders./" << getuid() << '/' << head.name;
	std::string new_name = ss.str();
	strncpy(head.name, new_name.c_str(), sizeof(head.name));
	head.name[sizeof(head.name) - 1] = '\0';

	strcpy(head.mode, "0000644");

	head.typeflag[0] = 'x'; // POSIX extend


	std::stringstream entries_ss;

	entries_ss << pax_entry("size", size);
	entries_ss << pax_entry("path", name);

	const std::string &entries_str = entries_ss.str();

	assert(to_oct(head.size, sizeof(head.size), entries_str.size()) == 0);
	build_checksum(&head);

	assert(fwrite(&head, sizeof(head), 1, target) == 1);
	assert(fwrite(entries_str.c_str(), entries_str.size(), 1, target) == 1);

	size_t total = sizeof(head);
	total += entries_str.size();

	if (entries_str.size() % sizeof(head) != 0) {
		size_t padding = sizeof(head) - (entries_str.size() % sizeof(head));
		void *pad_buf = calloc(1, padding);
		assert(fwrite(pad_buf, padding, 1, target) == 1);
		total += padding;
		free(pad_buf);
	}

	return total;
}

size_t output_dir(FILE *target, const char *name, time_t mtime){

	size_t attr_size = output_pax_extends(target, name, 0, mtime);

	tar_header_t head;
	tar_header_init(&head);
	tar_header_standard(&head, name, mtime);

	head.typeflag[0] = '5'; //Directory

	strcpy(head.mode, "0000755");
	strcpy(head.size, "00000000000");
	build_checksum(&head);

	assert( fwrite(&head, sizeof(head), 1, target) == 1);
	return sizeof(head) + attr_size;
}

size_t output_file(FILE *target, FILE *input, const char *name, size_t size, uint32_t *crc, time_t mtime, bool ro)
{
	size_t total = size + output_pax_extends(target, name, size, mtime);

	tar_header_t head;
	tar_header_init(&head);
	tar_header_standard(&head, name, mtime);
	head.typeflag[0] = '0'; // regular files.

	if (ro == false) {
		strcpy(head.mode, "0000644");
	}else {
		strcpy(head.mode, "0000444");
	}

	to_oct(head.size, sizeof(head.size), size);

	build_checksum(&head);

	void *buffer = calloc(1, BUF_SIZE);
	assert(buffer != NULL);

	assert(fwrite(&head, sizeof(head), 1, target) == 1);
	total += sizeof(head);

	size_t padding = sizeof(head) - (size % sizeof(head));
	padding = padding == 512 ? 0 : padding;

	uint32_t crc_i = crc32_begin();

	while ( size > 0 ){
		size_t next_handle = std::min(size, (size_t)BUF_SIZE);
		assert( fread(buffer, next_handle, 1, input) == 1);
		assert( fwrite(buffer, next_handle, 1, target) == 1);
		crc_i = crc32_update(crc_i, buffer, next_handle);
		size -= next_handle;
	}

	crc_i = crc32_end(crc_i);

	if (padding > 0) {
		memset(buffer, '\0', padding);
		assert( fwrite(buffer, padding, 1, target) == 1);
		total += padding;
	}

	free(buffer);

	*crc = crc_i;
	return total;
}

size_t output_end(FILE *target){
	tar_header_t head;
	tar_header_init(&head);

	assert(fwrite(&head, sizeof(head.mode), 1, target) == 1);
	assert(fwrite(&head, sizeof(head.mode), 1, target) == 1);
	fflush(target);

	return 2 * sizeof(head.mode);
}

void output_end_pad(FILE *target, size_t total_output_size) {
	size_t rest = total_output_size % BLOCK_SIZE;
 
	if( rest == 0) {
		return;
	}
	size_t padding = BLOCK_SIZE - rest;

	void *buffer = calloc(1, padding);
	assert(buffer != NULL);

	assert(fwrite(buffer, padding, 1, target) == 1);

	free(buffer);
}

