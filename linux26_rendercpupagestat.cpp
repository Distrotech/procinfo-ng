// uses renderPageStat and renderCPUstats to render both CPU and page stats
// returns a list of rows containing 2 columns.
vector< vector <string> > renderCPUandPageStats(bool perSecond, bool showTotals, const double &elapsed,
	const uint64_t &CPUcount, const uint64_t &uptime, const vector <uint64_t> &cpuDiffs, const uint64_t &ctxtDiff,
	const vector <uint64_t> &pageDiffs)

/*vector< vector <string> > renderCPUandPageStats(bool perSecond, bool showTotals, const double elapsed,
	const uint64_t CPUcount, const uint64_t uptime, const vector <uint64_t> cpuDiffs, const uint64_t ctxtDiff,
	const vector <uint64_t> pageDiffs)
*/
{
	vector< vector <string> > rows;
	vector<string> row;
	vector <string> names;
	
	names.push_back(string("user  :")); names.push_back(string("page in :"));
	names.push_back(string("nice  :")); names.push_back(string("page out:"));
	names.push_back(string("system:")); names.push_back(string("page act:")); 
	names.push_back(string("IOwait:")); names.push_back(string("page dea:")); 
	names.push_back(string("hw irq:")); names.push_back(string("page flt:")); 
	names.push_back(string("sw irq:")); names.push_back(string("swap in :"));
	names.push_back(string("idle  :")); names.push_back(string("swap out:"));
	names.push_back(string("uptime:")); names.push_back(string("context :"));

	for(uint32_t i = 0; i < 8; i++) {
		uint64_t val = 0;
		/* 
		 * This abomination is b/c idle is shown near last
		 * but it's 3rd in line in /proc/stat
		 */
		if(i == 7) {
			val = uptime;
		} else if(i == 6) {
			val = cpuDiffs[3];
		} else if(i > 2) {
			val = cpuDiffs[i+1];
		}
		if(val == ~0UL) { val = 0; }
		vector<string> cols = renderCPUstat(perSecond, showTotals, elapsed, CPUcount, cpuDiffs[8], 
			val, names[i*2]);
		row.push_back(cols[0]); row.push_back(cols[1]);

		cols = renderPageStat(perSecond, showTotals, elapsed,
			( i == 7 ? ctxtDiff : pageDiffs[i]), names[i*2+1]);
		row.push_back(cols[0]); row.push_back(cols[1]);
		rows.push_back(row); row.clear();
	}

	return rows;
}
