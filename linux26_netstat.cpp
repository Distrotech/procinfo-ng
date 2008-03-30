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
	vector <string> lines;

	lines = readFile(string("/sys/class/net/")+interface+"/statistics/rx_bytes");
	ifStat.rx_bytes = string2uint64(lines[0]);

	lines = readFile(string("/sys/class/net/")+interface+"/statistics/tx_bytes");
	ifStat.tx_bytes = string2uint64(lines[0]);

	lines = readFile(string("/sys/class/net/")+interface+"/statistics/rx_packets");
	ifStat.rx_packets = string2uint64(lines[0]);

	lines = readFile(string("/sys/class/net/")+interface+"/statistics/tx_packets");
	ifStat.tx_packets = string2uint64(lines[0]);

	return ifStat;
}

struct ltstr
{
  bool operator()(string s1, string s2) const
  {
    return strcmp(s1.c_str(), s2.c_str()) < 0;
  }
};

vector <vector <string> > getNetStats(bool perSecond, bool showTotals, double interval) {
	static map<string, struct __netStat, ltstr> oldInterfaceStats;
	static map<string, struct __netStat, ltstr> interfaceStats;
	vector <vector <string > > rows;
	
	vector <string> interfaces = findInterfaces();
	for(unsigned int i = 0; i < interfaces.size(); i++) {
		string interface = interfaces[i];
		vector <string> row;
		row.push_back(interfaces[i]);

		struct __netStat ifaceStats;
		interfaceStats[interface] = getIfaceStats(interface);
		if(perSecond && !showTotals && !(oldInterfaceStats.find(interface) == oldInterfaceStats.end()) ) {
			ifaceStats.rx_bytes = uint64_t((interfaceStats[interface].rx_bytes - oldInterfaceStats[interface].rx_bytes) / interval);
			ifaceStats.tx_bytes = uint64_t((interfaceStats[interface].tx_bytes - oldInterfaceStats[interface].tx_bytes) / interval);
		} else if(!perSecond && !showTotals && !(oldInterfaceStats.find(interface) == oldInterfaceStats.end()) ) {
			ifaceStats.rx_bytes = interfaceStats[interface].rx_bytes - oldInterfaceStats[interface].rx_bytes;
			ifaceStats.tx_bytes = interfaceStats[interface].tx_bytes - oldInterfaceStats[interface].tx_bytes;
		} else {
			ifaceStats.rx_bytes = interfaceStats[interface].rx_bytes;
			ifaceStats.tx_bytes = interfaceStats[interface].tx_bytes;
		}

		row.push_back("TX " + humanizeBigNums(ifaceStats.tx_bytes));
		row.push_back("RX " + humanizeBigNums(ifaceStats.rx_bytes));
		rows.push_back(row);
	}
	oldInterfaceStats = map<string, struct __netStat, ltstr>(interfaceStats);
	interfaceStats.clear();
	return rows;
}
