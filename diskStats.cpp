#include "routines.cpp"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// this might be wrong for optical, but it might not!
#define DEFAULT_SECTSZ 512
#define getSectorSize(x) ((DEFAULT_SECTSZ))

struct diskStat_t {
	bool display;
	uint32_t major, minor;
	string name;
	uint32_t sectorSize;
	vector <uint64_t> stats;
};

vector <struct diskStat_t> getDiskStats(bool showTotals, bool partitionStats) {
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
			if(!partitionStats && diskDiff.name.length() > 3 ) {
				const char *disk = diskDiff.name.c_str();
				if( (disk[0] == 'h' || disk[0] == 's') && (disk[1] == 'd') ) {
					if( isdigit(disk[strlen(disk)-1]) ) {
						diskDiff.display = false;
					}
				}
			}
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

inline string renderDiskBytes(bool perSecond, bool showTotals, bool showSectors, const double elapsed,
	const struct diskStat_t &diskStat) __attribute((always_inline));
string renderDiskBytes(bool perSecond, bool showTotals, bool showSectors, const double elapsed,
	const struct diskStat_t &diskStat)
{
	string output;
	if(showSectors) {
		string readStat =
			humanizeBigNums(
				uint64_t(perSecond ?
					(diskStat.stats[2] * diskStat.sectorSize) / elapsed :
					diskStat.stats[2]
				)
			);
		string writeStat =
			humanizeBigNums(
				uint64_t(perSecond ?
					(diskStat.stats[6] * diskStat.sectorSize) / elapsed :
					diskStat.stats[6]
				)
			);
		char buf[36]; bzero(buf, 36);
		snprintf(buf, 36, "r%15s w%15s", readStat.c_str(), writeStat.c_str());
		output = string(buf);
	} else {
		char buf[36]; bzero(buf, 36); // note callsite expects to align a 34-char string
#if __WORDSIZE == 64
		snprintf(buf, 34, "%15lur %15luw", diskStat.stats[0], diskStat.stats[4]);
#else
		snprintf(buf, 34, "%15llur %15lluw", diskStat.stats[0], diskStat.stats[4]);
#endif
		output = string(buf);
	}
	return output;
}

inline string renderDiskStat(bool perSecond, bool showTotals, bool showSectors,
	const double &elapsed, const struct diskStat_t &diskStat) __attribute((always_inline));
inline string renderDiskStat(bool perSecond, bool showTotals, bool showSectors, const double &elapsed,
	const struct diskStat_t &diskStat)
{
	char output[40]; bzero(output, 40);
	const string rendered = renderDiskBytes(perSecond, showTotals, showSectors, elapsed, diskStat);

	snprintf(output, 39, "%-4s %-34s", diskStat.name.c_str(), rendered.c_str());

	return output;
}

vector< vector <string> > renderDiskStats(bool perSecond, bool showTotals, bool showSectors, double elapsed,
	const vector <struct diskStat_t> &diskStats)
{
	vector< string> entries;
	if(showTotals) {
		elapsed = 1.000;
	}
	for(unsigned int i = 0; i < diskStats.size(); i++) {
		if(diskStats[i].display)
			entries.push_back(renderDiskStat(perSecond, showTotals, showSectors, elapsed, diskStats[i]));
	}
	vector< vector <string> > rows;
	const unsigned int split = entries.size() / 2 + (entries.size() & 1); // is equiv to (entries.size() % 2)
	for(unsigned int i = 0; i < split; i++) {
		vector<string> row;
		row.push_back(entries[i]);
		if(entries.size() > i+split) 
			row.push_back(entries[i+split]);
		rows.push_back(row);
	}
	return rows;
}
