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

using namespace std;

#include "routines.cpp"

#define DEFAULT_INTERVAL 5
#define USER_HZ sysconf(_SC_CLK_TCK)
#define VERSION "2.0"
#define REVISION "$Rev$"

// bzero is deprecated, but I like it enough to just alias it
#define bzero(ptr,len) memset(ptr, 0, len)

// inlined b/c it only has ONE caller.
// returns a list of uint32_t column widths.
inline vector<uint32_t> getMaxWidths(const vector<vector <string> > &rows) {
	vector<uint32_t> colWidths;

	for(uint32_t i = 0; i < rows.size(); i++)
		for(uint32_t j = 0; j < rows[i].size(); j++) {
			if(colWidths.size() < j+1)
				colWidths.resize(j+1);
			if(colWidths[j] < rows[i][j].length())
				colWidths[j] = rows[i][j].length();
		}

	return colWidths;
}

// accepts a list of rows containing columns, an optional static list of column-widths and leftJustify
// returns nothing
void prettyPrint(const vector <vector <string> > &rows, vector<uint32_t> *colWidthsPtr, bool leftJustify) {
	vector <uint32_t> colWidths;
	static const string spaces =
		"                                                                                ";

	if(colWidthsPtr == NULL) {
		colWidths = getMaxWidths(rows);
	} else {
		colWidths = *colWidthsPtr;
	}

	for(uint32_t i = 0; i < rows.size(); i++) {
		string line;
		for(uint32_t j = 0; j < rows[i].size(); j++) {
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
// For clarification, 'fail' means 'loop infinitely'
vector <string> readFile(const string &fileName) {
	vector <string> lines;
	ifstream file(fileName.c_str());

	for(uint32_t i = 0; !file.eof(); i++) {
		char str[4096];
		file.getline(str, 4094);
		lines.push_back(string(str));
	}
	return lines;
}

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
	row->push_back(int64toString(int64_t(MemTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row->push_back(int64toString(int64_t((MemTotalDiff - MemFreeDiff) / 
		(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row->push_back(int64toString(int64_t(MemFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row->push_back(int64toString(int64_t(BuffersDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	rows.push_back(*row);
	delete row;

	if(showRealMemFree) { // Produces free memory figures that consider Buffers + Cache as disposable.
		int64_t BuffCacheUsed = int64_t(((MemTotalDiff - MemFreeDiff) - (BuffersDiff + CacheDiff)) / 
			(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)));
		int64_t BuffCacheFree = int64_t((MemFreeDiff + (BuffersDiff + CacheDiff)) / (
			!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)));
		row = new vector<string>;
		row->push_back("-/+ buffers/cache  ");
		row->push_back(int64toString(BuffCacheUsed));
		row->push_back(int64toString(BuffCacheFree));
		rows.push_back(*row);
		delete row;
	}

	row = new vector<string>;
	row->push_back("Swap:");
	row->push_back(int64toString(int64_t(SwapTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row->push_back(int64toString(int64_t((SwapTotalDiff - SwapFreeDiff) / 
		(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	row->push_back(int64toString(int64_t(SwapFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	rows.push_back(*row);
	delete row;

	return rows;
}

// returns multiple lists of uint64s, cpuDiffs, intrDiffs, and a list consisting of context-switches and the boot-time
vector <vector <uint64_t> > getProcStat(bool showTotals) {
	vector <string> lines = readFile(string("/proc/stat"));
	vector <uint64_t> cpuDiff, cpuStat, intrDiff, intrStat;

	static vector <uint64_t> oldCPUstat, oldIntrStat;
	static uint64_t oldCtxtStat = 0;

	uint64_t ctxtStat, ctxtDiff, bootTime;
	uint64_t cpuTotal = 0;

	for(uint32_t i = 0; i < lines.size(); i++) {
		vector <string> tokens = splitString(" ", lines[i]);
		if (!tokens.size()) break; // a) prevents SIGSEGV b) skips empty lines
		if(tokens[0] == "cpu") {
			tokens.erase(tokens.begin()); // pop the first token off.

			cpuStat = stringVec2uint64Vec(tokens);
			if(!oldCPUstat.size())
				oldCPUstat.resize(cpuStat.size());
			cpuDiff = (showTotals ? cpuStat : subUint64Vec(cpuStat, oldCPUstat));
			for(uint32_t i = 0; i < cpuStat.size(); i++)
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
	vector <vector <uint64_t> > stats;
	stats.resize(3);
	stats[0] = cpuDiff;
	stats[1] = intrDiff;
	stats[2].push_back(ctxtDiff);
	stats[2].push_back(bootTime);
	return stats;
}

// returns the contents of /proc/vmstat, only the parts we want.
// as such it returns a vector of 4 elements, pageInDiff, pageOutDiff, swapInDiff, swapOutDiff
vector <uint64_t> getVMstat(bool showTotals) {
	vector <string> lines = readFile(string("/proc/vmstat"));

	static uint64_t oldPageIn = 0, oldPageOut = 0, oldSwapIn = 0, oldSwapOut = 0;
	uint64_t pageIn = 0, pageOut = 0, swapIn = 0, swapOut = 0;
	uint64_t pageInDiff = 0, pageOutDiff = 0, swapInDiff = 0, swapOutDiff = 0;

	static uint64_t oldPageAct = 0, oldPageDeact = 0, oldPageFault = 0;
	uint64_t pageAct = 0, pageDeact = 0, pageFault = 0;
	uint64_t pageActDiff = 0, pageDeactDiff = 0, pageFaultDiff = 0;

	for(uint32_t i = 0; i < lines.size(); i++) {
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
		} else if(tokens[0] == "pgactivate") {
			pageAct = string2uint64(tokens[1]);
			pageActDiff = (showTotals ? pageAct : pageAct - oldPageAct);
			oldPageAct = pageAct;
		} else if(tokens[0] == "pgdeactivate") {
			pageDeact = string2uint64(tokens[1]);
			pageDeactDiff = (showTotals ? pageDeact : pageDeact - oldPageDeact);
			oldPageDeact = pageDeact;
		} else if(tokens[0] == "pgfault") {
			pageFault = string2uint64(tokens[1]);
			pageFaultDiff = (showTotals ? pageFault : pageFault - oldPageFault);
			oldPageFault = pageFault;
		}
	}
	vector <uint64_t> vmStat;
	vmStat.push_back(pageInDiff);
	vmStat.push_back(pageOutDiff);
	vmStat.push_back(pageActDiff);
	vmStat.push_back(pageDeactDiff);
	vmStat.push_back(pageFaultDiff);
	vmStat.push_back(swapInDiff);
	vmStat.push_back(swapOutDiff);
	return vmStat;
}

// accepts multiple CPU statistics for rendering
// returns a single row.
/*inline vector <string> renderCPUstat(bool perSecond, bool showTotals, const double &elapsed,
	const uint32_t &CPUcount, const uint64_t &cpuTotal, const uint64_t &cpuDiff, const string &name)*/
inline vector <string> renderCPUstat(bool perSecond, bool showTotals, const double elapsed,
	const uint32_t CPUcount, const uint64_t cpuTotal, const uint64_t cpuDiff, const string name)
{

	struct timeWDHMS timeDiff = splitTime(cpuDiff / 
		( (double)USER_HZ * ( name == "uptime:" ? 1 :
		(!perSecond || elapsed == 0 || showTotals ? 1 : elapsed)) )
	);
	char *buf = new char[64]; bzero(buf, 64);
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
		char *percentBuf = new char[64]; bzero(percentBuf, 64); bzero(buf, 64);
		snprintf(percentBuf, 63, "%3.1f", 
			(double)cpuDiff / ( (showTotals || elapsed == 0 ? cpuTotal / USER_HZ : 
			(elapsed == 0 ? 1 : elapsed) * CPUcount))
		);
		snprintf(buf, 63, " %5s%%", percentBuf);
		output += buf;
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
inline vector <string> renderPageStat(bool perSecond, bool showTotals, double elapsed, const uint64_t &pageDiff, const string &name) {
	char *buf = new char[64]; bzero(buf, 64);
	snprintf(buf, 63, "%15llu", 
		uint64_t(pageDiff / (perSecond && !showTotals ? 
		( elapsed == 0 ? 1 : elapsed) : 1)));
	
	vector<string> row;
	row.push_back(name); row.push_back(string(buf));
	delete buf;

	return row;
}

// uses renderPageStat and renderCPUstats to render both CPU and page stats
// returns a list of rows containing 2 columns.
/*vector< vector <string> > renderCPUandPageStats(bool perSecond, bool showTotals, const double &elapsed,
	const uint64_t &CPUcount, const uint64_t &uptime, const vector <uint64_t> &cpuDiffs, const uint64_t &ctxtDiff, const vector <uint64_t> &pageDiffs)
*/
vector< vector <string> > renderCPUandPageStats(bool perSecond, bool showTotals, const double elapsed,
	const uint64_t CPUcount, const uint64_t uptime, const vector <uint64_t> cpuDiffs, const uint64_t ctxtDiff, const vector <uint64_t> pageDiffs)
{
	vector< vector <string> > rows;
	vector<string> row;
	vector <string> names;
	
	names.push_back(string("user  :")); names.push_back(string("page in :"));
	names.push_back(string("nice  :")); names.push_back(string("page out:"));
	names.push_back(string("system:")); names.push_back(string("page act:")); 
	names.push_back(string("IOwait:")); names.push_back(string("page dea:")); 
	names.push_back(string("hw irq:")); names.push_back(string("page flt:")); 
	names.push_back(string("sw irq:")); names.push_back(string("swap in :"));
	names.push_back(string("idle  :")); names.push_back(string("swap out:"));
	names.push_back(string("uptime:")); names.push_back(string("context :"));

	for(uint32_t i = 0; i < 8; i++) {
		uint64_t val = 0;
		/* 
		 * This abomination is b/c idle is shown near last
		 * but it's 3rd in line in /proc/stat
		 */
		if(i == 7) {
			val = uptime;
		} else if(i == 6) {
			val = cpuDiffs[3];
		} else if(i > 2) {
			val = cpuDiffs[i+1];
		}
		vector<string> cols = renderCPUstat(perSecond, showTotals, elapsed, CPUcount, cpuDiffs[8], 
			val, names[i*2]);
		row.push_back(cols[0]); row.push_back(cols[1]);

		cols = renderPageStat(perSecond, showTotals, elapsed,
			( i == 7 ? ctxtDiff : pageDiffs[i]), names[i*2+1]);
		row.push_back(cols[0]); row.push_back(cols[1]);

		rows.push_back(row); row.clear();
	}

	return rows;
}

struct IRQ {
	uint16_t IRQnum;
	string devs;
};

vector <struct IRQ> getIRQs() {
	vector <string> lines = readFile(string("/proc/interrupts"));
	
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

double getUptime() {
	vector <string> lines = readFile(string("/proc/uptime"));
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
	snprintf(countBuf, 63, "%llu", uint64_t(intrDiff / (perSecond && !showTotals ? ( elapsed ? elapsed : 1) : 1)));
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
		if (tokens.size() && tokens[0] == "processor\t:") { // x86/x86_64
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
		char *output = new char[40];
		snprintf(output, 39, "%-4s %15llur %15lluw", diskStats[i].name.c_str(), 
			(showSectors ? diskStats[i].stats[2]: diskStats[i].stats[0]),
			(showSectors ? diskStats[i].stats[6] : diskStats[i].stats[4]));
		entries.push_back(output);
		delete output;
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

static termios oldTerm;
inline void initConsole() {
	static const uint32_t STDIN = 0;
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
	const uint32_t &CPUcount, const vector <struct IRQ> &IRQs)
{
	static double oldUptime = 0;

	vector<vector <string> > rows;

	double uptime = getUptime();
	double elapsed = ( oldUptime != 0 ? uptime - oldUptime : 0 );
	if(fullScreen)
		printf("\e[H");
	rows = getMeminfo(perSecond, showTotalsMem, showRealMemFree, elapsed);
	vector <uint32_t> *rowWidth = new vector <uint32_t>;
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
	vector <uint64_t> cpuDiff = stats[0];
	vector <uint64_t> intrDiff = stats[1];
	uint64_t ctxtDiff = stats[2][0];
	uint64_t bootTime = stats[2][1];
*/
	vector <vector <uint64_t> > stats = getProcStat(showTotals);

	//uint64_t pageInDiff, pageOutDiff, swapInDiff, swapOutDiff;
	vector <uint64_t> vmStat = getVMstat(showTotals);

	string loadAvg = getLoadAvg();
	rows.push_back( renderBootandLoadAvg((time_t) stats[2][1], loadAvg) );
	prettyPrint(rows, rowWidth, false);
	rows.clear();
	cout << endl;

	rows = renderCPUandPageStats(perSecond, showTotals, elapsed, CPUcount, (uint64_t)(uptime * USER_HZ),
		 stats[0], stats[2][0], vmStat);
	prettyPrint(rows, rowWidth, false);
	rows.clear();
	cout << endl;


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
	const vector <struct IRQ> IRQs = getIRQs();
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
