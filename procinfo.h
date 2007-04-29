/********************************************************************** 
	Generic library functions
 **********************************************************************/

typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef signed int int32;
typedef signed long long int64;

struct timeWDHMS {
	uint32 weeks, days, hours, minutes;
	double seconds;
};

static inline struct timeWDHMS splitTime(uint64 difference) {
	struct timeWDHMS time;
	time.seconds = (double)(difference % 60);
	difference = (difference - (uint64)time.seconds) / 60;
	time.minutes = (int)(difference % 60);
	difference = (difference - time.minutes) / 60;
	time.hours = (int)(difference % 24);
	difference = (difference - time.hours) / 24;
	time.days = (int)(difference % 24);
	time.weeks = (int)((difference - time.days) / 7);

	return time;
}

static inline struct timeWDHMS splitTime(uint32 difference) {
	struct timeWDHMS time;
	time.seconds = (int)(difference % 60);
	difference = (difference - (uint32)time.seconds) / 60;
	time.minutes = (int)(difference % 60);
	difference = (difference - time.minutes) / 60;
	time.hours = (int)(difference % 24);
	difference = (difference - time.hours) / 24;
	time.days = (int)(difference % 24);
	time.weeks = (int)((difference - time.days) / 7);

	return time;
}

static inline struct timeWDHMS splitTime(double difference) {
	struct timeWDHMS time;

	uint64 difference2 = (uint64)(difference / 60);

	time.seconds = (difference - (difference2 * 60));
	time.minutes = (int)(difference2 % 60);
	difference2 = (uint64)(difference2 - time.minutes) / 60;
	time.hours = (int)(difference2 % 24);
	difference2 = (difference2 - time.hours) / 24;
	time.days = (int)(difference2 % 24);
	time.weeks = (int)((difference2 - time.days) / 7);

	return time;
}

static inline vector <string> splitString(string delim, string str) {
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

static inline string uint64toString(uint64 num) {
	char str[20+1];
	snprintf(str, 20, "%llu", (unsigned long long int)num);
	return string(str);
}

static inline string int64toString(uint64 num) {
	char str[20+1];
	snprintf(str, 20, "%lld", (unsigned long long int)num);
	return string(str);
}

static inline uint64 string2uint64(string &str) {
	return strtoull(str.c_str(), (char **)NULL, 10);
}

static inline uint64 string2int64(string &str) {
	return strtoll(str.c_str(), (char **)NULL, 10);
}

static inline uint32 string2uint32(string &str) {
	return strtoul(str.c_str(), (char **)NULL, 10);
}

static inline uint32 string2int32(string &str) {
	return strtol(str.c_str(), (char **)NULL, 10);
}

static inline vector <uint64> stringVec2uint64Vec(vector <string> stringVec) {
	vector <uint64> uint64Vec;
	for(uint32 i = 0; i < stringVec.size(); i++)
		uint64Vec.push_back(string2uint64(stringVec[i]));
	return uint64Vec;
}

static inline vector <uint64> subUint64Vec(vector <uint64> vec1, vector <uint64> vec2) {
	vector <uint64> vec3; vec3.resize(vec2.size());
	for(uint32 i = 0; i < vec2.size(); i++)
		vec3[i] = vec1[i] - vec2[i];
	return vec3;
}

static inline uint32 getFrac(double val, uint32 mod) {
	return (uint32(val * mod) % mod);
}
