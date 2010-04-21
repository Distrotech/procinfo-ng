inline uint32_t getCPUcount() __attribute__((always_inline));
inline uint32_t getCPUcount() { // has only one call-site.
	vector <string> lines = readFile(string("/proc/cpuinfo"));
	uint32_t CPUcount = 0;
	const uint32_t numLines = lines.size();
	for(uint32_t i = 0; i < numLines; i++) {
		vector <string> tokens = splitString(" ", lines[i]);
		//printf("getCPUcount token0: %s\n", tokens[0].c_str());
		if(!(tokens.empty())) {
			if (tokens[0] == "processor") { // x86/x86_64 Cygwin
				CPUcount++;
			} else if (tokens[0] == "processor\t:") { // x86/x86_64 Linux
				CPUcount++;
			}
#ifdef __sparc__
			else if(tokens[0] == "ncpus") { // SPARC
				CPUcount = string2uint32(tokens[2]); // untested, I don't have an SMP SPARC yet
				break;
			} else if(tokens[0] == "cpu\t\t:") { // SPARC
				CPUcount++;
				break;
			}
#endif
#ifdef __alpha__
			 else if(tokens[0] == "cpus" && tokens[1] == "detected\t:") { // Alpha
				CPUcount = string2uint32(tokens[2]); // untested, I don't have an Alpha yet
				break;
			}
#endif
		} else {
			// do nothing
		}
	}
	return CPUcount;
}
