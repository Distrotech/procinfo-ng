// Unlike most get* functions, this one does the rendering too.
// as such it returns a list of rows like any other render* function
// that is called by mainLoop()
vector <vector <string> > getMeminfo(bool perSecond, bool showTotals, bool showRealMemFree, bool humanizeNums,
	const double &elapsed) 
{
	vector <string> lines = readFile(string("/proc/meminfo"));

	static uint64_t oldMemFree = 0, oldMemTotal = 0, oldSwapTotal = 0, oldSwapFree = 0;
	static uint64_t oldCache = 0, oldBuffers = 0;

	// these have identical names to the keys in meminfo
	int64_t MemTotal = 0, MemFree = 0, Buffers = 0, SwapTotal = 0, SwapFree = 0;
	int64_t MemTotalDiff = 0, MemFreeDiff = 0, BuffersDiff = 0, SwapTotalDiff = 0, SwapFreeDiff = 0;
	int64_t Cache = 0, CacheDiff = 0;

	for(uint32_t i = 0; i < lines.size(); i++) {
		vector <string> tokens = splitString(" ", lines[i]);
		if (!tokens.size()) break;
		if(tokens[0] == "MemTotal:") {
			MemTotal = string2int64(tokens[1]);
			MemTotalDiff = (showTotals ? MemTotal : MemTotal - oldMemTotal);
			oldMemTotal = MemTotal;
		} else if(tokens[0] == "MemFree:") {
			MemFree = string2int64(tokens[1]);
			MemFreeDiff = (showTotals ? MemFree : MemFree - oldMemFree);
			oldMemFree = MemFree;
		} else if(tokens[0] == "Buffers:") {
			Buffers = string2int64(tokens[1]);
			BuffersDiff = (showTotals ? Buffers : Buffers - oldBuffers);
			oldBuffers = Buffers;
		} else if(tokens[0] == "Cached:") {
			Cache = string2int64(tokens[1]);
			CacheDiff = (showTotals ? Cache : Cache - oldCache);
			oldCache = Cache;
		} else if(tokens[0] == "SwapTotal:") {
			SwapTotal = string2int64(tokens[1]);
			SwapTotalDiff = (showTotals ? SwapTotal : SwapTotal - oldSwapTotal);
			oldSwapTotal = SwapTotal;
		} else if(tokens[0] == "SwapFree:") {
			SwapFree = string2int64(tokens[1]);
			SwapFreeDiff = (showTotals ? SwapFree : SwapFree - oldSwapFree);
			oldSwapFree = SwapFree;
		} 
	}
	vector <vector <string> > rows;
	vector <string> row;
	row.push_back("Memory:");
	row.push_back("Total");
	row.push_back("Used");
	row.push_back("Free");
	row.push_back("Buffers");
	rows.push_back(row);
	row.clear();

	row.push_back("RAM:");
	if(humanizeNums) {
		row.push_back(humanizeBigNums(int64_t(MemTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
		row.push_back(humanizeBigNums(int64_t((MemTotalDiff - MemFreeDiff) / 
			(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
		row.push_back(humanizeBigNums(int64_t(MemFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
		row.push_back(humanizeBigNums(int64_t(BuffersDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
	} else {
		row.push_back(int64toString(int64_t(MemTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
		row.push_back(int64toString(int64_t((MemTotalDiff - MemFreeDiff) / 
			(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
		row.push_back(int64toString(int64_t(MemFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
		row.push_back(int64toString(int64_t(BuffersDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	}
	rows.push_back(row);
	row.clear();

	if(showRealMemFree) { // Produces free memory figures that consider Buffers + Cache as disposable.
		int64_t BuffCacheUsed = int64_t(((BuffersDiff + CacheDiff)) / 
			(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)));
		int64_t BuffCacheFree = int64_t((MemFreeDiff + (BuffersDiff + CacheDiff)) / (
			!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)));
		row.push_back("-/+ buffers/cache  ");
		row.push_back("");
		if(humanizeNums) {
			row.push_back(humanizeBigNums(BuffCacheUsed << 10 ));
			row.push_back(humanizeBigNums(BuffCacheFree << 10 ));
		} else {
			row.push_back(int64toString(BuffCacheUsed));
			row.push_back(int64toString(BuffCacheFree));
		}
		rows.push_back(row);
		row.clear();
	}

	row.push_back("Swap:");
	if(humanizeNums) {
		row.push_back(humanizeBigNums(int64_t(SwapTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
		row.push_back(humanizeBigNums(int64_t((SwapTotalDiff - SwapFreeDiff) / 
			(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
		row.push_back(humanizeBigNums(int64_t(SwapFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed))) << 10 ));
	} else {
		row.push_back(int64toString(int64_t(SwapTotalDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
		row.push_back(int64toString(int64_t((SwapTotalDiff - SwapFreeDiff) / 
			(!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
		row.push_back(int64toString(int64_t(SwapFreeDiff / (!perSecond || elapsed == 0 ? 1 : (showTotals ? 1 : elapsed)))));
	}
	rows.push_back(row);
	row.clear();

	return rows;
}
