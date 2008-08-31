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

// uses renderPageStat and renderCPUstats to render both CPU and page stats
// returns a list of rows containing 2 columns.
vector< vector <string> > renderCPUandPageStats(bool perSecond, bool showTotals, const double elapsed,
	const uint64_t CPUcount, const uint64_t uptime, const vector <uint64_t> cpuDiffs, const uint64_t ctxtDiff, const vector <uint64_t> &pageDiffs)
{
	vector< vector <string> > rows;
	vector<string> row;
	vector<string> names;
	
	names.push_back(string("user  :")); names.push_back(string("page in :"));
	names.push_back(string("       ")); names.push_back(string("page out:"));
	names.push_back(string("system:")); names.push_back(string("swap in :"));
	names.push_back(string("idle  :")); names.push_back(string("swap out:"));
	names.push_back(string("uptime:")); names.push_back(string("context :"));

	for(uint32_t i = 0; i <= 4; i++) {
		uint64_t val = 0;
		/* 
		 * This abomination is b/c idle is shown near last
		 * but it's 3rd in line in /proc/stat
		 */
		val = cpuDiffs[i];
		if(i == 4) {
			val = uptime;
		}/* else if(i == 6) {
			val = cpuDiffs[3];
		} else if(i > 2) {
			val = cpuDiffs[i+1];
		}*/
		vector<string> cols = renderCPUstat(perSecond, showTotals, elapsed, CPUcount, cpuDiffs[4],
			val, names[i*2]);
		row.push_back(cols[0]); row.push_back(cols[1]);

		cols = renderPageStat(perSecond, showTotals, elapsed,
			( i == 4 ? ctxtDiff : pageDiffs[i]), names[i*2+1]);
		row.push_back(cols[0]); row.push_back(cols[1]);

		rows.push_back(row); row.clear();
	}

	return rows;
}
