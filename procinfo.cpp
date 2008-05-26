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
#define VERSION "2.0"
#define REVISION "$Rev$"

// This really should use linkable objects, not includes. -.-
struct IRQ {
	uint16_t IRQnum;
	string devs;
};

vector <struct IRQ> getIRQs() {
	vector <string> lines = readFile("/proc/interrupts");
	
	vector <struct IRQ> IRQs;
	for(uint32_t i = 0; i < lines.size(); i++) {
		struct IRQ irq;
		vector <string> tokens = splitString(" ", lines[i]);
		if (!tokens.size()) continue;
		// we need a char array b/c of isdigit below.
		const char *irqToken = tokens[0].c_str();
		if( !(strlen(irqToken) && isdigit(irqToken[0])) ) {
			continue;
		}

		string devs; uint32_t j;
		for(j = 0; j < tokens.size(); j++)
			if (tokens[j].find("PIC", 0) != string::npos) {
				break;
			}
			else if (tokens[j].find("MSI", 0) != string::npos) {
				break;
			}
			else if (tokens[j].find("-irq", 0) != string::npos) {
				break;
			}
		for(j++; j < tokens.size(); j++)
			devs = devs + " " + tokens[j];
		irq.IRQnum = (uint16_t)string2uint32(irqToken);
		irq.devs = devs;
		IRQs.push_back(irq);
	}
	return IRQs;
}

vector <uint64_t> getIRQcount() {
	vector <string> lines = readFile("/proc/interrupts");
	
	vector <uint64_t> IRQcount;
	for(uint32_t i = 0; i < lines.size(); i++) {
		//struct IRQ irq;
		vector <string> tokens = splitString(" ", lines[i]);
		if (!tokens.size()) continue;
		// we need a char array b/c of isdigit below.
		const char *irqToken = tokens[0].c_str();
		if( !(strlen(irqToken) && isdigit(irqToken[0])) ) {
			continue;
		}
		uint32_t irqNum = string2uint32(irqToken);

		uint32_t j;
		for(j = 1; j < tokens.size() - 1; j++) {
			if( tokens[j].length() && isdigit(tokens[j][0])  ) {
				if(IRQcount.size() < irqNum+1) {
					IRQcount.resize(irqNum+1, 0);
				}
				IRQcount[irqNum] += string2uint64(tokens[j]);
			}
			else if (tokens[j].find("PIC", 0) != string::npos) {
				break;
			}
			else if (tokens[j].find("MSI", 0) != string::npos) {
				break;
			}
			else if (tokens[j].find("-irq", 0) != string::npos) {
				break;
			}
		}
	}
	return IRQcount;
}

#ifdef __CYGWIN__
#include "cygwin_procstat.cpp"
#else
#include "linux26_procstat.cpp"
#endif

