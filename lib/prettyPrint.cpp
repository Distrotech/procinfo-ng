/*
	This file is part of procinfo-NG

	procinfo-NG/prettyPrint.cpp is free software; you can redistribute it
	and or modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; version 2.1.

	procinfo-NG is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with procinfo-NG; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Procinfo-NG is Copyright tabris@tabris.net 2007, 2008, 2009

#ifndef PRETTYPRINT_CPP
#define PRETTYPRINT_CPP

#include <stdio.h>
using namespace std;
#include <vector>
#include <string>
#include <iostream>

#ifndef NO_NCURSES
#include <ncurses.h>
bool ncursesInit = false;
#else
#include <stdarg.h>
#include "ncurses_compat.h"
#endif

const static bool ignorethis = false;

static int print(const char *fmt, ...) GCC_PRINTFLIKE(1,2);

static int print(const char *fmt, ...) {
	va_list argp;
	int code;

	va_start(argp, fmt);

#ifndef NO_NCURSES
	if(ncursesInit) {
		code = vwprintw(stdscr, fmt, argp);
	} else {
#endif
		code = vprintf(fmt, argp);
#ifndef NO_NCURSES
	}
#endif
	va_end(argp);

	return code;
}

#ifndef NO_NCURSES
inline void initConsole() {
	initscr(); // init ncurses
	ncursesInit = true;
	cbreak();  // turn off line buffering, but leave Ctrl-C alone
}

inline void resetConsole() {
	ncursesInit = false;
	endwin();
}
#endif

// inlined b/c it only has ONE caller.
// returns a list of uint32_t column widths.
static inline vector<uint32_t> getMaxWidths(const vector<vector <string> > &rows, vector<uint32_t> &colWidths) {
	for(uint32_t i = 0; i < rows.size(); i++) {
		if(colWidths.size() < rows[i].size())
			colWidths.resize(rows[i].size());
		for(uint32_t j = 0; j < rows[i].size(); j++)
			if(colWidths[j] < rows[i][j].length())
				colWidths[j] = rows[i][j].length();
	}

	return colWidths;
}

// accepts a list of rows containing columns,
// an optional static list of [minimum] column-widths, and leftJustify
// returns nothing
static vector <string> __prettyPrint(const vector <vector <string> > &rows, vector<uint32_t> &colWidths, bool leftJustify) {
	const unsigned int numRows = rows.size();
	vector <string> output(numRows);
	colWidths = getMaxWidths(rows, colWidths);

	for(unsigned int i = 0; i < numRows; i++) {
		string line;
		const unsigned int numCols = rows[i].size();
		for(unsigned int j = 0; j < numCols; j++) {
			char fmt[16]; // oversized to be aligned on the stack.
			if(!leftJustify) {
				snprintf(fmt, 10, "%%%s%ds", (!j ? "-" : ""), colWidths[j] + 1);
			} else {
				snprintf(fmt, 10, "%%-%ds", colWidths[j] + 1);
			}
			char subline[128]; // ditto
			snprintf(subline, 100, fmt, rows[i][j].c_str());
			line = line + subline + ((j + 1) == rows[i].size() ? "" : " ");
		}
		output[i] = line;
	}
	return output;
}

static void prettyPrint(const vector <vector <string> > &rows, vector<uint32_t> &colWidths, bool leftJustify) {
	static const string spaces = // 4 * 80 = 320
		"                                                                                ";
		/*
		"                                                                                "
		"                                                                                "
		"                                                                                ";
		*/

	vector <string> lines = __prettyPrint(rows, colWidths, leftJustify);
	const unsigned int numRows = lines.size();
	for(unsigned int i = 0; i < numRows; i++) {
#ifdef PRETTYPRINT_NO_PAD
		print("%s\n", lines[i].c_str());
#else
		static const signed int lineLength = 80 - 1;
		print("%s%s\n", lines[i].c_str(),
			spaces.substr(0,
				max( (lineLength - (signed)lines[i].length()), (signed)0)
				 ).c_str() 
		);
#endif
	}
}

static inline void prettyPrint(const vector <vector <string> > &rows, bool leftJustify) {
	vector<uint32_t> colWidths;
	prettyPrint(rows, colWidths, leftJustify);
}

#endif
