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

using namespace std;

#include "routines.cpp"

#define DEFAULT_INTERVAL 5
#define USER_HZ sysconf(_SC_CLK_TCK)

// inlined b/c it only has ONE caller.
// returns a list of uint32 column widths.
inline vector<uint32> getMaxWidths(vector<vector <string> > rows) {
	vector<uint32> colWidths;

	for(uint32 i = 0; i < rows.size(); i++)
		for(uint32 j = 0; j < rows[i].size(); j++) {
			if(colWidths.size() < j+1)
				colWidths.resize(j+1);
			if(colWidths[j] < rows[i][j].length())
				colWidths[j] = rows[i][j].length();
		}

	return colWidths;
}

// accepts a list of rows containing columns, an optional static list of column-widths and leftJustify
// returns nothing
void prettyPrint(vector <vector <string> > rows, vector<uint32> *colWidthsPtr, bool leftJustify) {
	vector <uint32> colWidths;
	static const string spaces =
		"                                                                                ";

	if(colWidthsPtr == NULL) {
		colWidths = getMaxWidths(rows);
	} else {
		colWidths = *colWidthsPtr;
	}

	for(uint32 i = 0; i < rows.size(); i++) {
		string line;
		for(uint32 j = 0; j < rows[i].size(); j++) {
			char fmt[11];
			if(!leftJustify) {
				snprintf(fmt, 10, "%%%s%ds", (!j ? "-" : ""), colWidths[j] + 1);
			} else {
				snprintf(fmt, 10, "%%-%ds", colWidths[j] + 1);
			}
			char subline[101];
			snprintf(subline, 100, fmt, rows[i][j].c_str());
			line = line + subline + ((j + 1) == rows[i].size() ? "" : " ");
		}

		static const signed int lineLength = 80;
		cout << line
			<< spaces.substr(0, max( (lineLength - (int)line.length()), (int)0) )
			<< endl;
	}
}

// Don't use this for large files,
// b/c it slurps the whole thing into RAM.
// Also, it _will_ fail_ for lines over ~4094 bytes
vector <string> readFile(string fileName) {
	vector <string> lines;
	ifstream file(fileName.c_str());

	for(uint32 i = 0; !file.eof(); i++) {
		char str[4096];
		file.getline(str, 4094);
		lines.push_back(string(str));
	}
	return lines;
}

