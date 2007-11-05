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
