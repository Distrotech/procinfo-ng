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

// Procinfo-NG is Copyright tabris@tabris.net 2007, 2008

#ifndef PRETTYPRINT_CPP
#define PRETTYPRINT_CPP

#include <stdio.h>
using namespace std;
#include <vector>
#include <string>
#include <iostream>

bool ncursesInit = false;

static inline int print(const char *fmt, ...) GCC_PRINTFLIKE(1,2);

static inline int print(const char *fmt, ...) {
	va_list argp;
	int code;

	va_start(argp, fmt);

	if(ncursesInit) {
		code = vwprintw(stdscr, fmt, argp);
	} else {
		code = vprintf(fmt, argp);
	}
	va_end(argp);

	return code;
}

//static termios oldTerm;
inline void initConsole() {
/*	static const uint32_t STDIN = 0;
	termios term;
	tcgetattr(STDIN, &term);
	oldTerm = term;
*/	/*
	  enables canonical mode
	  which for our purposes is
	  a fancy name for enabling various
	  raw chars like EOF, EOL, etc.
	*/
/*	term.c_lflag &= !ICANON;
	tcsetattr(STDIN, TCSANOW, &term);
	setbuf(stdin, NULL); // disables line-buffering on stdin
*/
	initscr(); // init ncurses
	ncursesInit = true;
	cbreak();  // turn off line buffering, but leave Ctrl-C alone
	
}

inline void resetConsole() {
	//tcsetattr(0, TCSANOW, &oldTerm);
	ncursesInit = false;
	endwin();
}

// inlined b/c it only has ONE caller.
// returns a list of uint32_t column widths.
static inline vector<uint32_t> getMaxWidths(const vector<vector <string> > &rows, vector<uint32_t> &colWidths) {
	for(uint32_t i = 0; i < rows.size(); i++)
		for(uint32_t j = 0; j < rows[i].size(); j++) {
			if(colWidths.size() < j+1)
				colWidths.resize(j+1);
			if(colWidths[j] < rows[i][j].length())
				colWidths[j] = rows[i][j].length();
		}

	return colWidths;
}

// accepts a list of rows containing columns,
// an optional static list of [minimum] column-widths and leftJustify
// returns nothing
static void prettyPrint(const vector <vector <string> > &rows, vector<uint32_t> &colWidths, bool leftJustify) {
	static const string spaces = // 4 * 80 = 320
		"                                                                                "
		"                                                                                "
		"                                                                                "
		"                                                                                ";

	colWidths = getMaxWidths(rows, colWidths);

	for(uint32_t i = 0; i < rows.size(); i++) {
		string line;
		for(uint32_t j = 0; j < rows[i].size(); j++) {
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

		static const signed int lineLength = 80 - 1;
		/*cout << line
			<< 
			<< endl;
		*/
		print("%s%s\n", line.c_str(), spaces.substr(0, max( (lineLength - (int)line.length()), (int)0) ).c_str() );
	}
}

static void prettyPrint(const vector <vector <string> > &rows, bool leftJustify) {
	vector<uint32_t> colWidths;
	prettyPrint(rows, colWidths, leftJustify);
}

#endif
