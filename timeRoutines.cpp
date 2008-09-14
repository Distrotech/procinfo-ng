/*
	This file is part of procinfo-NG

	procinfo-NG/routines.cpp is free software; you can redistribute it
	and/or modify it under the terms of the GNU Lesser General Public
	License as published by	the Free Software Foundation; version 2.1.

	procinfo-NG is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with procinfo-NG; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Procinfo-NG is Copyright tabris@tabris.net 2007, 2008

#ifndef TIMEROUTINES_CPP
#define TIMEROUTINES_CPP

#include <time.h>
#include <sys/time.h>

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

static inline double getCurTime() {
	struct timeval timeNow;
	gettimeofday(&timeNow, NULL);
	return (double(timeNow.tv_sec) + double(timeNow.tv_usec) / double(1e6));
}

template <typename T> const static inline struct timeWDHMS splitTime(T difference) {
	struct timeWDHMS time;
	time.seconds = (double)(difference % secPerMin);
	difference = (difference - (T)time.seconds) / secPerMin;
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
#ifndef __isleap
#define __isleap(year) \
  !!((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))
#endif
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
  double tm_sec;		/* Seconds.       [0-60] (1 leap second) */
  int tm_min;			/* Minutes.       [0-59] */
  int tm_hour;			/* Hours.         [0-23] */
  int tm_wday;			/* Day of week.   [0-6] */
  int tm_week;			/* Week of month. [0-6] */
  int tm_mon;			/* Month.         [0-11] */
  int tm_year;			/* Year - 1900.  */
};

const static inline struct timeDiff __time_rel(const time_t &lesser_time, const time_t &greater_time) {
	const struct timeWDHMS result1 = splitTime(greater_time - lesser_time);
	const struct timeDiff result2 =
		{ tm_sec: result1.seconds, tm_min: result1.minutes, tm_hour: result1.hours,
		tm_wday: result1.days, tm_week: result1.weeks };
	return result2;
}
const static inline struct timeDiff __time_rel(const double &lesser_time, const double &greater_time) {
	const struct timeWDHMS result1 = splitTime(greater_time - lesser_time);
	const struct timeDiff result2 =
		{ tm_sec: result1.seconds, tm_min: result1.minutes, tm_hour: result1.hours,
		tm_wday: result1.days, tm_week: result1.weeks };
	return result2;
}

/*
   This is for cases over 4 weeks, when we need years, months, weeks, and days
   It is also aparrently unreliable for less than that time.
   WARNING. This code used to be a port of some Perl code, but has since been changed a lot.
*/
const static inline struct timeDiff __time_rel_long(const struct timeDiff &lesser_time, const struct timeDiff &greater_time) {
	struct timeDiff result = { tm_sec: 0 };

	result.tm_sec = greater_time.tm_sec - lesser_time.tm_sec;
	result.tm_min = greater_time.tm_min - lesser_time.tm_min;
	if(result.tm_sec < 0) {
		result.tm_sec += secPerMin; result.tm_min--;
	}
	result.tm_hour = greater_time.tm_hour - lesser_time.tm_hour;
	if(result.tm_min < 0) {
		result.tm_min += minPerHour; result.tm_hour--;
	}
	result.tm_wday = greater_time.tm_wday - lesser_time.tm_wday;
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

const static inline struct timeDiff structTM2structTD(struct tm input) {
	const struct timeDiff result = {
		tm_sec: input.tm_sec, tm_min: input.tm_min, tm_hour: input.tm_hour,
		tm_wday: input.tm_mday, tm_week: 0, tm_mon: input.tm_mon,
		tm_year: input.tm_year
	};
	return result;
}
const static inline struct timeDiff __time_rel_long(const time_t lesser_time, const time_t greater_time) {
	struct tm __greater_time; gmtime_r(&greater_time, &__greater_time);
	struct tm __lesser_time; gmtime_r(&lesser_time, &__lesser_time);
	const struct timeDiff result = __time_rel_long(structTM2structTD(__lesser_time), structTM2structTD(__greater_time));
	return result;
}
const static inline struct timeDiff __time_rel_long(const double lesser_time, const double greater_time) {
	const time_t __greater_time = time_t(greater_time);
	const time_t __lesser_time = time_t(lesser_time);
	struct timeDiff result = __time_rel_long(__lesser_time, __greater_time);
	result.tm_sec += getFrac(greater_time-lesser_time, 100)/100.0;
	return result;
}

const static inline string time_rel_abbrev(const double &lesser_time, const double &greater_time) {
	struct timeDiff result;
	if((greater_time - lesser_time) > (secPerDay * dayPerWeek * 4)) {
		result = __time_rel_long(lesser_time, greater_time);
	} else {
		result = __time_rel(lesser_time, greater_time);
	}
	string tmp;
	char buf[64]; bzero(buf, 64);
	if(result.tm_year) {
		snprintf(buf, 63, "%dy", result.tm_year);
		tmp = buf;
	}
	if(result.tm_mon) {
		snprintf(buf, 63, "%s%s%dm", tmp.c_str(), (!tmp.empty() ? " " : ""), result.tm_mon);
		tmp = buf;
	}
	if(result.tm_week) {
		snprintf(buf, 63, "%s%s%dw", tmp.c_str(), (!tmp.empty() ? " " : ""), result.tm_week);
		tmp = buf;
	}
	if(result.tm_wday) {
		snprintf(buf, 63, "%s%s%dd", tmp.c_str(), (!tmp.empty() ? " " : ""), result.tm_wday);
		tmp = buf;
	}
	snprintf(buf, 63, "%s%s%02d:%02d:%02d.%02d", tmp.c_str(), (!tmp.empty() ? " " : ""),
		result.tm_hour, result.tm_min, (uint32_t)result.tm_sec,
		getFrac(greater_time-lesser_time, 100));
	return buf;
}

const static inline string time_rel_abbrev(const time_t lesser_time) {
	const time_t greater_time = time(NULL);
	return time_rel_abbrev(lesser_time, greater_time);
}
const static inline string time_rel_abbrev(const double lesser_time) {
	const double greater_time = getCurTime();
	return time_rel_abbrev(lesser_time, greater_time);
}

#endif
