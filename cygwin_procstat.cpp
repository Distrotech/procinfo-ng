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
