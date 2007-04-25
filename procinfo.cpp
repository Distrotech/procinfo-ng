#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define INTERVAL 1
#define USER_HZ 100

using namespace std;

#define zalloc(x) calloc(1, x)
typedef unsigned int uint32;
typedef unsigned long long uint64;

struct timeWDHMS {
	uint32 weeks, days, hours, minutes;
	double seconds;
};

struct timeWDHMS splitTime(uint64 difference) {
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

struct timeWDHMS splitTime(double difference) {
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

struct timeWDHMS splitTime(uint32 difference) {
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

vector<uint32> getMaxWidths(vector<vector <string> > rows) {
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

void prettyPrint(vector <vector <string> > rows, vector<uint32> *colWidthsPtr, bool leftJustify) {
	vector <uint32> colWidths;
	if(colWidthsPtr == NULL) {
		colWidths = getMaxWidths(rows);
	} else {
		colWidths = *colWidthsPtr;
	}

	for(uint32 i = 0; i < rows.size(); i++) {
		string line;
		for(uint32 j = 0; j < rows[i].size(); j++) {
			char *fmt = (char *)zalloc(10);
			if(!leftJustify) {
				sprintf(fmt, "%%%s%ds", (!j ? "-" : ""), colWidths[j] + 1);
			} else {
				sprintf(fmt, "%%-%ds", colWidths[j] + 1);
			}
			char *subline = (char *)zalloc(100);
			sprintf(subline, fmt, rows[i][j].c_str());
			line = line + subline + " ";
		}
		cout << line << endl;
		//printf("%s\n", line.c_str());
	}
}

vector <string> splitString(string delim, string str) {
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

string uint64toString(uint64 num) {
	char str[20+1];
	snprintf(str, 20, "%llu", (unsigned long long int)num);
	return string(str);
}

uint64 string2uint64(string &str) {
	return strtoull(str.c_str(), (char **)NULL, 10);
}

vector <vector <string> > getMeminfo() {
	vector <string> lines = readFile(string("/proc/meminfo"));

	// these have identical names to the keys in meminfo
	uint64 MemTotal = 0, MemFree = 0, Buffers = 0, SwapTotal = 0, SwapFree = 0;

	for(uint32 i = 0; i < lines.size(); i++) {
		vector <string> tokens = splitString(" ", lines[i]);
		if (!tokens.size()) break;
		if(tokens[0] == "MemTotal:") {
			MemTotal = string2uint64(tokens[1]);
		} else if(tokens[0] == "MemFree:") {
			MemFree = string2uint64(tokens[1]);
		} else if(tokens[0] == "Buffers:") {
			Buffers = string2uint64(tokens[1]);
		} else if(tokens[0] == "SwapTotal:") {
			SwapTotal = string2uint64(tokens[1]);
		} else if(tokens[0] == "SwapFree:") {
			SwapFree = string2uint64(tokens[1]);
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

	row = new vector<string>;
	row->push_back("RAM:");
	row->push_back(uint64toString(MemTotal));
	row->push_back(uint64toString(MemTotal - MemFree));
	row->push_back(uint64toString(MemFree));
	row->push_back(uint64toString(Buffers));
	rows.push_back(*row);

	row = new vector<string>;
	row->push_back("Swap:");
	row->push_back(uint64toString(SwapTotal));
	row->push_back(uint64toString(SwapTotal - SwapFree));
	row->push_back(uint64toString(SwapFree));
	rows.push_back(*row);

	return rows;
}

vector <uint64> stringVec2uint64Vec(vector <string> stringVec) {
	vector <uint64> uint64Vec;
	for(uint32 i = 0; i < stringVec.size(); i++)
		uint64Vec.push_back(string2uint64(stringVec[i]));
	return uint64Vec;
}

vector <uint64> subUint64Vec(vector <uint64> vec1, vector <uint64> vec2) {
	vector <uint64> vec3; vec3.resize(vec2.size());
	for(uint32 i = 0; i < vec2.size(); i++)
		vec3[i] = vec1[i] - vec2[i];
	return vec3;
}

vector <uint64> oldCPUstat, oldIntrStat;
uint64 oldCtxtStat = 0;
vector <vector <uint64> > getProcStat() {
	vector <string> lines = readFile(string("/proc/stat"));
	vector <uint64> cpuDiff, cpuStat, intrDiff, intrStat;
	uint64 ctxtStat, ctxtDiff;

	for(uint32 i = 0; i < lines.size(); i++) {
		vector <string> tokens = splitString(" ", lines[i]);
		if (!tokens.size()) break; // a) prevents SIGSEGV b) skips empty lines
		if(tokens[0] == "cpu") {
			tokens.erase(tokens.begin()); // pop the first token off.

			cpuStat = stringVec2uint64Vec(tokens);
			if(!oldCPUstat.size())
				oldCPUstat.resize(cpuStat.size());
			cpuDiff = subUint64Vec(cpuStat, oldCPUstat);
			oldCPUstat.assign(cpuStat.begin(), cpuStat.end());
		} else if(tokens[0] == "intr") {
			tokens.erase(tokens.begin()); // pop the first token off.
			tokens.erase(tokens.begin()); // pop the second token off.

			intrStat = stringVec2uint64Vec(tokens);
			if(!oldIntrStat.size())
				oldIntrStat.resize(intrStat.size());
			intrDiff = subUint64Vec(intrStat, oldIntrStat);
			oldIntrStat.assign(intrStat.begin(), intrStat.end());
		} else if(tokens[0] == "ctxt") {
			tokens.erase(tokens.begin()); // pop the first token off.

			ctxtStat = string2uint64(tokens[0]);
			ctxtDiff = ctxtStat - oldCtxtStat;
			oldCtxtStat = ctxtStat;
		}
	}
	vector <vector <uint64> > stats;
	stats.resize(3);
	stats[0] = cpuDiff;
	stats[1] = intrDiff;
	stats[2].push_back(ctxtDiff);
	return stats;
}

uint64 oldPageIn = 0, oldPageOut = 0, oldSwapIn = 0, oldSwapOut = 0;
vector <uint64> getVMstat() {
	vector <string> lines = readFile(string("/proc/vmstat"));

	uint64 pageIn = 0, pageOut = 0, swapIn = 0, swapOut = 0;
	uint64 pageInDiff = 0, pageOutDiff = 0, swapInDiff = 0, swapOutDiff = 0;

	for(uint32 i = 0; i < lines.size(); i++) {
		vector <string> tokens = splitString(" ", lines[i]);
		if (!tokens.size()) break;
		if(tokens[0] == "pgpgin") {
			pageIn = string2uint64(tokens[1]);
			pageInDiff = pageIn - oldPageIn;
			oldPageIn = pageIn;
		} else if(tokens[0] == "pgpgout") {
			pageOut = string2uint64(tokens[1]);
			pageOutDiff = pageOut - oldPageOut;
			oldPageOut = pageOut;
		} else if(tokens[0] == "pswpin") {
			swapIn = string2uint64(tokens[1]);
			swapInDiff = swapIn - oldSwapIn;
			oldSwapIn = swapIn;
		} else if(tokens[0] == "pswpout") {
			swapOut = string2uint64(tokens[1]);
			swapOutDiff = swapOut - oldSwapOut;
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

vector <string> renderCPUstat(double elapsed, uint64 cpuDiff, string name) {

	struct timeWDHMS timeDiff = splitTime(cpuDiff / (double)USER_HZ);
	char *buf = new char[64]; bzero(buf, 63);
	string output;
	if(timeDiff.weeks) {
		sprintf(buf, "%dw ", timeDiff.weeks);
		output += buf;
	}
	if(timeDiff.days) {
		sprintf(buf, "%dd ", timeDiff.days);
		output += buf;
	}
	sprintf(buf, "%02d:%02d:%02.2f", timeDiff.hours, timeDiff.minutes, timeDiff.seconds);
	output += buf;
	char *percentBuf = new char[64]; bzero(percentBuf, 63); bzero(buf, 63);
	sprintf(percentBuf, "%3.1f", (double)cpuDiff / elapsed);
	sprintf(buf, " %5s%%", percentBuf);
	output = output + buf;
	delete percentBuf; delete buf;

	vector<string> row;
	row.push_back(name); row.push_back(output);

	return row;
}

vector <string> renderPageStat(double elapsed, uint64 pageDiff, string name) {
	char *buf = new char[64]; bzero(buf, 63);
	sprintf(buf, "%llu", uint64(pageDiff / elapsed));
	
	vector<string> row;
	row.push_back(name); row.push_back(string(buf));
	delete buf;

	return row;
}

vector< vector <string> > renderCPUandPageStats(double elapsed, vector <uint64> cpuDiffs, uint64 ctxtDiff, vector <uint64> pageDiffs) {

	vector< vector <string> > rows;
	vector<string> row;
	vector <string> names;
	names.push_back(string("user  :")); names.push_back(string("page in :"));
	names.push_back(string("nice  :")); names.push_back(string("page out:"));
	names.push_back(string("system:")); names.push_back(string("swap in :"));
	names.push_back(string("idle  :")); names.push_back(string("swap out:"));
	names.push_back(string("uptime:")); names.push_back(string("context :"));
	for(uint32 i = 0; i <= 4; i++) {
		vector<string> cols = renderCPUstat(elapsed, cpuDiffs[i], names[i*2]);
		row.push_back(cols[0]); row.push_back(cols[1]);

		cols = renderPageStat(elapsed, ( i == 4 ? ctxtDiff : pageDiffs[i]), names[i*2+1]);
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

time_t getBootTime(double uptime) {
	return time(NULL)-(time_t)uptime;
}

string getLoadAvg() {
	vector <string> lines = readFile(string("/proc/loadavg"));
	return lines[0];
}

vector <string> renderBootandLoadAvg(double uptime, string loadAvg) {
	vector <string> row;
	
	time_t bootTime;
	bootTime = getBootTime(uptime);
	string bootTimeStr = string(ctime(&bootTime));
	// remove the "\n". don't ask me why ctime does that...
	bootTimeStr.erase(bootTimeStr.end()-1);
	row.push_back(string(string("Bootup: ") + bootTimeStr));
	row.push_back(string("Load average: " + loadAvg));
	return row;
}

string renderIRQ(double elapsed, struct IRQ irq, uint64 intrDiff) {
	char buf[64]; bzero(buf, 63);
	string output;

	sprintf(buf, "irq %3d:", irq.IRQnum); 
	output += buf; bzero(buf, 63);
	char countBuf[64]; bzero(countBuf, 63);
	sprintf(countBuf, "%llu", uint64(intrDiff / elapsed));
	sprintf(buf, "%9s %-20s", countBuf, irq.devs.substr(0, 20).c_str());
	output = output + " " + buf; bzero(countBuf, 63); bzero(buf, 63);

	return output;
}

vector< vector <string> > renderIRQs(double elapsed, vector <struct IRQ> IRQs, vector <uint64> intrDiffs) {
	vector<vector <string> > rows;
	uint32 split = IRQs.size() / 2;
	for(uint32 i = 0; i < split; i++) {
		vector <string> row;
		row.push_back( renderIRQ(elapsed, IRQs[i], intrDiffs[IRQs[i].IRQnum]) );
		if(i+split < IRQs.size())
			row.push_back( renderIRQ(elapsed, IRQs[i+split], intrDiffs[IRQs[i+split].IRQnum]) );
		rows.push_back(row);
		
	}
	return rows;
}

int mainLoop();

int main() {
	printf("\e[2J");
	mainLoop();
	sleep(INTERVAL);
	printf("\e[2J");
	mainLoop();
	sleep(INTERVAL);
	mainLoop();
	sleep(INTERVAL);
	mainLoop();
	sleep(INTERVAL);
	mainLoop();
	return 0;	
}

double oldUptime = 0;
int mainLoop() {
	vector<vector <string> > rows;

	double uptime = getUptime();
	double elapsed = ( oldUptime ? uptime - oldUptime : INTERVAL );
	printf("\e[H");
	rows = getMeminfo();
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
	vector <uint64> ctxtDiff = stats[2]; // only contains one entry.
*/
	vector <vector <uint64> > stats = getProcStat();

	//uint64 pageInDiff, pageOutDiff, swapInDiff, swapOutDiff;
	vector <uint64> vmStat = getVMstat();

	string loadAvg = getLoadAvg();
	rows.push_back( renderBootandLoadAvg(uptime, loadAvg) );
	prettyPrint(rows, rowWidth, false);
	rows.clear();
	cout << endl;

	rows = renderCPUandPageStats(elapsed, stats[0], stats[2][0], vmStat);
	prettyPrint(rows, rowWidth, false);
	rows.clear();
	cout << endl;


	vector <struct IRQ> IRQs = getIRQs();

	rows = renderIRQs(elapsed, IRQs, stats[1]);
	prettyPrint(rows, rowWidth, false);
	cout << endl;
	
	oldUptime = uptime;
	return 0;
}
