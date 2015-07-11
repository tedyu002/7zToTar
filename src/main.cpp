#include<vector>
#include<cassert>
#include<sstream>
#include<unistd.h>

#include"tar.h"
#include"7z.h"


int main(int argc, char *argv[] ){

	assert (access(argv[1], R_OK) == 0);

	std::vector<Entry> entries = get_7z_list(argv[1]);

	Handle hand;
	get_7z_input(&hand, argv[1]);

	size_t total_size = 0;
	uint32_t crc = 0;
	char buffer[128];
	std::stringstream ss;

	for( const Entry &entry : entries) {
		if (entry.is_dir == true) {
			total_size += output_dir(stdout, entry.name.c_str(), entry.mtime);
		}else{
			total_size += output_file(stdout, hand.input, entry.name.c_str(), entry.size, &crc, entry.mtime);
			if( entry.size != 0) {
				assert(crc == entry.crc);
			}
		}
	}
	total_size += output_end(stdout);
	output_end_pad(stdout, total_size);

	end_7z_input(&hand);

	return 0;
}