// Unlike most get* functions, this one does the rendering too.
// as such it returns a list of rows like any other render* function
// that is called by mainLoop()
vector <vector <string> > getMeminfo(bool perSecond, bool showTotals, bool showRealMemFree, double elapsed) {
	vector <string> lines = readFile(string("/proc/meminfo"));

	static uint64 oldMemFree = 0, oldMemTotal = 0, oldSwapTotal = 0, oldSwapFree = 0;
	static uint64 oldCache = 0, oldBuffers = 0;

	// these have identical names to the keys in meminfo
	int64 MemTotal = 0, MemFree = 0, Buffers = 0, SwapTotal = 0, SwapFree = 0;
	int64 MemTotalDiff = 0, MemFreeDiff = 0, BuffersDiff = 0, SwapTotalDiff = 0, SwapFreeDiff = 0;
	int64 Cache = 0, CacheDiff = 0;

	for(uint32 i = 0; i < lines.size(); i++) {
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
	vector <string> *row = new vector<string>;
	row->push_back("Memory:");
	row->push_back("Total");
	row->push_back("Used");
	row->push_back("Free");
	row->push_back("Buffers");
	rows.push_back(*row);
	delete row;

	row = new vector<string>;
	row->push_back("RAM:");
	row->push_back(int64toString(int64(MemTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row->push_back(int64toString(int64((MemTotalDiff - MemFreeDiff) / 
		(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row->push_back(int64toString(int64(MemFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row->push_back(int64toString(int64(BuffersDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	rows.push_back(*row);
	delete row;

	if(showRealMemFree) { // Produces free memory figures that consider Buffers + Cache as disposable.
		int64 BuffCacheUsed = int64(((MemTotalDiff - MemFreeDiff) - (BuffersDiff + CacheDiff)) / 
			(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)));
		int64 BuffCacheFree = int64((MemFreeDiff + (BuffersDiff + CacheDiff)) / (
			!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)));
		row = new vector<string>;
		row->push_back("-/+ buffers/cache");
		//row->push_back("");
		row->push_back(int64toString(BuffCacheUsed));
		row->push_back(int64toString(BuffCacheFree));
		rows.push_back(*row);
		delete row;
	}

	row = new vector<string>;
	row->push_back("Swap:");
	row->push_back(int64toString(int64(SwapTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row->push_back(int64toString(int64((SwapTotalDiff - SwapFreeDiff) / 
		(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row->push_back(int64toString(int64(SwapFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	rows.push_back(*row);
	delete row;

	return rows;
}

// returns multiple lists of uint64s, cpuDiffs, intrDiffs, and a list consisting of context-switches and the boot-time
vector <vector <uint64> > getProcStat(bool showTotals) {
	vector <string> lines = readFile(string("/proc/stat"));
	vector <uint64> cpuDiff, cpuStat, intrDiff, intrStat;

	static vector <uint64> oldCPUstat, oldIntrStat;
	static uint64 oldCtxtStat = 0;

	uint64 ctxtStat, ctxtDiff, bootTime;
	uint64 cpuTotal = 0;

	for(uint32 i = 0; i < lines.size(); i++) {
		vector <string> tokens = splitString(" ", lines[i]);
		if (!tokens.size()) break; // a) prevents SIGSEGV b) skips empty lines
		if(tokens[0] == "cpu") {
			tokens.erase(tokens.begin()); // pop the first token off.

			cpuStat = stringVec2uint64Vec(tokens);
			if(!oldCPUstat.size())
				oldCPUstat.resize(cpuStat.size());
			cpuDiff = (showTotals ? cpuStat : subUint64Vec(cpuStat, oldCPUstat));
			for(uint32 i = 0; i < cpuStat.size(); i++)
				cpuTotal += cpuStat[i];
			oldCPUstat.assign(cpuStat.begin(), cpuStat.end());
			cpuDiff.push_back(cpuTotal);
		} else if(tokens[0] == "intr") {
			// We don't want the second token b/c it's just the total number of interrupts serviced.
			tokens.erase(tokens.begin()); // pop the first token off.
			tokens.erase(tokens.begin()); // pop the second token off.

			intrStat = stringVec2uint64Vec(tokens);
			if(!oldIntrStat.size())
				oldIntrStat.resize(intrStat.size());
			intrDiff = (showTotals ? intrStat : subUint64Vec(intrStat, oldIntrStat));
			oldIntrStat.assign(intrStat.begin(), intrStat.end());
		} else if(tokens[0] == "ctxt") {
			ctxtStat = string2uint64(tokens[1]);
			ctxtDiff = (showTotals ? ctxtStat : ctxtStat - oldCtxtStat);
			oldCtxtStat = ctxtStat;
		} else if(tokens[0] == "btime") {
			bootTime = string2uint64(tokens[1]);
		}
	}
	vector <vector <uint64> > stats;
	stats.resize(3);
	stats[0] = cpuDiff;
	stats[1] = intrDiff;
	stats[2].push_back(ctxtDiff);
	stats[2].push_back(bootTime);
	return stats;
}

// returns the contents of /proc/vmstat, only the parts we want.
// as such it returns a vector of 4 elements, pageInDiff, pageOutDiff, swapInDiff, swapOutDiff
vector <uint64> getVMstat(bool showTotals) {
	vector <string> lines = readFile(string("/proc/vmstat"));

	static uint64 oldPageIn = 0, oldPageOut = 0, oldSwapIn = 0, oldSwapOut = 0;

	uint64 pageIn = 0, pageOut = 0, swapIn = 0, swapOut = 0;
	uint64 pageInDiff = 0, pageOutDiff = 0, swapInDiff = 0, swapOutDiff = 0;

	for(uint32 i = 0; i < lines.size(); i++) {
		vector <string> tokens = splitString(" ", lines[i]);
		if (!tokens.size()) break;
		if(tokens[0] == "pgpgin") {
			pageIn = string2uint64(tokens[1]);
			pageInDiff = (showTotals ? pageIn : pageIn - oldPageIn);
			oldPageIn = pageIn;
		} else if(tokens[0] == "pgpgout") {
			pageOut = string2uint64(tokens[1]);
			pageOutDiff = (showTotals ? pageOut : pageOut - oldPageOut);
			oldPageOut = pageOut;
		} else if(tokens[0] == "pswpin") {
			swapIn = string2uint64(tokens[1]);
			swapInDiff = (showTotals ? swapIn : swapIn - oldSwapIn);
			oldSwapIn = swapIn;
		} else if(tokens[0] == "pswpout") {
			swapOut = string2uint64(tokens[1]);
			swapOutDiff = (showTotals ? swapOut : swapOut - oldSwapOut);
			oldSwapOut = swapOut;
		} 
	}
	vector <uint64> vmStat;
	vmStat.push_back(pageInDiff);
	vmStat.push_back(pageOutDiff);
	vmStat.push_back(swapInDiff);
	vmStat.push_back(swapOutDiff);
	return vmStat;
}

// accepts multiple CPU statistics for rendering
// returns a single row.
inline vector <string> renderCPUstat(bool perSecond, bool showTotals, double elapsed, 
	uint32 CPUcount, uint64 cpuTotal, uint64 cpuDiff, string name) 
{

	struct timeWDHMS timeDiff = splitTime(cpuDiff / 
		( (double)USER_HZ * ( name == "uptime:" ? 1 :
		(!perSecond || elapsed == 0 || showTotals ? 1 : elapsed)) )
	);
	char *buf = new char[64]; bzero(buf, 63);
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
		(uint32)timeDiff.seconds, getFrac(timeDiff.seconds, 100));
	output += buf;
	if( name != "uptime:" ) {
		char *percentBuf = new char[64]; bzero(percentBuf, 63); bzero(buf, 63);
		snprintf(percentBuf, 63, "%3.1f", 
			(double)cpuDiff / ( (showTotals || elapsed == 0 ? cpuTotal / USER_HZ : 
			(elapsed == 0 ? 1 : elapsed) * CPUcount))
		);
		snprintf(buf, 63, " %5s%%", percentBuf);
		output = output + buf;
		delete percentBuf;
	} else {
		output += "       ";
	}
	delete buf;
	

	vector<string> row;
	row.push_back(name); row.push_back(output);

	return row;
}

// accepts a single page statistic for rendering
// returns a single row.
inline vector <string> renderPageStat(bool perSecond, bool showTotals, double elapsed, uint64 pageDiff, string name) {
	char *buf = new char[64]; bzero(buf, 63);
	snprintf(buf, 63, "%15llu", 
		uint64(pageDiff / (perSecond && !showTotals ? 
		( elapsed == 0 ? 1 : elapsed) : 1)));
	
	vector<string> row;
	row.push_back(name); row.push_back(string(buf));
	delete buf;

	return row;
}

// uses renderPageStat and renderCPUstats to render both CPU and page stats
// returns a list of rows containing 4 columns.
vector< vector <string> > renderCPUandPageStats(bool perSecond, bool showTotals, double elapsed,
	uint64 CPUcount, uint64 uptime, vector <uint64> cpuDiffs, uint64 ctxtDiff, vector <uint64> pageDiffs)
{
	vector< vector <string> > rows;
	vector<string> row;
	vector <string> names;
	names.push_back(string("user  :")); names.push_back(string("page in :"));
	names.push_back(string("nice  :")); names.push_back(string("page out:"));
	names.push_back(string("system:")); names.push_back(string("swap in :"));
	names.push_back(string("idle  :")); names.push_back(string("swap out:"));
	names.push_back(string("uptime:")); names.push_back(string("context :"));
	for(uint32 i = 0; i <= 4; i++) {
		vector<string> cols = renderCPUstat(perSecond, showTotals, elapsed, CPUcount, cpuDiffs[8], 
			(i == 4 ? uptime : cpuDiffs[i]), names[i*2]);
		row.push_back(cols[0]); row.push_back(cols[1]);

		cols = renderPageStat(perSecond, showTotals, elapsed,
			( i == 4 ? ctxtDiff : pageDiffs[i]), names[i*2+1]);
		row.push_back(cols[0]); row.push_back(cols[1]);

		rows.push_back(row); row.clear();
	}

	return rows;
}

struct IRQ {
	uint32 IRQnum;
	string devs;
};

vector <struct IRQ> getIRQs() {
	vector <string> lines = readFile(string("/proc/interrupts"));
	
	vector <struct IRQ> IRQs;
	for(uint32 i = 0; i < lines.size(); i++) {
		struct IRQ irq;
		vector <string> tokens = splitString(" ", lines[i]);
		if (!tokens.size()) continue;
		const char *irqToken = tokens[0].c_str();
		if(!(strlen(irqToken) && isdigit(irqToken[0]))) {
			continue;
		}

		string devs; uint32 j;
		for(j = 0; j < tokens.size(); j++)
			if (tokens[j].find("PIC", 0) != string::npos)
				break;
		for(j++; j < tokens.size(); j++)
			devs = devs + " " + tokens[j];
		irq.IRQnum = strtoul(irqToken, (char **)NULL, 10);
		irq.devs = devs;
		IRQs.push_back(irq);
	}
	return IRQs;
}

double getUptime() {
	vector <string> lines = readFile(string("/proc/uptime"));
	vector <string> tokens = splitString(" ", lines[0]);
	return strtod(tokens[0].c_str(), (char **)NULL);
}

string getLoadAvg() {
	vector <string> lines = readFile(string("/proc/loadavg"));
	return lines[0];
}

vector <string> renderBootandLoadAvg(time_t bootTime, string loadAvg) {
	vector <string> row;
	
	string bootTimeStr = string(ctime(&bootTime));
	// remove the "\n". don't ask me why ctime does that...
	bootTimeStr.erase(bootTimeStr.end()-1);
	row.push_back(string(string("Bootup: ") + bootTimeStr));
	row.push_back(string("Load average: " + loadAvg));
	return row;
}

inline string renderIRQ(bool perSecond, bool showTotals, double elapsed, struct IRQ irq, uint64 intrDiff) {
	char buf[64]; bzero(buf, 63);
	string output;

	snprintf(buf, 63, "irq %3d:", irq.IRQnum); 
	output += buf; bzero(buf, 63);
	char countBuf[64]; bzero(countBuf, 63);
	snprintf(countBuf, 63, "%llu", uint64(intrDiff / (perSecond && !showTotals ? ( elapsed ? elapsed : 1) : 1)));
	snprintf(buf, 63, "%9s %-19s", countBuf, irq.devs.substr(0, 19).c_str());
	output = output + " " + buf; bzero(countBuf, 63); bzero(buf, 63);

	return output;
}

vector< vector <string> > renderIRQs(bool perSecond, bool showTotals, double elapsed,
	vector <struct IRQ> IRQs, vector <uint64> intrDiffs)
{
	vector<vector <string> > rows;
	uint32 split = IRQs.size() / 2 + 1;
	for(uint32 i = 0; i < split; i++) {
		vector <string> row;
		row.push_back( renderIRQ(perSecond, showTotals, elapsed, IRQs[i], intrDiffs[IRQs[i].IRQnum]) );
		if(i+split < IRQs.size())
			row.push_back( 
				renderIRQ(perSecond, showTotals, elapsed, IRQs[i+split], intrDiffs[IRQs[i+split].IRQnum]) );
		rows.push_back(row);
		
	}
	return rows;
}

inline uint32 getCPUcount() {
	vector <string> lines = readFile(string("/proc/cpuinfo"));
	uint32 CPUcount = 0;
	for(uint32 i = 0; i < lines.size(); i++) {
		vector <string> tokens = splitString(" ", lines[i]);
		if (tokens.size() && tokens[0] == "processor\t:") {
			CPUcount++;
		} else {
			continue;
		}
	}
	return CPUcount;
}

struct diskStat_t {
	bool display;
	uint32 major, minor;
	string name;
	vector <uint64> stats;
};

vector <struct diskStat_t> oldDiskStats;
vector <struct diskStat_t> getDiskStats(bool showTotals) {
	static vector <struct diskStat_t> oldDiskStats;
	vector <struct diskStat_t> diskStatDiffs;

	vector <string> lines = readFile("/proc/diskstats");
	uint32 offset = 0; // we skip some lines.
	for(uint32 i = 0; i < lines.size(); i++) {
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
			vector <uint64>(11,0)
		};
		struct diskStat_t diskDiff = {
			false,
			string2uint32(tokens[0]), 
			string2uint32(tokens[2]),
			tokens[2],
			vector <uint64>(11,0)
		};
		tokens.erase(tokens.begin(), tokens.begin()+3);
		diskStat.stats = stringVec2uint64Vec(tokens);
		if(oldDiskStats.size() < i + 1) {
			struct diskStat_t tmpObj = {
				false,
				0, 0,
				"",
				vector <uint64>(11,0)
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

vector< vector <string> > renderDiskStats(bool perSecond, bool showTotals, bool showSectors, double elapsed,
	vector <struct diskStat_t> diskStats)
{
	vector< string> entries;
	for(uint32 i = 0; i < diskStats.size(); i++) {
		if(!diskStats[i].display)
			continue;
		//vector<string> row;
		//row.push_back(diskStats[i].name + ":");
		char *output = new char[40];
		snprintf(output, 38, "%s: %15llur %15lluw", diskStats[i].name.c_str(), 
			(showSectors ? diskStats[i].stats[2]: diskStats[i].stats[0]),
			(showSectors ? diskStats[i].stats[6] : diskStats[i].stats[4]));
		//row.push_back(output);
		entries.push_back(output);
		delete output;
	}
	vector< vector <string> > rows;
	uint32 split = entries.size() / 2 + 1;
	for(uint32 i = 0; i < split; i++) {
		vector<string> row;
		row.push_back(entries[i]);
		if(entries.size() > i+split) 
			row.push_back(entries[i+split]);
		rows.push_back(row);
	}
	return rows;
}

static termios oldTerm;
inline void initConsole() {
	static const uint32 STDIN = 0;
	termios term;
	tcgetattr(STDIN, &term);
	oldTerm = term;
	/*
	  enables canonical mode
	  which for our purposes is
	  a fancy name for enabling various
	  raw chars like EOF, EOL, etc.
	*/
	term.c_lflag &= !ICANON;
	tcsetattr(STDIN, TCSANOW, &term);
	setbuf(stdin, NULL); // disables line-buffering on stdin
}

inline void resetConsole() {
	tcsetattr(0, TCSANOW, &oldTerm);
}

int mainLoop(bool perSecond, bool showTotals, bool showTotalsMem, bool fullScreen, bool showRealMemFree, bool showSectors,
	uint32 CPUcount)
{
	static double oldUptime = 0;

	vector<vector <string> > rows;

	double uptime = getUptime();
	double elapsed = ( oldUptime != 0 ? uptime - oldUptime : 0 );
	if(fullScreen)
		printf("\e[H");
	rows = getMeminfo(perSecond, showTotalsMem, showRealMemFree, elapsed);
	vector <uint32> *rowWidth = new vector <uint32>;
	rowWidth->push_back(6);
	rowWidth->push_back(10);
	rowWidth->push_back(10);
	rowWidth->push_back(10);
	rowWidth->push_back(10);
	prettyPrint(rows, rowWidth, false);
	delete rowWidth; rowWidth = NULL;
	rows.clear();
	cout << endl;

/*
	vector <uint64> cpuDiff = stats[0];
	vector <uint64> intrDiff = stats[1];
	vector <uint64> ctxtDiff AND bootTime = stats[2];
*/
	vector <vector <uint64> > stats = getProcStat(showTotals);

	//uint64 pageInDiff, pageOutDiff, swapInDiff, swapOutDiff;
	vector <uint64> vmStat = getVMstat(showTotals);

	string loadAvg = getLoadAvg();
	rows.push_back( renderBootandLoadAvg((time_t) stats[2][1], loadAvg) );
	prettyPrint(rows, rowWidth, false);
	rows.clear();
	cout << endl;

	rows = renderCPUandPageStats(perSecond, showTotals, elapsed, CPUcount, (uint64)(uptime * USER_HZ),
		 stats[0], stats[2][0], vmStat);
	prettyPrint(rows, rowWidth, false);
	rows.clear();
	cout << endl;

	vector <struct IRQ> IRQs = getIRQs();

	rows = renderIRQs(perSecond, showTotals, elapsed, IRQs, stats[1]);
	prettyPrint(rows, rowWidth, false);
	cout << endl;

	vector <struct diskStat_t> diskStats = getDiskStats(showTotals);
	rows=renderDiskStats(perSecond, showTotals, showSectors, elapsed, diskStats);
	prettyPrint(rows, rowWidth, false);
	
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
		while((c = getopt(argc, argv, "n:N:fSDdrb")) != -1) {
		
			switch(c) {
				case 'n':
				case 'N':
					interval = strtod(optarg, (char **)NULL);
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
			}
		}
	} else {
		perSecond = true;
		interval = 0;
		fullScreen = false;
	}

	if(fullScreen)
		printf("\e[2J");

	uint32 CPUcount = getCPUcount();
	const struct timeval sleepInterval = { (int)interval, getFrac(interval, 1000000) };
	initConsole();
	oldDiskStats.clear();
	while(1) {
		fd_set fdSet;
		FD_ZERO(&fdSet);
		FD_SET(0, &fdSet);
		struct timeval sleepTime = sleepInterval; // select can modify sleepTime
		mainLoop(perSecond, showTotals, showTotalsMem, fullScreen, showRealMemFree, showSectors, CPUcount);
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
