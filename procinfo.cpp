/*
	This file is part of procinfo-NG

	procinfo-NG is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2.

	procinfo-NG is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with procinfo-NG; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

//#include "eventxx"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <termios.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <unistd.h>

#ifdef __CYGWIN__
#include <sys/select.h>
#endif

#include <ncurses.h>

using namespace std;

#include "routines.cpp"
#include "timeRoutines.cpp"
#include "prettyPrint.cpp"

#define DEFAULT_INTERVAL 5
#define USER_HZ sysconf(_SC_CLK_TCK)
// this might be wrong for optical, but it might not!

#define VERSION "2.0"
#define REVISION "$Rev$"

// This really should use linkable objects, not includes. -.-

#include "interrupts.cpp"

#ifdef __CYGWIN__
#include "cygwin_procstat.cpp"
#else
#include "linux26_procstat.cpp"
#endif

// Unlike most get* functions, this one does the rendering too.
// as such it returns a list of rows like any other render* function
// that is called by mainLoop()
vector <vector <string> > getMeminfo(bool perSecond, bool showTotals, bool showRealMemFree, bool humanizeNums,
	const double &elapsed) 
{
	vector <string> lines = readFile(string("/proc/meminfo"));

	static uint64_t oldMemFree = 0, oldMemTotal = 0, oldSwapTotal = 0, oldSwapFree = 0;
	static uint64_t oldCache = 0, oldBuffers = 0;

	// these have identical names to the keys in meminfo
	int64_t MemTotal = 0, MemFree = 0, Buffers = 0, SwapTotal = 0, SwapFree = 0;
	int64_t MemTotalDiff = 0, MemFreeDiff = 0, BuffersDiff = 0, SwapTotalDiff = 0, SwapFreeDiff = 0;
	int64_t Cache = 0, CacheDiff = 0;

	for(uint32_t i = 0; i < lines.size(); i++) {
		vector <string> tokens = splitString(" ", lines[i]);
		if (!tokens.size()) break;
		if(tokens[0] == "MemTotal:") {
			MemTotal = string2int64(tokens[1]);
			MemTotalDiff = (showTotals ? MemTotal : MemTotal - oldMemTotal);
			oldMemTotal = MemTotal;
		} else if(tokens[0] == "MemFree:") {
			MemFree = string2int64(tokens[1]);
			MemFreeDiff = (showTotals ? MemFree : MemFree - oldMemFree);
			oldMemFree = MemFree;
		} else if(tokens[0] == "Buffers:") {
			Buffers = string2int64(tokens[1]);
			BuffersDiff = (showTotals ? Buffers : Buffers - oldBuffers);
			oldBuffers = Buffers;
		} else if(tokens[0] == "Cached:") {
			Cache = string2int64(tokens[1]);
			CacheDiff = (showTotals ? Cache : Cache - oldCache);
			oldCache = Cache;
		} else if(tokens[0] == "SwapTotal:") {
			SwapTotal = string2int64(tokens[1]);
			SwapTotalDiff = (showTotals ? SwapTotal : SwapTotal - oldSwapTotal);
			oldSwapTotal = SwapTotal;
		} else if(tokens[0] == "SwapFree:") {
			SwapFree = string2int64(tokens[1]);
			SwapFreeDiff = (showTotals ? SwapFree : SwapFree - oldSwapFree);
			oldSwapFree = SwapFree;
		} 
	}
	vector <vector <string> > rows;
	vector <string> row;
	row.push_back("Memory:");
	row.push_back("Total");
	row.push_back("Used");
	row.push_back("Free");
	row.push_back("Buffers");
	rows.push_back(row);
	row.clear();

	row.push_back("RAM:");
	if(humanizeNums) {
		row.push_back(humanizeBigNums(int64_t(MemTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
		row.push_back(humanizeBigNums(int64_t((MemTotalDiff - MemFreeDiff) / 
			(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
		row.push_back(humanizeBigNums(int64_t(MemFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
		row.push_back(humanizeBigNums(int64_t(BuffersDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
	} else {
		row.push_back(int64toString(int64_t(MemTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
		row.push_back(int64toString(int64_t((MemTotalDiff - MemFreeDiff) / 
			(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
		row.push_back(int64toString(int64_t(MemFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
		row.push_back(int64toString(int64_t(BuffersDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	}
	rows.push_back(row);
	row.clear();

	if(showRealMemFree) { // Produces free memory figures that consider Buffers + Cache as disposable.
		int64_t BuffCacheUsed = int64_t(((BuffersDiff + CacheDiff)) / 
			(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)));
		int64_t BuffCacheFree = int64_t((MemFreeDiff + (BuffersDiff + CacheDiff)) / (
			!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)));
		row.push_back("-/+ buffers/cache  ");
		row.push_back("");
		if(humanizeNums) {
			row.push_back(humanizeBigNums(BuffCacheUsed << 10 ));
			row.push_back(humanizeBigNums(BuffCacheFree << 10 ));
		} else {
			row.push_back(int64toString(BuffCacheUsed));
			row.push_back(int64toString(BuffCacheFree));
		}
		rows.push_back(row);
		row.clear();
	}

	row.push_back("Swap:");
	if(humanizeNums) {
		row.push_back(humanizeBigNums(int64_t(SwapTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
		row.push_back(humanizeBigNums(int64_t((SwapTotalDiff - SwapFreeDiff) / 
			(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
		row.push_back(humanizeBigNums(int64_t(SwapFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
	} else {
		row.push_back(int64toString(int64_t(SwapTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
		row.push_back(int64toString(int64_t((SwapTotalDiff - SwapFreeDiff) / 
			(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
		row.push_back(int64toString(int64_t(SwapFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	}
	rows.push_back(row);
	row.clear();

	return rows;
}

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

#ifdef __CYGWIN__
#include "cygwin_rendercpupagestat.cpp"
#else
#include "linux26_rendercpupagestat.cpp"
#endif

#ifdef __linux__
#include "linux26_netstat.cpp"
#endif

double getUptime() {
	getUptime_label:
	vector <string> lines = readFile(string("/proc/uptime"));
	if(lines.size() == 0) { goto getUptime_label; };
	vector <string> tokens = splitString(" ", lines[0]);
	return string2double(tokens[0]);
}

string getLoadAvg() {
	vector <string> lines = readFile(string("/proc/loadavg"));
	return lines[0];
}

vector <string> renderBootandLoadAvg(const time_t &bootTime, const string &loadAvg) {
	vector <string> row;
	
	string bootTimeStr = string(ctime(&bootTime));
	// remove the "\n". don't ask me why ctime does that...
	bootTimeStr.erase(bootTimeStr.end()-1);
	row.push_back(string(string("Bootup: ") + bootTimeStr));
	row.push_back(string("Load average: " + loadAvg));
	return row;
}

inline uint32_t getCPUcount() __attribute__((always_inline));
inline uint32_t getCPUcount() { // has only one call-site.
	vector <string> lines = readFile(string("/proc/cpuinfo"));
	uint32_t CPUcount = 0;
	for(uint32_t i = 0; i < lines.size(); i++) {
		vector <string> tokens = splitString(" ", lines[i]);
		//printf("getCPUcount token0: %s\n", tokens[0].c_str());
		if (tokens.size() && tokens[0] == "processor") { // x86/x86_64 Cygwin
			CPUcount++;
		} else if (tokens.size() && tokens[0] == "processor\t:") { // x86/x86_64 Linux
			CPUcount++;
		} else if(tokens.size() && tokens[0] == "ncpus") { // SPARC
			CPUcount = string2uint32(tokens[2]); // untested, I don't have a SPARC yet
			break;
		} else if(tokens.size() && tokens[0] == "cpus" && tokens[1] == "detected\t:") { // Alpha
			CPUcount = string2uint32(tokens[2]); // untested, I don't have an Alpha yet
			break;
		} else {
			// do nothing
		}
	}
	return CPUcount;
}

#include "diskStats.cpp"

int mainLoop(bool perSecond, bool showTotals, bool showTotalsMem, bool fullScreen,
	bool showRealMemFree, bool showSectors, bool humanizeNums, bool partitionStats,
	const uint32_t CPUcount, const vector <struct IRQ> &IRQs)
{
	static double oldUptime = 0;

	vector<vector <string> > rows;

	double uptime = getUptime();
	double elapsed = ( oldUptime != 0 ? uptime - oldUptime : 0 );
	if(fullScreen) // returns to home-position on screen.
		printf("\e[H");
	rows = getMeminfo(perSecond, showTotalsMem, showRealMemFree, humanizeNums, elapsed);

	vector <uint32_t> rowWidth(5, 10);
	rowWidth[0] = 6;
/*
	rowWidth.push_back(6);
	rowWidth.push_back(10);
	rowWidth.push_back(10);
	rowWidth.push_back(10);
	rowWidth.push_back(10);
*/
	prettyPrint(rows, rowWidth, false);
	rows.clear();
	print("\n");

