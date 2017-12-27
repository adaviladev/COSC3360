//
// Created by hackeysack09 on 11/6/2016.
//

#ifndef SEM_V3_PROBLEM_H
#define SEM_V3_PROBLEM_H

#endif //SEM_V3_PROBLEM_H

#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <locale>

using namespace std;
#define MAXAMOUNT 50

struct Problem {

public:
	int value;
	char equation[MAXAMOUNT];
	char subbedEquation[MAXAMOUNT];
	char name[MAXAMOUNT];
	bool isSolved;
	bool isConcurrent;
	int semWaits[MAXAMOUNT];
	int semSignals[MAXAMOUNT];
	int numSemSignals;
	int numSemWaits;
};