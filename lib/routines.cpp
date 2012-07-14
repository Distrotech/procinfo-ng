/*
	This file is part of procinfo-NG

	procinfo-NG/routines.cpp is free software; you can redistribute it
	and/or modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; version 2.1.

	procinfo-NG is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with procinfo-NG; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Procinfo-NG is Copyright tabris@tabris.net 2007, 2008, 2009
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

template <typename T> const static inline bool isOdd(const T &x) {
	// this is equivalent to (x % 2).
	// It can be faster, and should never be slower.
	return bool(x & 1);
}
template <typename T> const static inline bool isEven(const T &x) {
	return !isOdd(x);
}


template <typename T> const static T gcd(const T &a, const T &b) {
	if (b == 0)
		return a;
	return gcd(b, a % b);
}

const static vector <string> splitString(const string &delim, const string &str) {
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
	char str[32]; // log10(2**64-1) = ~19.26, 32 for alignment
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
	char str[32]; // log10(2**64-1) = ~19.26, 32 for alignment
#if __WORDSIZE == 64
	// int64_t is 'long signed int' here
	snprintf(str, 20+1, "%ld", num); // +1 for sign
#else
	// int64_t is 'long long signed int' here
	snprintf(str, 20+1, "%lld", num); // +1 for sign
#endif
	return string(str);
}

const static inline string uint32toString(const uint32_t &num) {
	char str[16]; // log10(2**32-1) = ~9.63, 16 for alignment
	snprintf(str, 10, "%u", num);
	return string(str);
}

const static inline string int32toString(const int32_t &num) {
	char str[16]; // log10(2**32-1) = ~9.63, 16 for alignment
	snprintf(str, 10+1, "%d", num); // +1 for sign
	return string(str);
}

const static inline uint64_t string2uint64(const string &str) {
	// the '10' means 'base-10', or decimal.
	return strtoull(str.c_str(), (char **)NULL, 10);
}

const static inline string toString(const uint32_t &input) {
	return uint32toString(input);
}
const static inline string toString(const int32_t &input) {
	return int32toString(input);
}
const static inline string toString(const uint64_t &input) {
	return uint64toString(input);
}
const static inline string toString(const int64_t &input) {
	return int64toString(input);
}

const static inline int64_t string2int64(const char *str) {
	// the '10' means 'base-10', or decimal.
	return strtoll(str, (char **)NULL, 10);
}
const static inline int64_t string2int64(const string &str) {
	// the '10' means 'base-10', or decimal.
	//return strtoll(str.c_str(), (char **)NULL, 10);
	return string2int64(str.c_str());
}

// This first instance isn't really necessary, but it reduces the number of conversions
const static inline uint32_t string2uint32(const char *str) {
	return strtoul(str, (char **)NULL, 10);
}
const static inline uint32_t string2uint32(const string &str) {
	// the '10' means 'base-10', or decimal.
	//return strtoul(str.c_str(), (char **)NULL, 10);
	return string2uint32(str.c_str());
}

const static inline int32_t string2int32(const char *str) {
	return strtol(str, (char **)NULL, 10);
}
const static inline int32_t string2int32(const string &str) {
	//return strtol(str.c_str(), (char **)NULL, 10);
	return string2int32(str.c_str());
}

const static inline double string2double(const char *str) {
	return strtod(str, (char **)NULL);
}
const static inline double string2double(const string &str) {
	return string2double(str.c_str());
}

const static inline vector <uint64_t> stringVec2uint64Vec(const vector <string> &stringVec) {
	vector <uint64_t> uint64Vec; uint64Vec.resize(stringVec.size());
	for(uint32_t i = stringVec.size() - 1; i > 0; i--)
		uint64Vec[i] = string2uint64(stringVec[i]);
	return uint64Vec;
}

template <typename T> const static inline vector <T> subVec(const vector <T> &vec1, const vector <T> &vec2) {
	vector <T> vec3; vec3.resize( min(vec2.size(), vec1.size()) );
	for(uint32_t i = min(vec2.size(), vec1.size()) - 1; i > 0; i--)
		vec3[i] = vec1[i] - vec2[i];
	return vec3;
}

template <typename T> const static inline T sumVec(const vector <T> &vec) {
	register T sum = 0;
	for(uint32_t i = vec.size() - 1; i > 0; i--)
		sum += vec[i];
	return sum;
}

const static inline uint32_t getFrac(const double &val, const uint32_t &mod) {
	return (uint32_t(val * mod) % mod);
}

template <typename T> static inline void swap(T &x, T &y) {
	const T tmp = y;
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
			throw "Unable to open " + string(fileName);
			//abort();
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
		if(lines.at(lines.size()-1) == "") {
			lines.pop_back();
		}
		return lines;
	}
}
static inline vector <string> readFile(const string &fileName) {
	return readFile(fileName.c_str());
}

const static inline string double2StringPrecision(const double &input, const uint32_t precision) {
	char fmtBuf[3+(10*2)+1]; bzero(fmtBuf, sizeof(fmtBuf));
	snprintf(fmtBuf, 3+(10*2), "%%.%uf", precision);
	char output[32]; bzero(output, sizeof(output));
	snprintf(output, 31, fmtBuf, input);
	return string(output);
}

const static inline string toString2digits(const double &input) {
	char output[32]; bzero(output, sizeof(output));
	snprintf(output, 31, "%.2f", input);
	return string(output);
}
const static string humanizeBigNums(const int64_t &val, const uint32_t precision) {
	const register int64_t absVal = llabs(val);
	int32_t shiftVal = 0;
	string suffix;
	if(absVal >= (1LL << 60)) {
		shiftVal = 60;
		suffix = "EiB";
	}
	else if(absVal >= (1LL << 50)) {
		shiftVal = 50;
		suffix = "PiB";
	}
	else if(absVal >= (1LL << 40)) {
		shiftVal = 40;
		suffix = "TiB";
	}
	else if(absVal >= (1LL << 30)) {
		shiftVal = 30;
		suffix = "GiB";
	}
	else if(absVal >= (1 << 20)) {
		shiftVal = 20;
		suffix = "MiB";
	}
	else if(absVal >= (1 << 10)) {
		shiftVal = 10;
		suffix = "KiB";
	} else {
		suffix = "B";
	}
	return double2StringPrecision(double(val) / (1LL << shiftVal), precision) + suffix;
}
const static string humanizeBigNums(const uint64_t &val, const uint32_t precision) {
	int32_t shiftVal = 0;
	string suffix;
	if(val >= (1ULL << 60)) {
		shiftVal = 60;
		suffix = "EiB";
	}
	else if(val >= (1ULL << 50)) {
		shiftVal = 50;
		suffix = "PiB";
	}
	else if(val >= (1ULL << 40)) {
		shiftVal = 40;
		suffix = "TiB";
	}
	else if(val >= (1ULL << 30)) {
		shiftVal = 30;
		suffix = "GiB";
	}
	else if(val >= (1 << 20)) {
		shiftVal = 20;
		suffix = "MiB";
	}
	else if(val >= (1 << 10)) {
		shiftVal = 10;
		suffix = "KiB";
	} else {
		suffix = "B";
	}
	return double2StringPrecision(double(val) / (1ULL << shiftVal), precision) + suffix;
}
template <typename T> const static string humanizeBigNums(const T &val, const uint32_t precision) {
	const register T absVal = fabs(val);
	int32_t shiftVal = 0;
	string suffix;
	if(val >= (1LL << 60)) {
		shiftVal = 60;
		suffix = "EiB";
	}
	else if(val >= (1LL << 50)) {
		shiftVal = 50;
		suffix = "PiB";
	}
	else if(val >= (1LL << 40)) {
		shiftVal = 40;
		suffix = "TiB";
	}
	else if(val >= (1LL << 30)) {
		shiftVal = 30;
		suffix = "GiB";
	}
	else if(val >= (1 << 20)) {
		shiftVal = 20;
		suffix = "MiB";
	}
	else if(val >= (1 << 10)) {
		shiftVal = 10;
		suffix = "KiB";
	} else {
		suffix = "B";
	}
	return double2StringPrecision(double(val) / (1LL << shiftVal), precision) + suffix;
}
template <typename T> const static inline string humanizeBigNums(const T &val) __attribute((always_inline));
template <typename T> const static inline string humanizeBigNums(const T &val) {
	return humanizeBigNums(val, 2);
}

#endif