/*
	// This isn't code, but rather documenting the contents of stats[][]
	// Please don't delete it.
	vector <uint64_t> cpuDiff = stats[0];
	vector <uint64_t> intrDiff = stats[1];
	uint64_t ctxtDiff = stats[2][0];
	uint64_t bootTime = stats[2][1];
*/
	vector <vector <uint64_t> > stats = getProcStat(showTotals, CPUcount, elapsed);

	//uint64_t pageInDiff, pageOutDiff, swapInDiff, swapOutDiff;
	vector <uint64_t> vmStat;
//#ifndef __CYGWIN__
		vmStat = getVMstat(showTotals);
//#endif

	string loadAvg = getLoadAvg();
	rows.push_back( renderBootandLoadAvg((time_t) stats[2][1], loadAvg) );
	prettyPrint(rows, false);
	rows.clear();
	//cout << endl;
	print("\n");

	rows = renderCPUandPageStats(perSecond, showTotals, elapsed, CPUcount, (uint64_t)(uptime * USER_HZ),
		 stats[0], stats[2][0], vmStat);
	prettyPrint(rows, false);
	rows.clear();
	//cout << endl;
	print("\n");


	rows = renderIRQs(perSecond, showTotals, elapsed, IRQs, stats[1]);
	prettyPrint(rows, false);
	//cout << endl;
	print("\n");
	rows.clear();

