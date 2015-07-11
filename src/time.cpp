#include<ctime>
#include<cstdlib>
#include<array>
#include<cassert>
#include<cstring>
#include<climits>

#include"time.h"

time_t parse_time(const std::string &str) {
	char *d_str = new char[str.size() + 2]; // 1 room for end_ptr move

	strcpy(d_str, str.c_str());
	d_str[str.size() + 1] = '\0';

	std::array<long,6> nums; // 6:y m d h m s
	size_t count = 0;

	for(char *c_str = d_str; *c_str != '\0';) {
		assert(count < 6);
		char *end_ptr;
		long val = strtol(c_str, &end_ptr, 10);
		assert(c_str != end_ptr);
		assert( val != LONG_MAX && val != LONG_MIN);
		assert( val >= 0 );
		nums[count] = val;
		c_str = end_ptr + 1;
		count++;
	}

	assert(count == 6);
	assert(nums[0] >= 1900);
	assert(1 <= nums[1] && nums[1] <= 12);
	assert(1 <= nums[2] && nums[2] <= 31);
	assert(0 <= nums[3] && nums[3] <= 23);
	assert(0 <= nums[4] && nums[4] <= 59);
	assert(0 <= nums[5] && nums[5] <= 60); //leap second

	struct tm tm;
	memset(&tm, 0, sizeof(tm));

	tm.tm_year = (int)nums[0] - 1900; //since 1900
	tm.tm_mon = (int)nums[1] - 1; //0~11
	tm.tm_mday = (int)nums[2];
	tm.tm_hour = (int)nums[3];
	tm.tm_min = (int)nums[4];
	tm.tm_sec = (int)nums[5];

	delete[] d_str;

	return mktime(&tm);
}
