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

struct IRQ {
	uint16_t IRQnum;
	string devs;
};

vector <struct IRQ> getIRQs() {
	vector <string> lines = readFile("/proc/interrupts");
	
	vector <struct IRQ> IRQs;
	for(uint32_t i = 0; i < lines.size(); i++) {
		struct IRQ irq;
		vector <string> tokens = splitString(" ", lines[i]);
		if (tokens.empty())
			continue;
		const string irqToken = tokens[0];
		if( !(irqToken.length() && isdigit(irqToken[0])) ) {
			continue;
		}

		string devs; uint32_t j;
		for(j = 0; j < tokens.size(); j++)
			if (tokens[j].find("PIC", 0) != string::npos) {
				break;
			}
			else if (tokens[j].find("MSI", 0) != string::npos) {
				break;
			}
			else if (tokens[j].find("-irq", 0) != string::npos) {
				break;
			}
		for(j++; j < tokens.size(); j++)
			// Think of this loop as the same as
			// perl's join(' ', @tokens[$j .. -1])
			devs = devs + " " + tokens[j];

		irq.IRQnum = (uint16_t)string2uint32(irqToken);
		irq.devs = devs;
		IRQs.push_back(irq);
	}
	return IRQs;
}

vector <uint64_t> getIRQcount() {
	vector <string> lines = readFile("/proc/interrupts");
	
	vector <uint64_t> IRQcount;
	for(uint32_t i = 0; i < lines.size(); i++) {
		//struct IRQ irq;
		vector <string> tokens = splitString(" ", lines[i]);
		if (tokens.empty())
			continue;
		const string irqToken = tokens[0];
		if( !((irqToken.length()) && isdigit(irqToken[0])) ) {
			continue;
		}
		uint32_t irqNum = string2uint32(irqToken);

		uint32_t j;
		for(j = 1; j < tokens.size() - 1; j++) {
			if( tokens[j].length() && isdigit(tokens[j][0])  ) {
				if(IRQcount.size() < irqNum+1) {
					IRQcount.resize(irqNum+1, 0);
				}
				IRQcount[irqNum] += string2uint64(tokens[j]);
			}
			else if (tokens[j].find("PIC", 0) != string::npos) {
				break;
			}
			else if (tokens[j].find("MSI", 0) != string::npos) {
				break;
			}
			else if (tokens[j].find("-irq", 0) != string::npos) {
				break;
			}
		}
	}
	return IRQcount;
}

inline string renderIRQ(bool perSecond, bool showTotals,
	const double &elapsed, const struct IRQ &irq, const uint64_t &intrDiff) __attribute__((always_inline));
	// has only one callsite
inline string renderIRQ(bool perSecond, bool showTotals, const double &elapsed, const struct IRQ &irq, const uint64_t &intrDiff) {
	char buf[64]; bzero(buf, 64);
	string output;

	snprintf(buf, 63, "irq %3d:", irq.IRQnum); 
	output += buf; bzero(buf, 64);

	string count = uint64toString(uint64_t(intrDiff / (perSecond && !showTotals ? ( elapsed ? elapsed : 1) : 1)));
	snprintf(buf, 63, "%10s %-18s", count.c_str(), irq.devs.substr(0, 18).c_str());
	output += string(string(" ") + buf);

	return output;
}

vector< vector <string> > renderIRQs(bool perSecond, bool showTotals, const double &elapsed,
	const vector <struct IRQ> &IRQs, const vector <uint64_t> &intrDiffs)
{
	vector<vector <string> > rows;
	uint32_t split = IRQs.size() / 2 + (IRQs.size() & 1); // is equiv to (IRQs.size() % 2)
	for(uint32_t i = 0; i < split; i++) {
		vector <string> row;
		row.push_back( renderIRQ(perSecond, showTotals, elapsed, IRQs[i], intrDiffs[IRQs[i].IRQnum]) );
		if(i+split < IRQs.size())
			row.push_back( 
				renderIRQ(perSecond, showTotals, elapsed, IRQs[i+split], intrDiffs[IRQs[i+split].IRQnum]) );
		rows.push_back(row);
		
	}
	return rows;
}