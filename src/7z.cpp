#include<unistd.h>
#include<cstdlib>
#include<cstdio>
#include<cassert>
#include<cstring>
#include<cstdint>
#include<climits>
#include<cctype>
#include<ctime>
#include<unistd.h>
#include<sys/wait.h>

#include<arpa/inet.h>

#include"7z.h"
#include"time.h"

#define PATH_HEAD "Path = "
#define SIZE_HEAD "Size = "
#define CRC_HEAD  "CRC = "
#define MTIME_HEAD "Modified = "
#define ATTR_HEAD "Attributes = "

std::vector<Entry> get_7z_list(const char *name) {
	pid_t pid;
	int fd[2];
	assert(pipe(fd) == 0);
	pid = fork();
	if (pid == 0) {
		assert(close(fd[0]) == 0);
		assert(dup2(fd[1], 1) != -1);
		assert(close(fd[1]) == 0);
		execlp("7z", "7z", "l", "-slt", name, NULL);
		perror("Exec Error");
		exit(-1);
	}else if( pid < 0){
		perror("fork error");
		assert("0" && "fork error");
	}

	assert(close(fd[1]) == 0);

	char *buf = NULL;
	size_t size = 0;
	ssize_t r_size = 0;

	FILE *read_data = fdopen(fd[0], "r");
	assert(read_data != NULL);

	bool con = false;
	while((r_size = ::getline(&buf, &size, read_data)) != -1){
		if( r_size >= 4 && buf[0] == '-') {
			con = true;
			break;
		}
	}

	assert( con == true );

	std::vector<Entry> entries;

	while(1){
		Entry entry;
		int set_field = 0;
		bool first = true;
		for(;;) {
			r_size = ::getline(&buf, &size, read_data);
			if ( first == true && r_size == -1){
				goto END;
			}
			first = false;
			assert(r_size != -1 && "get list error");
			if(r_size > 0 && buf[r_size-1] == '\n'){
				r_size--;
				buf[r_size] = '\0';
			}else {
				assert(0 && "No new line");
			}

			if (r_size == 0) {
				assert(set_field == 5);
				entries.push_back(entry);
				break;
			}

			if( strncmp(buf, PATH_HEAD, strlen(PATH_HEAD)) == 0) {
				entry.name = &buf[strlen(PATH_HEAD)];
				assert(entry.name.size() > 0 );
				set_field++;
			} else if(strncmp(buf, SIZE_HEAD, strlen(SIZE_HEAD)) == 0) {
				char *end = NULL;
				assert(isdigit(buf[strlen(SIZE_HEAD)]) != 0 && "Size is not correct");
				long res = (size_t)strtol(&buf[strlen("Size = ")], &end, 10);
				assert( res != LONG_MIN && res != LONG_MAX && *end == '\0' && res >= 0);
				entry.size = (size_t)res;
				set_field++;
			} else if(strncmp(buf, ATTR_HEAD, strlen(ATTR_HEAD)) == 0) {
				set_field++;
				const char *head = &buf[strlen(ATTR_HEAD)];
				if (strchr(head, 'D') != NULL) {
					entry.is_dir = true;
				} else if(strchr(head, 'R') != NULL) {
					entry.is_ro = true;
				}
			} else if(strncmp(buf, CRC_HEAD, strlen(CRC_HEAD)) == 0) {
				set_field++;
				if( entry.is_dir == false) {
					std::string val = &buf[strlen(CRC_HEAD)];
					assert (val.size() == 8 /*uint32_t to hex format*/ || entry.size == 0);
					uint32_t present = 0;

					size_t shift = 0;
					entry.crc = 0;
					for(size_t i = 0 ; i < 8; i += 2){
						present = 0;
						for(size_t j = 0 ; j < 2; j++){
							present <<= 4;
							if (isdigit(val[i + j]) != 0 ){
								present += val[i + j] - '0';
							} else {
								present += (val[i + j] - 'A') + 10;
							}
						}
						entry.crc = (entry.crc << 8) + present;
					}
				}
			} else if(strncmp(buf, MTIME_HEAD, strlen(MTIME_HEAD)) == 0) {
				set_field++;
				const std::string mtime = &buf[strlen(MTIME_HEAD)];

				entry.mtime = parse_time(mtime);
			}
		}
	}

END:
	fclose(read_data);
	assert(waitpid(pid, NULL, 0) != -1);
	return entries;
}

void get_7z_input(Handle *hand, const char *name) {

	pid_t pid;
	int fd[2];

	assert(pipe(fd) == 0);

	pid = fork();
	if (pid == 0) {
		assert(close(fd[0]) == 0);
		assert(dup2(fd[1], 1) != -1);
		assert(close(fd[1]) == 0);
		execlp("7z", "7z", "x", "-so" , name, NULL);
		perror("exec error:");
		exit(-1);
	} else if( pid < 0) {
		perror("Fork error");
		assert(0 && "Fork error");
	}

	assert(close(fd[1]) == 0);

	FILE *file = fdopen(fd[0], "r");
	assert(file != NULL);

	hand->input = file;
	hand->pid = pid;

}

void end_7z_input(Handle *head){
	assert(fclose(head->input) != EOF);
	assert(waitpid(head->pid, NULL, 0) != -1);
}

