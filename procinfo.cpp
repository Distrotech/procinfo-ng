#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>
#include <stdlib.h>


using namespace std;

#define zalloc(x) calloc(1, x)

vector<int> getMaxWidths(vector<vector <string> > rows) {
	vector<int> colWidths;

	for(int i = 0; i < rows.size(); i++)
		for(int j = 0; j < rows[i].size(); j++) {
			if(colWidths.size() < j+1)
				colWidths.resize(j+1);
			if(colWidths[j] < rows[i][j].length())
				colWidths[j] = rows[i][j].length();
		}

	return colWidths;
}

int prettyPrint(vector <vector <string> > rows, vector<int> colWidths, bool leftJustify) {
	if(colWidths.size() == 0)
		colWidths = getMaxWidths(rows);

	for(int i = 0; i < rows.size(); i++) {
		string line;
		for(int j = 0; j < rows[i].size(); j++) {
			char *fmt = (char *)zalloc(10);
			if(!leftJustify) {
				sprintf(fmt, "%%%s%ds", (!j ? "-" : ""), colWidths[j] + 1);
			} else {
				sprintf(fmt, "%%-%ds", colWidths[j] + 1);
			}
			char *subline = (char *)zalloc(100);
			sprintf(subline, fmt, rows[i][j].c_str());
			line = line + subline + " ";
		}
		cout << line << endl;
		//printf("%s\n", line.c_str());
	}
}

int main(void) {
	vector<vector <string> > rows;
	const int numRows = 3, numCols = 2; 
	rows.resize(numRows); 
	for(int i = 0; i < numRows; i++) {
		rows[i].resize(numCols);
	}
	rows[0][0] = "I am the very";
	rows[1][0] = "model of a modern";
	rows[2][0] = "major general.";

	rows[0][1] = "I've information";
	rows[1][1] = "vegetable, animal";
	rows[2][1] = "and mineral.";
	const vector <int> empty;
	prettyPrint(rows, empty, false);
}
