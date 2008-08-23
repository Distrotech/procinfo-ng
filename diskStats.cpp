#include "routines.cpp"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

struct diskStat_t {
	bool display;
	uint32_t major, minor;
	string name;
	uint32_t sectorSize;
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
		
		if( tokens[2].length() > 3 ) {
			const char *disk = tokens[2].c_str();
			if( (disk[0] == 'h' || disk[0] == 's') && (disk[1] == 'd') ) {
				if( isdigit(disk[strlen(disk)-1]) ) {
					offset++;
					continue;
				}
			}
		}

		struct diskStat_t diskStat = {
			false,
			string2uint32(tokens[0]), string2uint32(tokens[1]),
			tokens[2],
			getSectorSize(tokens[2]),
			vector <uint64_t>(11,0)
		};
		struct diskStat_t diskDiff = {
			false,
			string2uint32(tokens[0]), 
			string2uint32(tokens[2]),
			tokens[2],
			getSectorSize(tokens[2]),
			vector <uint64_t>(11,0)
		};
		if(oldDiskStats.size() < i + 1) {
			struct diskStat_t tmpObj = {
				false,
				0, 0,
				tokens[2],
				getSectorSize(tokens[2]),
				vector <uint64_t>(11,0)
			};
			
			oldDiskStats.push_back(tmpObj);
		}
		tokens.erase(tokens.begin(), tokens.begin()+3);
		diskStat.stats = stringVec2uint64Vec(tokens);
		if( (diskStat.stats[0] || diskStat.stats[4]) ||
			( (diskStat.name[0] == 'h' || diskStat.name[0] == 's' ) && diskStat.name[1] == 'd' ) )
		{
			diskDiff.display = true;
		}
		if(!showTotals) {
			diskDiff.stats = subVec(diskStat.stats, oldDiskStats[i-offset].stats);
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
			(showSectors ? diskStats[i].stats[2] : diskStats[i].stats[0]),
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
