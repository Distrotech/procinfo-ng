#ifndef ROUTINES_CPP
#define ROUTINES_CPP

#include <string>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <fstream>

//using namespace std;
using std::cout; using std::vector; using std::string;
using std::ifstream;
using std::min; using std::max;
using std::endl;

/**********************************************************************
       Generic library macros
 **********************************************************************/

// bzero is deprecated, but I like it enough to just alias it
#define bzero(ptr,len) memset(ptr, 0, len)
// C++ safe version of zalloc.
// normally has only one arg.
#define zalloc(len,type) (type)calloc(1,len)

// Blatantly stolen from the linux kernel
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)


/**********************************************************************
	Generic library functions
 **********************************************************************/

template <typename T> const static inline bool isOdd(const T x) {
	// this is equivalent to (x % 2).
	// It can be faster, and should never be slower.
	return bool(x & 1);
}
template <typename T> const static inline bool isEven(const T x) {
	return !isOdd(x);
}


template <typename T> const static T gcd(const T &a, const T &b) {
	if (b == 0)
		return a;
	return gcd(b, a % b);
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

const static inline string uint32toString(const uint32_t &num) {
	char str[10+1]; // log10(2**32-1) = ~9.63
	snprintf(str, 20, "%u", num);
	return string(str);
}

const static inline string int32toString(const int32_t &num) {
	char str[10+1]; // log10(2**32-1) = ~9.63
	snprintf(str, 20, "%d", num);
	return string(str);
}

const static inline uint64_t string2uint64(const string &str) {
	// the '10' means 'base-10', or decimal.
	return strtoull(str.c_str(), (char **)NULL, 10);
}

const static inline int64_t string2int64(const string &str) {
	// the '10' means 'base-10', or decimal.
	return strtoll(str.c_str(), (char **)NULL, 10);
}

const static inline uint32_t string2uint32(const string &str) {
	// the '10' means 'base-10', or decimal.
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

/* Don't use this for large files,
   b/c it slurps the whole thing into RAM.
   This isn't const b/c the file might change underneath us.
   This isn't inline b/c it looks too big.
   If the compiler inlines it anyway, who cares.
*/
static vector <string> readFile(const char *fileName) {
	vector <string> lines;
	ifstream file;

	int i = 0;
	readFile_label:
	file.open(fileName);
	if( unlikely( !file.is_open() ) ) {
		if( likely(++i < 10) ) {
			goto readFile_label;
		} else {
			abort();
		}
	}

	for( ; !file.eof(); ) {
		string str;
		getline(file, str);

		lines.push_back(str);
	}
	if( unlikely( lines.size() == 0 ) ) {
		if( likely( ++i < 10 ) ) {
			goto readFile_label;
		} else {
			abort();
		}
	} else {
		return lines;
	}
}
static vector <string> readFile(const string &fileName) {
	return readFile(fileName.c_str());
}

const static inline string toString(uint32_t input) {
	return uint32toString(input);
}
const static inline string toString(int32_t input) {
	return int32toString(input);
}
const static inline string toString(uint64_t input) {
	return uint64toString(input);
}
const static inline string toString(int64_t input) {
	return int64toString(input);
}

const static inline string double2StringPrecision(double input, uint32_t precision) {
	char fmtBuf[3+10+1]; bzero(fmtBuf, sizeof(fmtBuf));
	snprintf(fmtBuf, 3+10, "%%.%uf", precision);
	char output[64]; bzero(output, sizeof(output));
	snprintf(output, 63, fmtBuf, input);
	return string(output);
}

const static inline string toString2digits(double input) {
	char output[64]; bzero(output, sizeof(output));
	snprintf(output, 63, "%.2f", input);
	return string(output);
}

const static inline string humanizeBigNums(uint64_t val, uint32_t precision) {
	if(val > (1 << 30)) {
		return double2StringPrecision(double(val) / (1 << 30), precision) + "GiB";
	}
	else if(val > (1 << 20)) {
		return double2StringPrecision(double(val) / (1 << 20), precision) + "MiB";
	}
	else if(val > (1 << 10)) {
		return double2StringPrecision(double(val) / (1 << 10), precision) + "KiB";
	}
	return toString(val) + "B";
}
const static inline string humanizeBigNums(int64_t val, uint32_t precision) {
	if(abs(val) > (1 << 30)) {
		return double2StringPrecision(double(val) / (1 << 30), precision) + "GiB";
	}
	else if(abs(val) > (1 << 20)) {
		return double2StringPrecision(double(val) / (1 << 20), precision) + "MiB";
	}
	else if(abs(val) > (1 << 10)) {
		return double2StringPrecision(double(val) / (1 << 10), precision) + "KiB";
	}
	return toString(val) + "B";
}
const static inline string humanizeBigNums(double val, uint32_t precision) {
	if(fabs(val) > (1 << 30)) {
		return double2StringPrecision(val / (1 << 30), precision) + "GiB";
	}
	else if(fabs(val) > (1 << 20)) {
		return double2StringPrecision(val / (1 << 20), precision) + "MiB";
	}
	else if(fabs(val) > (1 << 10)) {
		return double2StringPrecision(val / (1 << 10), precision) + "KiB";
	}
	return double2StringPrecision(val, precision) + "B";
}
template <typename T> const static inline string humanizeBigNums(T val) {
	return humanizeBigNums(val, 2);
}

#endif
