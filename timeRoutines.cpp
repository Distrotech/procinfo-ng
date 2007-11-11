#ifndef TIMEROUTINES_CPP
#define TIMEROUTINES_CPP

#include <time.h>

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

#define secPerMin 60
#define minPerHour 60
#define hourPerDay 24
#define dayPerWeek 7
#define secPerHour secPerMin*minPerHour
#define secPerDay secPerMin*minPerHour*hourPerDay

const static inline struct timeWDHMS splitTime(uint32_t difference) {
	struct timeWDHMS time;
	time.seconds = (int)(difference % secPerMin);
	difference = (difference - (uint32_t)time.seconds) / secPerMin;
	time.minutes = (int)(difference % minPerHour);
	difference = (difference - time.minutes) / minPerHour;
	time.hours = (int)(difference % hourPerDay);
	difference = (difference - time.hours) / hourPerDay;
	time.days = (int)(difference % dayPerWeek);
	time.weeks = (int)((difference - time.days) / dayPerWeek);

	return time;
}

const static inline struct timeWDHMS splitTime(const double &difference) {
	struct timeWDHMS time;

	uint64_t difference2 = (uint64_t)(difference / secPerMin);

	time.seconds = (difference - (difference2 * secPerMin));
	time.minutes = (int)(difference2 % minPerHour);
	difference2 = (uint64_t)(difference2 - time.minutes) / minPerHour;
	time.hours = (int)(difference2 % hourPerDay);
	difference2 = (difference2 - time.hours) / hourPerDay;
	time.days = (int)(difference2 % dayPerWeek);
	time.weeks = (int)((difference2 - time.days) / secPerDay);

	return time;
}

static inline const int get_monthdays(const int month, const int year) {
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

struct tm __time_rel_long() {
}
/* Nonzero if YEAR is a leap year (every 4 years,
   except every 100th isn't, and every 400th is).  */
// In case the macro isn't available in time.h
//# define __isleap(year) \
//  !!((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))


/*
# This is for cases over 4 weeks, when we need years, months, weeks, and days
sub __time_rel_long($;$) {
        my ($lesser_time, $greater_time) = @_;
        $greater_time = time() unless $greater_time;

        my ($sec1, $min1, $hour1, $mday1, $month1, $year1, undef, undef, undef) = gmtime($lesser_time);
        my ($sec2, $min2, $hour2, $mday2, $month2, $year2, undef, undef, undef) = gmtime($greater_time);

        my ($result_years, $result_months, $result_weeks, $result_days,
                $result_hours, $result_mins, $result_secs);
        $result_secs = $sec2 - $sec1;
        $result_mins = $min2 - $min1;
        if($result_secs < 0) {
                $result_secs += 60; $result_mins--;
        }
        $result_hours = $hour2 - $hour1;
        if($result_mins < 0) {
                $result_mins += 60; $result_hours--;
        }
        $result_days = $mday2 - $mday1;
        if($result_hours < 0) {
                $result_hours += 24; $result_days--;
        }
        $result_months = $month2 - $month1;
        if($result_days < 0) {
                $result_days += get_monthdays(
                        ($month2 == 0 ? 11 : $month2 - 1),
                        ($month2 == 0 ? $year2 - 1: $year2));
                $result_months--;
        }
        $result_weeks = $result_days / 7;
        $result_days = $result_days % 7;
        $result_years = $year2 - $year1;
        if($result_months < 0) {
                $result_months += 12; $result_years--
        }
        return ($result_years, $result_months, $result_weeks, $result_days, $result_hours, $result_mins, $result_secs);
}
*/

#endif
