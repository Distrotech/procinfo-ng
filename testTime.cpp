#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#include "routines.cpp"
#include "timeRoutines.cpp"

int main(void) {
	signed int timeDiff = 86400*131;
	//"Fri Feb  6 16:15:32 UTC 2009"
	const signed int startTime = 1233936932;
	cout << time_rel_abbrev(startTime-timeDiff, startTime) << endl;
}