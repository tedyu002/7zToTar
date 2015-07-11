#ifndef __TT_7z_H
#define __TT_7z_H

#include<cctype>
#include<vector>
#include<cstdio>
#include<string>
#include<unistd.h>

struct Handle{
	FILE *input;
	pid_t pid;
};

class Entry{
public:
	std::string name;
	size_t size;
	uint32_t crc;
	bool is_dir;
	bool is_ro;
	time_t mtime;
	inline Entry() : size(0), crc(0), is_dir(false), is_ro(false), mtime(0){}
};

std::vector<Entry> get_7z_list(const char *name);
void get_7z_input(Handle *hand, const char *name);
void end_7z_input(Handle *hand);

#endif
