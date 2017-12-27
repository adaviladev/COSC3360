#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

int parseInt(string str) {
	int holder;
	stringstream ss(str);
	ss >> holder;

	return holder;
}

int main(int argc, char *argv[]) {
	if (!argv[1] || !argv[2]) {
		exit(0);
	}

	string codeFile = argv[1];
	string dataFile = argv[2];

	fstream finCode(codeFile.c_str());
	fstream finData(dataFile.c_str());
	string txtLine;
	if (finCode.peek() == EOF || finData.peek() == EOF) {
		return 0;
	}

	cout << "Code file: " << endl;
	string codeLine, dataLine;
	while (getline(finCode, codeLine)) {
		cout << codeLine << endl;
	}
	cout << "End code file: " << endl;

	cout << "Data file: " << endl;
	while (getline(finData, dataLine)) {
		cout << dataLine << endl;
	}
	cout << "End data file: " << endl;

	return 0;
}