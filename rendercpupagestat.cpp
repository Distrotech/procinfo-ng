// accepts multiple CPU statistics for rendering
// returns a single row.
inline vector <string> renderCPUstat(bool perSecond, bool showTotals, const double &elapsed,
	const uint32_t &CPUcount, const uint64_t &cpuTotal, const uint64_t &cpuDiff, const string &name)
	__attribute((always_inline)); // has only one call site.
inline vector <string> renderCPUstat(bool perSecond, bool showTotals, const double &elapsed,
	const uint32_t &CPUcount, const uint64_t &cpuTotal, const uint64_t &cpuDiff, const string &name)
{

	struct timeWDHMS timeDiff = splitTime(cpuDiff / 
		( (double)USER_HZ * ( name == "uptime:" ? 1 :
		(!perSecond || elapsed == 0 || showTotals ? 1 : elapsed)) )
	);
	char buf[64]; bzero(buf, 64);
	string output;
	if(name == "uptime:") {
		char fractionalSeconds[3];
		snprintf(fractionalSeconds, 2, "%2d", getFrac(cpuDiff / USER_HZ, 100));
		output += time_rel_abbrev( getCurTime() - (cpuDiff / double(USER_HZ)) );
	} else {
		if(timeDiff.weeks) {
			snprintf(buf, 63, "%3dw ", timeDiff.weeks);
			output += buf;
		}
		if(timeDiff.days) {
			snprintf(buf, 63, "%dd ", timeDiff.days);
			output += buf;
		}
		snprintf(buf, 63, "%02d:%02d:%02d.%02d", timeDiff.hours, timeDiff.minutes,
			(uint32_t)timeDiff.seconds, getFrac(timeDiff.seconds, 100));
		output += buf;
	}
	if( name != "uptime:" ) {
		bzero(buf, 64);
// ADJUSTFACTOR is a cygwin/win32 hack. Hopefully there's a better way.
// However, w/o this, the percentage figures are screwed.
#ifdef __CYGWIN__
#define ADJUSTFACTOR 10
#else
#define ADJUSTFACTOR 1
#endif
		if(elapsed != 0) {
			snprintf(buf, 63, "%6.1f%%", 
				double(cpuDiff) / double( (showTotals ? cpuTotal / USER_HZ : elapsed * CPUcount) ) /
					ADJUSTFACTOR
			);
		} else {
			snprintf(buf, 63, "%6.1f%%", 
				double(cpuDiff) / ( double(cpuTotal) / USER_HZ ) / ADJUSTFACTOR
			);
		}
		output += buf;
	} else {
		output += "       ";
	}
	

	vector<string> row;
	row.push_back(name); row.push_back(output);

	return row;
}

// accepts a single page statistic for rendering
// returns a single row.
inline vector <string> renderPageStat(bool perSecond, bool showTotals, double elapsed, const uint64_t &pageDiff, const string &name) {
	char buf[64]; bzero(buf, 64);
#if __WORDSIZE == 64
	snprintf(buf, 63, "%15lu",
#else
	snprintf(buf, 63, "%15llu",
#endif
		uint64_t(pageDiff / (perSecond && !showTotals ? 
		( elapsed == 0 ? 1 : elapsed) : 1)));
	
	vector<string> row;
	row.push_back(name); row.push_back(string(buf));

	return row;
}
