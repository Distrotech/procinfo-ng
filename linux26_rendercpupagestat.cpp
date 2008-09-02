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
	
	static vector <string> names(16); // Wish I could make this const static.
	bool namesInit = false;
	
	if( unlikely(!namesInit) ) {
		// Initialize only once, should save time.
		names[0]  = "user  :";  names[1]  = "page in :";
		names[2]  = "nice  :";  names[3]  = "page out:";
		names[4]  = "system:";  names[5]  = "page act:"; 
		names[6]  = "IOwait:";  names[7]  = "page dea:"; 
		names[8]  = "hw irq:";  names[9]  = "page flt:"; 
		names[10] = "sw irq:";  names[11] = "swap in :";
		names[12] = "idle  :";  names[13] = "swap out:";
		names[14] = "uptime:";  names[15] = "context :";
		namesInit = true;
	}

	const uint32_t numDiffs = cpuDiffs.size();
	for(uint32_t i = 0; i < 8; i++) {
		uint64_t val = 0;
		/* 
		 * This abomination is b/c idle is shown near last
		 * but it's 3rd in line in /proc/stat
		 */
		switch(i) {
			case 7:
				val = uptime;
				break;
			case 6:
				val = cpuDiffs[3];
				break;
			case 5:
			case 4:
			case 3:
				val = cpuDiffs[i+1];
				break;
			default:
				val = cpuDiffs[i];
		}
		vector<string> row = renderCPUstat(perSecond, showTotals, elapsed, CPUcount, cpuDiffs[numDiffs-1],
			val, names[i*2]);
		//row.push_back(cols[0]); row.push_back(cols[1]);

		const vector<string> cols = renderPageStat(perSecond, showTotals, elapsed,
			( i == 7 ? ctxtDiff : pageDiffs[i]), names[i*2+1]);
		row.push_back(cols[0]); row.push_back(cols[1]);
		rows.push_back(row); row.clear();
	}

	return rows;
}