// Unlike most get* functions, this one does the rendering too.
// as such it returns a list of rows like any other render* function
// that is called by mainLoop()
vector <vector <string> > getMeminfo(bool perSecond, bool showTotals, bool showRealMemFree, const double &elapsed) {
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
	row.push_back(int64toString(int64_t(MemTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row.push_back(int64toString(int64_t((MemTotalDiff - MemFreeDiff) / 
		(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row.push_back(int64toString(int64_t(MemFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row.push_back(int64toString(int64_t(BuffersDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	rows.push_back(row);
	row.clear();

	if(showRealMemFree) { // Produces free memory figures that consider Buffers + Cache as disposable.
		//int64_t BuffCacheUsed = int64_t(((MemTotalDiff - MemFreeDiff) - (BuffersDiff + CacheDiff)) /
		int64_t BuffCacheUsed = int64_t(((BuffersDiff + CacheDiff)) / 
			(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)));
		int64_t BuffCacheFree = int64_t((MemFreeDiff + (BuffersDiff + CacheDiff)) / (
			!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)));
		row.push_back("-/+ buffers/cache  ");
		row.push_back("");
		row.push_back(int64toString(BuffCacheUsed));
		row.push_back(int64toString(BuffCacheFree));
		rows.push_back(row);
		row.clear();
	}

	row.push_back("Swap:");
	row.push_back(int64toString(int64_t(SwapTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row.push_back(int64toString(int64_t((SwapTotalDiff - SwapFreeDiff) / 
		(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row.push_back(int64toString(int64_t(SwapFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	rows.push_back(row);
	row.clear();

	return rows;
}

// accepts multiple CPU statistics for rendering
// returns a single row.
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
		snprintf(buf, 63, "%dw ", timeDiff.weeks);
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
		char percentBuf[64]; bzero(percentBuf, 64); bzero(buf, 64);
// ADJUSTFACTOR is a cygwin/win32 hack. Hopefully there's a better way.
// However, w/o this, the percentage figures are screwed.
#ifdef __CYGWIN__
#define ADJUSTFACTOR 10
#else
#define ADJUSTFACTOR 1
#endif
		snprintf(percentBuf, 63, "%3.1f", 
			(double)cpuDiff / ( (showTotals || elapsed == 0 ? cpuTotal / USER_HZ : 
			(elapsed == 0 ? 1 : elapsed) * CPUcount)) / ADJUSTFACTOR
		);
		snprintf(buf, 63, " %5s%%", percentBuf);
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

inline string renderIRQ(bool perSecond, bool showTotals, const double &elapsed, const struct IRQ &irq, const uint64_t &intrDiff) {
	char buf[64]; bzero(buf, 64);
	string output;

	snprintf(buf, 63, "irq %3d:", irq.IRQnum); 
	output += buf; bzero(buf, 64);
	char countBuf[64]; bzero(countBuf, 64);
#if __WORDSIZE == 64
	// uint64_t is 'long unsigned int' here
	snprintf(countBuf, 63, "%lu", uint64_t(intrDiff / (perSecond && !showTotals ? ( elapsed ? elapsed : 1) : 1)));
#else 
	// uint64_t is 'long long unsigned int' here
	snprintf(countBuf, 63, "%llu", uint64_t(intrDiff / (perSecond && !showTotals ? ( elapsed ? elapsed : 1) : 1)));
#endif
	snprintf(buf, 63, "%10s %-18s", countBuf, irq.devs.substr(0, 18).c_str());
	output += string(string(" ") + buf);

	return output;
}

vector< vector <string> > renderIRQs(bool perSecond, bool showTotals, const double &elapsed,
	const vector <struct IRQ> &IRQs, const vector <uint64_t> &intrDiffs)
{
	vector<vector <string> > rows;
	uint32_t split = IRQs.size() / 2 + (IRQs.size() & 1); // is equiv to (IRQs.size() % 2)
	for(uint32_t i = 0; i < split; i++) {
		vector <string> row;
		row.push_back( renderIRQ(perSecond, showTotals, elapsed, IRQs[i], intrDiffs[IRQs[i].IRQnum]) );
		if(i+split < IRQs.size())
			row.push_back( 
				renderIRQ(perSecond, showTotals, elapsed, IRQs[i+split], intrDiffs[IRQs[i+split].IRQnum]) );
		rows.push_back(row);
		
	}
	return rows;
}

inline uint32_t getCPUcount() {
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

struct diskStat_t {
	bool display;
	uint32_t major, minor;
	string name;
	vector <uint64_t> stats;
};

vector <struct diskStat_t> getDiskStats(bool showTotals) {
	static vector <struct diskStat_t> oldDiskStats;
	vector <struct diskStat_t> diskStatDiffs;

	vector <string> lines = readFile("/proc/diskstats");
	uint32_t offset = 0; // we skip some lines.
	for(uint32_t i = 0; i < lines.size(); i++) {
		if(lines[i].size() <= 1) {
			offset++;
			continue;
		}
		vector <string> tokens = splitString(" ", lines[i]);
		if(tokens.size() != 14) {
			offset++;
			continue;
		}
		
		struct diskStat_t diskStat = {
			false,
			string2uint32(tokens[0]), string2uint32(tokens[1]),
			tokens[2],
			vector <uint64_t>(11,0)
		};
		struct diskStat_t diskDiff = {
			false,
			string2uint32(tokens[0]), 
			string2uint32(tokens[2]),
			tokens[2],
			vector <uint64_t>(11,0)
		};
		tokens.erase(tokens.begin(), tokens.begin()+3);
		diskStat.stats = stringVec2uint64Vec(tokens);
		if(oldDiskStats.size() < i + 1) {
			struct diskStat_t tmpObj = {
				false,
				0, 0,
				"",
				vector <uint64_t>(11,0)
			};
			
			oldDiskStats.push_back(tmpObj);
		}
		if( (diskStat.stats[0] || diskStat.stats[4]) ||
			( (diskStat.name[0] == 'h' || diskStat.name[0] == 's' ) && diskStat.name[1] == 'd' ) )
		{
			diskDiff.display = true;
		}
		if(!showTotals) {
			diskDiff.stats = subUint64Vec(diskStat.stats, oldDiskStats[i-offset].stats);
		} else {
			diskDiff.stats = diskStat.stats;
		}
		diskStatDiffs.push_back(diskDiff);
		oldDiskStats[i-offset] = diskStat;
	}

	return diskStatDiffs;
}

vector< vector <string> > renderDiskStats(bool perSecond, bool showTotals, bool showSectors, const double &elapsed,
	const vector <struct diskStat_t> &diskStats)
{
	vector< string> entries;
	for(uint32_t i = 0; i < diskStats.size(); i++) {
		if(!diskStats[i].display)
			continue;
		char output[40]; bzero(output, 40);
#if __WORDSIZE == 64
		snprintf(output, 39, "%-4s %15lur %15luw", diskStats[i].name.c_str(),
#else
		snprintf(output, 39, "%-4s %15llur %15lluw", diskStats[i].name.c_str(),
#endif
			(showSectors ? diskStats[i].stats[2]: diskStats[i].stats[0]),
			(showSectors ? diskStats[i].stats[6] : diskStats[i].stats[4]));
		entries.push_back(output);
	}
	vector< vector <string> > rows;
	uint32_t split = entries.size() / 2 + (entries.size() & 1); // is equiv to (entries.size() % 2)
	for(uint32_t i = 0; i < split; i++) {
		vector<string> row;
		row.push_back(entries[i]);
		if(entries.size() > i+split) 
			row.push_back(entries[i+split]);
		rows.push_back(row);
	}
	return rows;
}

//static termios oldTerm;
inline void initConsole() {
/*	static const uint32_t STDIN = 0;
	termios term;
	tcgetattr(STDIN, &term);
	oldTerm = term;
*/	/*
	  enables canonical mode
	  which for our purposes is
	  a fancy name for enabling various
	  raw chars like EOF, EOL, etc.
	*/
/*	term.c_lflag &= !ICANON;
	tcsetattr(STDIN, TCSANOW, &term);
	setbuf(stdin, NULL); // disables line-buffering on stdin
*/
	initscr(); // init ncurses
	cbreak();  // turn off line buffering, but leave Ctrl-C alone
	
}

inline void resetConsole() {
	//tcsetattr(0, TCSANOW, &oldTerm);
	endwin();
}

int mainLoop(bool perSecond, bool showTotals, bool showTotalsMem, bool fullScreen, bool showRealMemFree, bool showSectors,
	const uint32_t &CPUcount, const vector <struct IRQ> &IRQs)
{
	static double oldUptime = 0;

	vector<vector <string> > rows;

	double uptime = getUptime();
	double elapsed = ( oldUptime != 0 ? uptime - oldUptime : 0 );
	if(fullScreen)
		printf("\e[H");
	rows = getMeminfo(perSecond, showTotalsMem, showRealMemFree, elapsed);
	vector <uint32_t> rowWidth;
	rowWidth.push_back(6);
	rowWidth.push_back(10);
	rowWidth.push_back(10);
	rowWidth.push_back(10);
	rowWidth.push_back(10);
	prettyPrint(rows, rowWidth, false);
	rows.clear();
	//cout << endl;
	printw("\n");

/*
	vector <uint64_t> cpuDiff = stats[0];
	vector <uint64_t> intrDiff = stats[1];
	uint64_t ctxtDiff = stats[2][0];
	uint64_t bootTime = stats[2][1];
*/
	vector <vector <uint64_t> > stats = getProcStat(showTotals);

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
	printw("\n");

	rows = renderCPUandPageStats(perSecond, showTotals, elapsed, CPUcount, (uint64_t)(uptime * USER_HZ),
		 stats[0], stats[2][0], vmStat);
	prettyPrint(rows, false);
	rows.clear();
	//cout << endl;
	printw("\n");


	rows = renderIRQs(perSecond, showTotals, elapsed, IRQs, stats[1]);
	prettyPrint(rows, false);
	//cout << endl;
	printw("\n");
	rows.clear();

#ifndef __CYGWIN__
		vector <struct diskStat_t> diskStats = getDiskStats(showTotals);
		rows=renderDiskStats(perSecond, showTotals, showSectors, elapsed, diskStats);
		prettyPrint(rows, false);
		rows.clear();
#endif
#ifdef __linux__
	rowWidth.push_back(10);
	rowWidth.push_back(15);
	rowWidth.push_back(15);
	rowWidth.push_back(10);
	rowWidth.push_back(15);
	rowWidth.push_back(15);
	rows = getNetStats(perSecond, showTotals, elapsed);
	printw("\n");
	prettyPrint(rows, rowWidth, true);
#endif
	rows.clear();
	refresh();
	clear();
	
	oldUptime = uptime;
	return 0;
}

int main(int argc, char *argv[]) {
	double interval = DEFAULT_INTERVAL;
	bool perSecond = false, showTotals = true, showTotalsMem = true, fullScreen = false;
	bool showRealMemFree = false, showSectors = false;
	extern char *optarg;
	int c;
	if(argc > 1) {
		perSecond = false; showTotals = true; showTotalsMem = true;
		while((c = getopt(argc, argv, "n:N:fSDdrbhv")) != -1) {
		
			switch(c) {
				case 'n':
				case 'N':
					interval = string2double(optarg);
					// in case of a bum param. Can't allow interval <= 0
					interval = (interval > 0 ? interval : DEFAULT_INTERVAL);
					fullScreen = true;
					break;
				case 'f':
					fullScreen = true;
					break;
				case 'S':
					perSecond = true;
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
				case 'h':
				default:
					printf ("procinfo version %s %s\n"
						"usage: %s [-sfidDSbhv] [-nN]\n"
						"\n"
						"\t-s\tdisplay memory, disk, IRQ & DMA info (default)\n"
						"\t-f\trun full screen\n"
						"\n"
						"\t-nN\tpause N second between updates (implies -f)\n"
						"\t-d\tshow differences rather than totals (implies -f)\n"
						"\t-D\tshow current memory/swap usage, differences on rest\n"
						"\t-b\tshow number of blocks instead of requests for disk statistics\n"
						"\t-S\twith -nN and -d/-D, always show values per second\n"
						"\t-r\tshow memory usage -/+ buffers/cache\n"
						"\t-v\tprint version info\n"
						"\t-h\tprint this help\n",
						VERSION, REVISION, argv[0]);
					exit (c == 'h' ? 0 : 1);
			}
		}
	} else {
		perSecond = true;
		interval = 0;
		fullScreen = false;
	}

	if(fullScreen)
		printf("\e[2J");

	uint32_t CPUcount = getCPUcount();
	const struct timeval sleepInterval = { (int)interval, getFrac(interval, 1000000) };
	initConsole();
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
		mainLoop(perSecond, showTotals, showTotalsMem, fullScreen, showRealMemFree, showSectors, CPUcount, IRQs);
		if(interval == 0) {
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
			}
			if(key == 'q' || key == 'Q') {
				break;
			}
			printf("\e[2J\n");
		}
	};
	resetConsole();
	return 0;	
}
