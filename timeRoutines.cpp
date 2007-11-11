#ifndef TIMEROUTINES_CPP
#define TIMEROUTINES_CPP

#include <time.h>

struct timeWDHMS {
	uint32_t weeks, days, hours, minutes;
	double seconds;
};

#define secPerMin 60
#define minPerHour 60
#define hourPerDay 24
#define dayPerWeek 7
#define secPerHour secPerMin*minPerHour
#define secPerDay secPerMin*minPerHour*hourPerDay
#define monthPerYear 12

const static inline struct timeWDHMS splitTime(uint64_t difference) {
	struct timeWDHMS time;
	time.seconds = (double)(difference % secPerMin);
	difference = (difference - (uint64_t)time.seconds) / secPerMin;
	time.minutes = (uint32_t)(difference % minPerHour);
	difference = (difference - time.minutes) / minPerHour;
	time.hours = (uint32_t)(difference % hourPerDay);
	difference = (difference - time.hours) / hourPerDay;
	time.days = (uint32_t)(difference % hourPerDay);
	time.weeks = (uint32_t)((difference - time.days) / dayPerWeek);

	return time;
}

const static inline struct timeWDHMS splitTime(uint32_t difference) {
	struct timeWDHMS time;
	time.seconds = (uint32_t)(difference % secPerMin);
	difference = (difference - (uint32_t)time.seconds) / secPerMin;
	time.minutes = (uint32_t)(difference % minPerHour);
	difference = (difference - time.minutes) / minPerHour;
	time.hours = (uint32_t)(difference % hourPerDay);
	difference = (difference - time.hours) / hourPerDay;
	time.days = (uint32_t)(difference % hourPerDay);
	time.weeks = (uint32_t)((difference - time.days) / dayPerWeek);

	return time;
}

const static inline struct timeWDHMS splitTime(const double &difference) {
	struct timeWDHMS time;

	uint64_t difference2 = (uint64_t)(difference / secPerMin);

	time.seconds = (difference - (difference2 * secPerMin));
	time.minutes = (uint32_t)(difference2 % minPerHour);
	difference2 = (uint64_t)(difference2 - time.minutes) / minPerHour;
	time.hours = (uint32_t)(difference2 % hourPerDay);
	difference2 = (difference2 - time.hours) / hourPerDay;
	time.days = (uint32_t)(difference2 % dayPerWeek);
	time.weeks = (uint32_t)((difference2 - time.days) / dayPerWeek);

	return time;
}

/* Nonzero if YEAR is a leap year (every 4 years,
   except every 100th isn't, and every 400th is).  */
// In case the macro isn't available in time.h
//# define __isleap(year) \
//  !!((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))
const static inline int get_monthdays(const int month, const int year) {
	switch(month) {
		case 1:
		case 3:
		case 5:
	        case 7:
		case 8:
		case 10:
		case 12:
			return 31;
		case 4:
		case 6:
		case 9:
		case 11:
			return 30;
		case 2:
			if( __isleap(year) ) { // macro from time.h
				return 29;
		        }
			else
				return 28;
		default:
	        	return 0;
	}
}

struct timeDiff
{ // shamelessly stolen from time.h's struct tm and modified
  int tm_sec;			/* Seconds.       [0-60] (1 leap second) */
  int tm_min;			/* Minutes.       [0-59] */
  int tm_hour;			/* Hours.         [0-23] */
  int tm_wday;			/* Day of week.   [0-6] */
  int tm_week;			/* Week of month. [0-6] */
  int tm_mon;			/* Month.         [0-11] */
  int tm_year;			/* Year - 1900.  */
};

// This is for cases over 4 weeks, when we need years, months, weeks, and days
// WARNING. This code is a straight port from some known-good Perl code.
// However, it has not been tested (yet) in this version.
const static inline struct timeDiff __time_rel_long(const struct tm &lesser_time, const struct tm &greater_time) {
	struct timeDiff result;

	result.tm_sec = greater_time.tm_sec - lesser_time.tm_sec;
	result.tm_min = greater_time.tm_min - lesser_time.tm_min;
	if(result.tm_sec < 0) {
		result.tm_sec += secPerMin; result.tm_min--;
	}
	result.tm_hour = greater_time.tm_hour - lesser_time.tm_hour;
	if(result.tm_min < 0) {
		result.tm_min += minPerHour; result.tm_hour--;
	}
	result.tm_wday = greater_time.tm_mday - lesser_time.tm_mday;
	if(result.tm_hour < 0) {
		result.tm_hour += hourPerDay; result.tm_wday--;
	}
	result.tm_mon = greater_time.tm_mon - lesser_time.tm_mon;
	if(result.tm_wday < 0) {
		result.tm_wday += get_monthdays(
			(greater_time.tm_mon == 0 ? monthPerYear - 1 : greater_time.tm_mon - 1),
			(greater_time.tm_mon == 0 ? greater_time.tm_year - 1 : greater_time.tm_year));
		result.tm_mon--;	
	}
	result.tm_week = result.tm_wday / dayPerWeek;
	result.tm_wday %= dayPerWeek;
	result.tm_year = greater_time.tm_year - lesser_time.tm_year;
	if(result.tm_mon < 0) {
		result.tm_mon += monthPerYear; result.tm_year--;
	}
	return result;
}

const static inline struct timeDiff __time_rel_long(const time_t lesser_time, const time_t greater_time) {
	struct tm *__greater_time = gmtime(&greater_time);
	struct tm *__lesser_time = gmtime(&lesser_time);
	return __time_rel_long(*__lesser_time, *__greater_time);
}

#endif