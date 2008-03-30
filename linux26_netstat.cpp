#include <dirent.h>
#include <sys/stat.h>

vector <string> findInterfaces(void) {
	vector <string> result;
	DIR *dirHandle = opendir("/sys/class/net/");
	struct dirent64 *dentry;
	const string thisDir = string("."), parentDir = string("..");
	struct stat buf;
	while((dentry = readdir64(dirHandle)) != NULL) {
		if(dentry->d_name == thisDir || dentry->d_name == parentDir ) {
			continue;
		}
		string path = string("/sys/class/net/") + string(dentry->d_name) + string ("/statistics/rx_bytes");
		if( stat(path.c_str(), &buf) == 0 ) {
			result.push_back(string(dentry->d_name));
		}
		
	}
	return result;
}

#include <map>

struct __netStat {
	char iface[32];
	uint64_t rx_bytes, tx_bytes;
	uint64_t rx_packets, tx_packets;
};

struct __netStat getIfaceStats(string interface) {
	struct __netStat ifStat;
	bzero(&ifStat, sizeof(ifStat));
	vector <string> lines;

	lines = readFile(string("/sys/class/net/")+interface+"/statistics/rx_bytes");
	if(lines.size()) {
		ifStat.rx_bytes = string2uint64(lines[0]);
	}

	lines = readFile(string("/sys/class/net/")+interface+"/statistics/tx_bytes");
	if(lines.size()) {
		ifStat.tx_bytes = string2uint64(lines[0]);
	}

	lines = readFile(string("/sys/class/net/")+interface+"/statistics/rx_packets");
	if(lines.size()) {
		ifStat.rx_packets = string2uint64(lines[0]);
	}

	lines = readFile(string("/sys/class/net/")+interface+"/statistics/tx_packets");
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

vector <vector <string> > getNetStats(bool perSecond, bool showTotals, double interval) {
	static map<string, struct __netStat, ltstr> oldInterfaceStats;
	static map<string, struct __netStat, ltstr> interfaceStats;
	
	vector <string> interfaces = findInterfaces();
	vector <vector <string > > rows; rows.reserve(interfaces.size());
	for(unsigned int i = 0; i < interfaces.size(); i++) {
		string iface = interfaces[i];

		interfaceStats[iface] = getIfaceStats(iface);
		bool newIF = ( oldInterfaceStats.find(iface) == oldInterfaceStats.end() );
		struct __netStat ifaceStats;
		if( perSecond && !showTotals && !newIF ) {
			ifaceStats.rx_bytes = uint64_t((interfaceStats[iface].rx_bytes - oldInterfaceStats[iface].rx_bytes) / interval);
			ifaceStats.tx_bytes = uint64_t((interfaceStats[iface].tx_bytes - oldInterfaceStats[iface].tx_bytes) / interval);
		} else if( !perSecond && !showTotals && !newIF ) {
			ifaceStats.rx_bytes = interfaceStats[iface].rx_bytes - oldInterfaceStats[iface].rx_bytes;
			ifaceStats.tx_bytes = interfaceStats[iface].tx_bytes - oldInterfaceStats[iface].tx_bytes;
		} else {
			ifaceStats.rx_bytes = interfaceStats[iface].rx_bytes;
			ifaceStats.tx_bytes = interfaceStats[iface].tx_bytes;
		}

		vector <string> row(3);
		row[0] = iface;
		row[1] = "TX " + humanizeBigNums(ifaceStats.tx_bytes);
		row[2] = "RX " + humanizeBigNums(ifaceStats.rx_bytes);
		rows.push_back(row);
	}
	oldInterfaceStats = interfaceStats;
	interfaceStats.clear();
	return rows;
}
