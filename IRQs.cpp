vector <struct IRQ> getIRQs() {
	vector <string> lines = readFile("/proc/interrupts");
	
	vector <struct IRQ> IRQs;
	for(uint32_t i = 0; i < lines.size(); i++) {
		struct IRQ irq;
		vector <string> tokens = splitString(" ", lines[i]);
		if (!tokens.size()) continue;
		// we need a char array b/c of isdigit below.
		const char *irqToken = tokens[0].c_str();
		if( !(strlen(irqToken) && isdigit(irqToken[0])) ) {
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
		if (!tokens.size()) continue;
		// we need a char array b/c of isdigit below.
		const char *irqToken = tokens[0].c_str();
		if( !(strlen(irqToken) && isdigit(irqToken[0])) ) {
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
	char countBuf[64]; bzero(countBuf, 64);
#if __WORDSIZE == 64
	// uint64_t is 'long unsigned int' here
	snprintf(countBuf, 63, "%lu", uint64_t(intrDiff / (perSecond && !showTotals ? ( elapsed ? elapsed : 1) : 1)));
#else 
	// uint64_t is 'long long unsigned int' here
	snprintf(countBuf, 63, "%llu", uint64_t(intrDiff / (perSecond && !showTotals ? ( elapsed ? elapsed : 1) : 1)));
#endif
	snprintf(buf, 63, "%10s %-18s", countBuf, irq.devs.substr(0, 18).c_str());
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
