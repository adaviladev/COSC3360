#include <iostream>
#include <string>

#include "List.h"
using namespace std;

class Process{
public:
	int id;
	int index;
	int forkId;
	int fd[2];
	int deadline;
	int computation_time;
	List<string> instructionSet;

	int *allocated;

	int *upPipe;
	int *downPipe;
	
	void request(int, int, int);
	void useresources(int);
	void release(int, int, int);
	void calculate(int);
};