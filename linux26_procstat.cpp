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

// Procinfo-NG is Copyright tabris@tabris.net 2007, 2008, 2009

vector <uint64_t> normalizeCPUstats(const double elapsed, const uint64_t CPUcount, const vector <uint64_t> &input) {
	if(elapsed == 0)
		return input;
	vector <uint64_t> output(input.begin(), input.end() - 2);
	uint64_t timeSum = sumVec(output);

	double factor = (USER_HZ * (CPUcount * elapsed)) / double(timeSum);
	for(uint32_t i = 0; i < output.size(); i++)
		output[i] = uint64_t(double(output[i]) * factor);

	return output;
}

// returns multiple lists of uint64s, cpuDiffs, intrDiffs, and a list consisting of context-switches and the boot-time
vector <vector <uint64_t> > getProcStat(bool showTotals, const uint32_t CPUcount, const double elapsed) {
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
			if(oldCPUstat.empty())
				oldCPUstat.resize(cpuStat.size(), 0);
			cpuDiff = (showTotals ? cpuStat : subVec(cpuStat, oldCPUstat));
			cpuTotal = sumVec(cpuStat);
			oldCPUstat.assign(cpuStat.begin(), cpuStat.end());
			cpuDiff.push_back(cpuTotal);
		} else if(tokens[0] == "intr") {
			if(tokens.size() <= 2) {
				try {
					intrStat = getIRQcount();
				} catch (...) {
				}
			} else {
				// We don't want the second token b/c it's just the total number of interrupts serviced.
				tokens.erase(tokens.begin()); // pop the first token off.
				tokens.erase(tokens.begin()); // pop the second token off.

				intrStat = stringVec2uint64Vec(tokens);
			}
			if(oldIntrStat.size() < intrStat.size())
				oldIntrStat.resize(intrStat.size(), 0);
			intrDiff = (showTotals ? intrStat : subVec(intrStat, oldIntrStat));
			oldIntrStat.assign(intrStat.begin(), intrStat.end());
		} else if(tokens[0] == "ctxt") {
			ctxtStat = string2uint64(tokens[1]);
			ctxtDiff = (showTotals ? ctxtStat : ctxtStat - oldCtxtStat);
			oldCtxtStat = ctxtStat;
		} else if(tokens[0] == "btime") {
			bootTime = string2uint64(tokens[1]);
		}
	}
	vector <vector <uint64_t> > stats(3);
	stats[0] = normalizeCPUstats(elapsed, CPUcount, cpuDiff);
	stats[1] = intrDiff;
	stats[2].push_back(ctxtDiff);
	stats[2].push_back(bootTime);
	return stats;
}

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
	vector <uint64_t> vmStat(7);
	vmStat[0] = pageInDiff;
	vmStat[1] = pageOutDiff;
	vmStat[2] = pageActDiff;
	vmStat[3] = pageDeactDiff;
	vmStat[4] = pageFaultDiff;
	vmStat[5] = swapInDiff;
	vmStat[6] = swapOutDiff;
	return vmStat;
}
