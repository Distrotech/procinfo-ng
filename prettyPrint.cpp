#ifndef PRETTYPRINT_CPP
#define PRETTYPRINT_CPP

// inlined b/c it only has ONE caller.
// returns a list of uint32_t column widths.
static inline vector<uint32_t> getMaxWidths(const vector<vector <string> > &rows) {
	vector<uint32_t> colWidths;

	for(uint32_t i = 0; i < rows.size(); i++)
		for(uint32_t j = 0; j < rows[i].size(); j++) {
			if(colWidths.size() < j+1)
				colWidths.resize(j+1);
			if(colWidths[j] < rows[i][j].length())
				colWidths[j] = rows[i][j].length();
		}

	return colWidths;
}

// accepts a list of rows containing columns, an optional static list of column-widths and leftJustify
// returns nothing
static void prettyPrint(const vector <vector <string> > &rows, vector<uint32_t> *colWidthsPtr, bool leftJustify) {
	vector <uint32_t> colWidths;
	static const string spaces =
		"                                                                                ";

	if(colWidthsPtr == NULL) {
		colWidths = getMaxWidths(rows);
	} else {
		colWidths = *colWidthsPtr;
	}

	for(uint32_t i = 0; i < rows.size(); i++) {
		string line;
		for(uint32_t j = 0; j < rows[i].size(); j++) {
			char fmt[11];
			if(!leftJustify) {
				snprintf(fmt, 10, "%%%s%ds", (!j ? "-" : ""), colWidths[j] + 1);
			} else {
				snprintf(fmt, 10, "%%-%ds", colWidths[j] + 1);
			}
			char subline[101];
			snprintf(subline, 100, fmt, rows[i][j].c_str());
			line = line + subline + ((j + 1) == rows[i].size() ? "" : " ");
		}

		static const signed int lineLength = 80;
		cout << line
			<< spaces.substr(0, max( (lineLength - (int)line.length()), (int)0) )
			<< endl;
	}
}

#endif
