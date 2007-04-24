#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>

using namespace std;

#define zalloc(x) calloc(1, x)
typedef unsigned int uint32;
typedef unsigned long long uint64;

struct timeWDHMS {
	uint32 weeks, days, hours, minutes, seconds;
};

struct timeWDHMS splitTime(uint64 difference) {
	struct timeWDHMS time;
	time.seconds = (int)(difference % 60);
	difference = (difference - time.seconds) / 60;
	time.minutes = (int)(difference % 60);
	difference = (difference - time.minutes) / 60;
	time.hours = (int)(difference % 24);
	difference = (difference - time.hours) / 24;
	time.days = (int)(difference % 24);
	time.weeks = (int)((difference - time.days) / 7);

	return time;
}

struct timeWDHMS splitTime(uint32 difference) {
	struct timeWDHMS time;
	time.seconds = (int)(difference % 60);
	difference = (difference - time.seconds) / 60;
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
	return strtoull(str.c_str(), (char **)NULL, 0);
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
			MemFree = strtoull(tokens[1].c_str(), (char **)NULL, 0);
		} else if(tokens[0] == "Buffers:") {
			Buffers = strtoull(tokens[1].c_str(), (char **)NULL, 0);
		} else if(tokens[0] == "SwapTotal:") {
			SwapTotal = strtoull(tokens[1].c_str(), (char **)NULL, 0);
		} else if(tokens[0] == "SwapFree:") {
			SwapFree = strtoull(tokens[1].c_str(), (char **)NULL, 0);
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
uint64 oldCtxtStat;
vector <vector <string> > getProcStat() {
	vector <string> lines = readFile(string("/proc/stat"));
	vector <uint64> cpuDiff, cpuStat, intrDiff, intrStat;

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

			intrStat = stringVec2uint64Vec(tokens);
			if(!oldIntrStat.size())
				oldIntrStat.resize(intrStat.size());
			intrDiff = subUint64Vec(intrStat, oldIntrStat);
			oldIntrStat.assign(intrStat.begin(), intrStat.end());
		}
	}
	vector <vector <string> > rows;
	return rows;
}



int main(int argc, char *argv[]) {
	vector<vector <string> > rows;
/*
	const int numRows = 3, numCols = 2; 
	rows.resize(numRows); 
	for(int i = 0; i < numRows; i++) {
		rows[i].resize(numCols);
	}
	rows[0][0] = "I am the very";
	rows[1][0] = "model of a modern";
	rows[2][0] = "major general.";

	rows[0][1] = "I've information";
	rows[1][1] = "vegetable, animal";
	rows[2][1] = "and mineral.";
	prettyPrint(rows, NULL, false);
*/
	rows = getMeminfo();
	vector <uint32> *rowWidth = new vector <uint32>;
	rowWidth->push_back(6);
	rowWidth->push_back(10);
	rowWidth->push_back(10);
	rowWidth->push_back(10);
	rowWidth->push_back(10);
	prettyPrint(rows, rowWidth, false);
	delete rowWidth; rowWidth = NULL;

	getProcStat();
}