#ifndef __CYGWIN__
		vector <struct diskStat_t> diskStats = getDiskStats(showTotals, partitionStats);
		rows = renderDiskStats(perSecond, showTotals, showSectors, elapsed, diskStats);
		prettyPrint(rows, false);
		rows.clear();
#endif
#ifdef __linux__
	rowWidth.clear();

	rowWidth.resize(6, 15);
	rowWidth[0] = rowWidth[3] = 10;
/*
	rowWidth.push_back(10);
	rowWidth.push_back(15);
	rowWidth.push_back(15);
	rowWidth.push_back(10);
	rowWidth.push_back(15);
	rowWidth.push_back(15);
*/
	rows = getNetStats(perSecond, showTotals, elapsed);
	print("\n");
	prettyPrint(rows, rowWidth, true);
#endif
	rows.clear();
	refresh();
	//clear();
	erase();
	
	oldUptime = uptime;
	return 0;
}

int main(int argc, char *argv[]) {
	double interval = DEFAULT_INTERVAL;
	bool perSecond = false, showTotals = true, showTotalsMem = true, fullScreen = false;
	bool showRealMemFree = false, showSectors = false;
	bool humanizeNums = false, partitionStats = false;
	bool repeat = false;
	extern char *optarg;
	int c;
	if(argc > 1) {
		perSecond = false; showTotals = true; showTotalsMem = true;
		while((c = getopt(argc, argv, "n:N:SDdrbhHvp")) != -1) {
		
			switch(c) {
				case 'n':
				case 'N':
					interval = string2double(optarg);
					// in case of a bum param. Can't allow interval <= 0
					interval = (interval > 0 ? interval : DEFAULT_INTERVAL);
					repeat = fullScreen = true;
					break;
				/*
				case 'f':
					// FIXME: 'f' has been removed from the options
					// as it always is in fullScreen mode now (ncurses)
					fullScreen = true;
					break;
				*/
				case 'S':
					perSecond = true;
					repeat = fullScreen = true;
					break;
				case 'D':
					showTotals = false;
					showTotalsMem = true;
					break;
				case 'd':
					showTotals = showTotalsMem = false;
					break;
				case 'r':
					showRealMemFree = true;
					break;
				case 'b':
					showSectors = true;
					break;
				case 'v':
					printf("procinfo version %s\n", VERSION);
					exit(0);
					break;
				case 'H':
					humanizeNums = true;
					break;
				case 'p':
					partitionStats = true;
					break;
				case 'h':
				default:
					printf ("procinfo version %s %s\n"
						"usage: %s [-sidDSbhHv] [-nN]\n"
						"\n"
						"\t-nN\tpause N second between updates (implies -f)\n"
						"\t-d\tshow differences rather than totals (implies -f)\n"
						"\t-D\tshow current memory/swap usage, differences on rest\n"
						"\t-S\twith -nN and -d/-D, always show values per second\n"
						"\t-b\tshow number of bytes instead of requests for disk statistics\n"
						"\t-H\tshow memory stats in KiB/MiB/GiB\n"
						"\t-r\tshow memory usage -/+ buffers/cache\n"
						"\t-h\tprint this help\n",
						"\t-v\tprint version info\n"
						VERSION, REVISION, argv[0]);
					exit (c == 'h' ? 0 : 1);
			}
		}
	} else {
		perSecond = true;
		interval = 0;
		fullScreen = false;
	}

	if(fullScreen) {
		printf("\e[2J");
		initConsole();
	}

	uint32_t CPUcount = getCPUcount();
	const struct timeval sleepInterval = { (int)interval, getFrac(interval, 1000000) };
	
#ifdef __CYGWIN__
	const vector <struct IRQ> IRQs;
#else
	const vector <struct IRQ> IRQs = getIRQs();
#endif
	while(1) {
		fd_set fdSet;
		FD_ZERO(&fdSet);
		FD_SET(0, &fdSet);
		struct timeval sleepTime = sleepInterval; // select can modify sleepTime
		mainLoop(perSecond, showTotals, showTotalsMem, fullScreen,
			showRealMemFree, showSectors, humanizeNums, partitionStats,
			CPUcount, IRQs);
		if(interval == 0 || repeat == false) {
			break;
		}
		int ret = select(1, &fdSet, NULL, NULL, &sleepTime);
		if(ret > 0) {
			char key = getchar();
			switch(key) {
				case 'f':
					fullScreen = !fullScreen;
					break;
				case 'S':
					perSecond = !perSecond;
				case 'D':
					showTotals = !showTotals;
					break;
				case 'd':
					showTotalsMem = !showTotalsMem;
					break;
				case 'r':
					showRealMemFree = !showRealMemFree;
					break;
				case 'b':
					showSectors = !showSectors;
					break;
				case 'h':
				case 'H':
					humanizeNums = !humanizeNums;
					break;
				case 'p':
				case 'P':
					partitionStats = !partitionStats;
					break;
			}
			if(key == 'q' || key == 'Q') {
				break;
			}
			printf("\e[2J\n");
			clear();
		}
	};
	if(fullScreen)
		resetConsole();
	return 0;	
}
