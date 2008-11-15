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

// Procinfo-NG is Copyright tabris@tabris.net 2007, 2008

#include <dirent.h>
#include <sys/stat.h>

#include <map>
#include <vector>
#include <string>
#include <algorithm>

const static string pathSysFs = "/sys/class/net/";

static inline bool dentryIsDir(const struct dirent64 *dentry) {
	if(dentry->d_type == DT_DIR) {
		return true;
	} else if(dentry->d_type == DT_LNK) {
		struct stat buf;
		if(stat(dentry->d_name, &buf) != 0) {
			return false;
		}
		if((buf.st_mode & S_IFMT) == S_IFDIR) {
			return true;
		}
	}
	return false;
}

vector <string> findInterfaces(void) {
	vector <string> result;
	DIR *dirHandle = opendir("/sys/class/net/");
	struct dirent64 *dentry;
	const static string thisDir("."), parentDir("..");
	struct stat buf;

	while((dentry = readdir64(dirHandle)) != NULL) {
		if(dentryIsDir(dentry)) {
			// if a directory
			if(dentry->d_name == thisDir || dentry->d_name == parentDir) {
				continue;
			}
		} else {
			// if not a directory
			continue;
			// This double if-block could be just one,
			// but I think this is more readable.
		}
		string path = pathSysFs + (dentry->d_name) + ("/statistics/rx_bytes");
		if( stat(path.c_str(), &buf) == 0 ) {
			// if stat(...) == 0, that is success
			// (all we care about is that it's not -ENOENT or some other error)
			result.push_back(dentry->d_name);
		}
		
	}
	closedir(dirHandle);

	sort(result.begin(), result.end());
	return result;
}

struct __netStat {
	char iface[32];
	uint64_t rx_bytes, tx_bytes;
	uint64_t rx_packets, tx_packets;
};

struct __netStat getIfaceStats(const string interface) {
	struct __netStat ifStat;
	bzero(&ifStat, sizeof(ifStat));
	vector <string> lines;

	lines = readFile(pathSysFs+interface+"/statistics/rx_bytes");
	if(lines.size()) {
		ifStat.rx_bytes = string2uint64(lines[0]);
	}

	lines = readFile(pathSysFs+interface+"/statistics/tx_bytes");
	if(lines.size()) {
		ifStat.tx_bytes = string2uint64(lines[0]);
	}

	lines = readFile(pathSysFs+interface+"/statistics/rx_packets");
	if(lines.size()) {
		ifStat.rx_packets = string2uint64(lines[0]);
	}

	lines = readFile(pathSysFs+interface+"/statistics/tx_packets");
	if(lines.size()) {
		ifStat.tx_packets = string2uint64(lines[0]);
	}

	return ifStat;
}

struct ltstr
{
  bool operator()(string s1, string s2) const
  {
    return (s1 < s2);
  }
};

const static inline uint64_t getRXdiff(const struct __netStat &newStat,
	const struct __netStat &oldStat)
{
	return newStat.rx_bytes - oldStat.rx_bytes;
}
const static inline uint64_t getTXdiff(const struct __netStat &newStat,
	const struct __netStat &oldStat)
{
	return newStat.tx_bytes - oldStat.tx_bytes;
}

vector <vector <string> > getNetStats(bool perSecond, bool showTotals, const double interval) {
	static map<string, struct __netStat, ltstr> oldInterfaceStats;
	static map<string, struct __netStat, ltstr> interfaceStats;
	
	vector <string> interfaces = findInterfaces();
	vector <vector <string > > entries; entries.reserve(interfaces.size());
	for(unsigned int i = 0; i < interfaces.size(); i++) {
		string iface = interfaces[i];

		interfaceStats[iface] = getIfaceStats(iface);
		const bool newIF = ( oldInterfaceStats.find(iface) == oldInterfaceStats.end() );
		struct __netStat ifaceStats;
		const struct __netStat ifaceStat = interfaceStats[iface];
		const struct __netStat oldIfaceStat = oldInterfaceStats[iface];
		if( perSecond && !showTotals && !newIF ) {
			ifaceStats.rx_bytes = uint64_t(getRXdiff(ifaceStat, oldIfaceStat) / interval);
			ifaceStats.tx_bytes = uint64_t(getTXdiff(ifaceStat, oldIfaceStat) / interval);
		} else if( !perSecond && !showTotals && !newIF ) {
			ifaceStats.rx_bytes = getRXdiff(ifaceStat, oldIfaceStat);
			ifaceStats.tx_bytes = getTXdiff(ifaceStat, oldIfaceStat);
		} else {
			ifaceStats.rx_bytes = ifaceStat.rx_bytes;
			ifaceStats.tx_bytes = ifaceStat.tx_bytes;
		}

		vector <string> row(3);
		row[0] = iface;
		row[1] = "TX " + humanizeBigNums(ifaceStats.tx_bytes);
		row[2] = "RX " + humanizeBigNums(ifaceStats.rx_bytes);
		entries.push_back(row);
	}
	oldInterfaceStats = interfaceStats;
	interfaceStats.clear();

	const unsigned int split = entries.size() / 2 + (entries.size() & 1); // is equiv to (entries.size() % 2)
	vector <vector <string > > rows; rows.reserve(split);
	for(unsigned int i = 0; i < split; i++) {
		vector <string> row(entries[i]); // note initial initializing, saves time/space
		if(entries.size() > i+split)
			//row.insert(row.end, entries[i+split].begin(), entries[i+split].end());
			try {
				// foo.at() throws an exception instead of segfaulting.
				row.push_back(entries.at(i+split)[0]);
				row.push_back(entries.at(i+split)[1]);
				row.push_back(entries.at(i+split)[2]);
			} catch(...) {
				// We don't do anything.
			}
		rows.push_back(row);
	}
	return rows;
}
