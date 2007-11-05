// returns the contents of /proc/vmstat, only the parts we want.
// as such it returns a vector of 4 elements, pageInDiff, pageOutDiff, swapInDiff, swapOutDiff
vector <uint64_t> getVMstat(bool showTotals) {
	vector <string> lines = readFile(string("/proc/stat"));

	static uint64_t oldPageIn = 0, oldPageOut = 0, oldSwapIn = 0, oldSwapOut = 0;
	uint64_t pageIn = 0, pageOut = 0, swapIn = 0, swapOut = 0;
	uint64_t pageInDiff = 0, pageOutDiff = 0, swapInDiff = 0, swapOutDiff = 0;

	static uint64_t oldPageAct = 0, oldPageDeact = 0, oldPageFault = 0;
	uint64_t pageAct = 0, pageDeact = 0, pageFault = 0;
	uint64_t pageActDiff = 0, pageDeactDiff = 0, pageFaultDiff = 0;

	for(uint32_t i = 0; i < lines.size(); i++) {
		vector <string> tokens = splitString(" ", lines[i]);
		if (!tokens.size()) break;
		if(tokens[0] == "page") {
			pageIn = string2uint64(tokens[1]);
			pageInDiff = (showTotals ? pageIn : pageIn - oldPageIn);
			oldPageIn = pageIn;
			pageOut = string2uint64(tokens[2]);
			pageOutDiff = (showTotals ? pageOut : pageOut - oldPageOut);
			oldPageOut = pageOut;
		} else if(tokens[0] == "swap") {
			swapIn = string2uint64(tokens[1]);
			swapInDiff = (showTotals ? swapIn : swapIn - oldSwapIn);
			oldSwapIn = swapIn;
			swapOut = string2uint64(tokens[2]);
			swapOutDiff = (showTotals ? swapOut : swapOut - oldSwapOut);
			oldSwapOut = swapOut;
		}
	}
	vector <uint64_t> vmStat;
	vmStat.push_back(pageInDiff);
	vmStat.push_back(pageOutDiff);
	vmStat.push_back(swapInDiff);
	vmStat.push_back(swapOutDiff);
	return vmStat;
}
