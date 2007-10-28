#ifndef ROUTINES_CPP
#define ROUTINES_CPP

#include <string>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

/**********************************************************************
       Generic library macros
 **********************************************************************/

// bzero is deprecated, but I like it enough to just alias it
#define bzero(ptr,len) memset(ptr, 0, len)
// C++ safe version of zalloc.
// normally has only one arg.
#define zalloc(len,type) (type)calloc(1,len)


/**********************************************************************
	Generic library functions
 **********************************************************************/

struct timeWDHMS {
	uint32_t weeks, days, hours, minutes;
	double seconds;
};

template <typename T> const static inline bool isOdd(const T x) {
	// this is equivalent to (x % 2).
	// It can be faster, and should never be slower.
	return bool(x & 1);
}
template <typename T> const static inline bool isEven(const T x) {
	return !isOdd(x);
}

const static inline struct timeWDHMS splitTime(uint64_t difference) {
	struct timeWDHMS time;
	time.seconds = (double)(difference % 60);
	difference = (difference - (uint64_t)time.seconds) / 60;
	time.minutes = (int)(difference % 60);
	difference = (difference - time.minutes) / 60;
	time.hours = (int)(difference % 24);
	difference = (difference - time.hours) / 24;
	time.days = (int)(difference % 24);
	time.weeks = (int)((difference - time.days) / 7);

	return time;
}

const static inline struct timeWDHMS splitTime(uint32_t difference) {
	struct timeWDHMS time;
	time.seconds = (int)(difference % 60);
	difference = (difference - (uint32_t)time.seconds) / 60;
	time.minutes = (int)(difference % 60);
	difference = (difference - time.minutes) / 60;
	time.hours = (int)(difference % 24);
	difference = (difference - time.hours) / 24;
	time.days = (int)(difference % 24);
	time.weeks = (int)((difference - time.days) / 7);

	return time;
}

const static inline struct timeWDHMS splitTime(const double &difference) {
	struct timeWDHMS time;

	uint64_t difference2 = (uint64_t)(difference / 60);

	time.seconds = (difference - (difference2 * 60));
	time.minutes = (int)(difference2 % 60);
	difference2 = (uint64_t)(difference2 - time.minutes) / 60;
	time.hours = (int)(difference2 % 24);
	difference2 = (difference2 - time.hours) / 24;
	time.days = (int)(difference2 % 24);
	time.weeks = (int)((difference2 - time.days) / 7);

	return time;
}

const static inline vector <string> splitString(const string &delim, const string &str) {
	vector <string> tokens;
	size_t idx1 = str.find_first_not_of(delim, 0);
	size_t idx2 = str.find_first_of(delim, idx1);
	while(string::npos != idx2 || string::npos != idx1) {
		tokens.push_back(str.substr(idx1, idx2-idx1));
		idx1 = str.find_first_not_of(delim, idx2);
		idx2 = str.find_first_of(delim, idx1);
	}
	
	return tokens;
}

const static inline string uint64toString(const uint64_t &num) {
	char str[20+1]; // log10(2**64-1) = ~19.26
#if __WORDSIZE == 64
	// uint64_t is 'long unsigned int' here
	snprintf(str, 20, "%lu", num);
#else
	// uint64_t is 'long long unsigned int' here
	snprintf(str, 20, "%llu", num);
#endif
	return string(str);
}

const static inline string int64toString(const int64_t &num) {
	char str[20+1]; // log10(2**64-1) = ~19.26
#if __WORDSIZE == 64
	// uint64_t is 'long unsigned int' here
	snprintf(str, 20, "%ld", num);
#else
	// uint64_t is 'long long unsigned int' here
	snprintf(str, 20, "%lld", num);
#endif
	return string(str);
}

const static inline uint64_t string2uint64(const string &str) {
	return strtoull(str.c_str(), (char **)NULL, 10);
}

const static inline int64_t string2int64(const string &str) {
	return strtoll(str.c_str(), (char **)NULL, 10);
}

const static inline uint32_t string2uint32(const string &str) {
	return strtoul(str.c_str(), (char **)NULL, 10);
}
// This isn't really necessary, but it reduces the number of conversions
const static inline uint32_t string2uint32(const char *str) {
	return strtoul(str, (char **)NULL, 10);
}

const static inline int32_t string2int32(const string &str) {
	return strtol(str.c_str(), (char **)NULL, 10);
}
const static inline int32_t string2int32(const char *str) {
	return strtol(str, (char **)NULL, 10);
}

const static inline double string2double(const string &str) {
	return strtod(str.c_str(), (char **)NULL);
}
const static inline double string2double(const char *str) {
	return strtod(str, (char **)NULL);
}

const static inline vector <uint64_t> stringVec2uint64Vec(const vector <string> &stringVec) {
	vector <uint64_t> uint64Vec; uint64Vec.resize(stringVec.size());
	for(uint32_t i = 0; i < stringVec.size(); i++)
		uint64Vec[i] = string2uint64(stringVec[i]);
	return uint64Vec;
}

const static inline vector <uint64_t> subUint64Vec(const vector <uint64_t> &vec1, const vector <uint64_t> &vec2) {
	vector <uint64_t> vec3; vec3.resize( min(vec2.size(), vec1.size()) );
	for(uint32_t i = 0; i < min(vec2.size(), vec1.size()); i++)
		vec3[i] = vec1[i] - vec2[i];
	return vec3;
}

const static inline uint32_t getFrac(const double &val, const uint32_t &mod) {
	return (uint32_t(val * mod) % mod);
}

template <typename T> static inline void swap(T &x, T &y) {
	T tmp = x;
	y = x;
	x = tmp;
	return;
}

// Don't use this for large files,
// b/c it slurps the whole thing into RAM.
// Also, it _will_ fail_ for lines over ~40960 bytes
// For clarification, 'fail' means 'loop infinitely'
// and allocate memory infinitely.
static vector <string> readFile(const string &fileName) {
	vector <string> lines;
	ifstream file(fileName.c_str());

	for(uint32_t i = 0; !file.eof(); i++) {
		char *str = zalloc(40960, char *);
		file.getline(str, 40960-2);
		lines.push_back(string(str));
	}
	return lines;
}


#endif
