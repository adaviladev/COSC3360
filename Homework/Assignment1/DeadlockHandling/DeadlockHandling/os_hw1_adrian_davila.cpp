#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "Process.h"
#include "Resource.h"
#include "List.h"
#include "Node.h"

using namespace std;

/* GLOBALS */
int timer = 0;
int numberOfProcs;
int *avail;
int **allocated;
int **need;
bool *finished;
bool sortLong = true;

int parseInt(string str) {
	int holder;
	stringstream ss(str);
	ss >> holder;

	return holder;
}

void mergeLong(Process arr1[], Process arr2[], int start, int mid, int end) {
	int ctr1 = start;
	int ctr2 = mid;
	for (int i = start; i < end; i++) {
		if (
			ctr1 < mid && (ctr2 >= end
				|| arr1[ctr1].deadline < arr1[ctr2].deadline
				|| (
					arr1[ctr1].deadline == arr1[ctr2].deadline &&
					arr1[ctr1].computation_time > arr1[ctr2].computation_time)
				)
			) {
			arr2[i] = arr1[ctr1];
			ctr1++;
		}
		else {
			arr2[i] = arr1[ctr2];
			ctr2++;
		}
	}
}

void mergeShort(Process arr1[], Process arr2[], int start, int mid, int end) {
	int ctr1 = start;
	int ctr2 = mid;
	for (int i = start; i < end; i++) {
		if (
			ctr1 < mid && (ctr2 >= end
				|| arr1[ctr1].deadline < arr1[ctr2].deadline
				|| (
						arr1[ctr1].deadline == arr1[ctr2].deadline && 
						arr1[ctr1].computation_time < arr1[ctr2].computation_time
					)
				)
			) {
			arr2[i] = arr1[ctr1];
			ctr1++;
		}
		else {
			arr2[i] = arr1[ctr2];
			ctr2++;
		}
	}
}

void copyArr(Process arr1[], Process arr2[], int start, int end) {
	for (int i = start; i < end; i++) {
		arr1[i] = arr2[i];
	}
}

void mergeSort(Process arr1[], Process arr2[], int start, int end, bool tie) {
	if (end - start <= 1) {
		return;
	}
	int mid = (end + start) / 2;
	mergeSort(arr1, arr2, start, mid, tie);
	mergeSort(arr1, arr2, mid, end, tie);
	if (tie){
		mergeLong(arr1, arr2, start, mid, end);
	}
	else {
		mergeShort(arr1, arr2, start, mid, end);
	}
	copyArr(arr1, arr2, start, end);
}

/*
 * 1 = calculate
 * 2 = request
 * 3 = useResource
 * 4 = release
 */
int *parseInstruction(string inst, int resCount){
	int paramPosStart = inst.find("(");
	int paramPosEnd = inst.find(")");
	string function = inst.substr(0, paramPosStart);
	string params = inst.substr(paramPosStart+1, paramPosEnd - paramPosStart-1);
	int func;
	int *opArray;
	if (function == "calculate"){
		opArray = new int[2];
		opArray[0] = 1;
		opArray[1] = parseInt(params);
	}
	else if (function == "request"){
		func = 2;
		opArray = new int[resCount+1];
		opArray[0] = 2;
		int paramsDelimiter;
		for (int i = 1; i <= resCount; i++) {
			paramsDelimiter = params.find(",");
			opArray[i] = parseInt(params.substr(0, paramsDelimiter));
			params = params.substr(paramsDelimiter + 1);
		}
	}
	else if (function == "useresources"){
		opArray = new int[2];
		opArray[0] = 3;
		opArray[1] = parseInt(params);
	}
	else if (function == "release"){
		opArray = new int[resCount + 1];
		opArray[0] = 4;
		int paramsDelimiter = params.find(",");
		for (int i = 1; i <= resCount; i++) {
			paramsDelimiter = params.find(",");
			opArray[i] = parseInt(params.substr(0, paramsDelimiter));
			params = params.substr(paramsDelimiter + 1);
		}
	}
	else {
		cout << "Function not recognized" << endl;
	}
	return opArray;
}

int pretend(int opCode[], int *need[], int index, int resCount){
	for (int i = 0; i < resCount; i++) {
		if (opCode[i+1] > need[index][i]) {
			/*cout << "Invalid request by Process " << id << ": Request exceeds allowed." << endl;
			cout << "Requested: " << opCode[i + 1] << "; Maximum allowed: " << max[id-1][i] << " for resource " << i << endl;*/
			return -1;
		}
		if(opCode[i + 1] > avail[i]) {
			//cout << "Invalid request: Resources not available." << endl;
			return 0;
		}
	}
	return 1;
}

void displayAvail(int size){
	cout << "Resources available:" << endl;
	char ind = 'A';
	for (int i = 0; i < size; i++){
		cout << ind++ << '\t';
	}
	cout << endl;
	for (int i = 0; i < size; i++){
		cout << avail[i] << '\t';
	}
	cout << endl;
}

void displayAlloc(int procs, int res, int id, int ind){
	cout << "Resources allocated for Process " << id << ":" << endl;
	char resInd = 'A';
	for (int i = 0; i < res; i++){
		cout << resInd++ << '\t';
	}
	cout << endl;
	for (int i = 0; i < res; i++){
		cout << allocated[ind][i] << '\t';
	}
	cout << endl;
}

void displayNeed(int procs, int res, int id, int ind){
	cout << "Resources potentially needed by Process " << id << ":" << endl;
	char resInd = 'A';
	for (int i = 0; i < res; i++){
		cout << resInd++ << '\t';
	}
	cout << endl;
	for (int i = 0; i < res; i++){
		cout << need[ind][i] << '\t';
	}
	cout << endl;
}

bool isFinished() {
	for (int i = 0; i < numberOfProcs; i++) {
		if (!finished[i]) {
			cout << endl << "Not finished: " << i << endl;
			return false;
		}
	}
	return true;
}

int firstUnfinished() {
	for (int i = 0; i < numberOfProcs; i++) {
		if (!finished[i]) {
			return i;
		}
	}
	return numberOfProcs;
}

void resetFinished() {
	for (int i = 0; i < numberOfProcs; i++) {
		finished[i] = false;
	}
}
Process *procArr;

int main(int argc, char *argv[]){
	if (!argv[1]) {
		exit(0);
	}

	string input = argv[1];

	fstream fin(input.c_str());
	string txtLine;
	if (fin.peek() == EOF) {
		return 0;
	}

	int pid;

	int processes;
	int numOfResources;
	int **max;
	int **pipeArr;

		int indexCtr = 0;
	for (int k = 0; k < 1; k++) {
		bool buildingProcAndMax = true;
		fin.clear();
		fin.seekg(0, ios::beg);
		if (sortLong) {
			cout << "\n\nEDF with LJF tiebreaker:" << endl;
		}
		else {
			cout << "\n\nEDF with SJF tiebreaker:" << endl;
		}
		while (getline(fin, txtLine)) { // this loop builds the stack
			if (txtLine == "") {
				continue;
			}
			if (buildingProcAndMax) {
				int resourcePos = txtLine.find(' ');
				numOfResources = parseInt(txtLine.substr(0, resourcePos));
				avail = new int[numOfResources];
				getline(fin, txtLine);
				int procPos = txtLine.find(' ');
				numberOfProcs = processes = parseInt(txtLine.substr(0, procPos));
				finished = new bool[numberOfProcs];
				procArr = new Process[processes];
				for (int i = 0; i < numOfResources; i++){
					getline(fin, txtLine);
					int resourceAmtPos = txtLine.find(' ');
					int resourceAmt = parseInt(txtLine.substr(0, procPos));
					avail[i] = resourceAmt;
				}
				max = new int*[processes];
				need = new int*[processes];
				allocated = new int*[processes];
				for (int j = 0; j < processes; j++) {
					max[j] = new int[numOfResources];
					need[j] = new int[numOfResources];
					allocated[j] = new int[numOfResources];
				}

				for (int j = 0; j < processes; j++) {
					finished[j] = false;
					for (int k = 0; k < numOfResources; k++){
						getline(fin, txtLine);
						int leftBacketPos = txtLine.find('[');
						int commaPos = txtLine.find(',');
						int rightBacketPos = txtLine.find(']');
						int valuePos = txtLine.find('=');
						int procIndex = parseInt(txtLine.substr(leftBacketPos + 1, commaPos - leftBacketPos - 1));
						int resIndex = parseInt(txtLine.substr(commaPos + 1, rightBacketPos - commaPos - 1));
						int value = parseInt(txtLine.substr(valuePos + 1));
						max[procIndex - 1][resIndex - 1] = value;
						need[procIndex - 1][resIndex - 1] = value;
						allocated[procIndex - 1][resIndex - 1] = 0;
					}
				}
				buildingProcAndMax = false;
			} else {
				int undPos = txtLine.find("_");
				if (undPos >= 0) {
					string index = txtLine.substr(undPos + 1, txtLine.find(":") - undPos);
					int procId = parseInt(index);
					procArr[indexCtr].id = procId;
					procArr[indexCtr].index = indexCtr;
					indexCtr++;
					int *fd = procArr[procId - 1].fd;
					getline(fin, txtLine);
					procArr[procId - 1].deadline = parseInt(txtLine);
					getline(fin, txtLine);
					procArr[procId - 1].computation_time = parseInt(txtLine);
					while (getline(fin, txtLine)){
						procArr[procId - 1].instructionSet.insertAtEnd(txtLine);
						if (txtLine == "end"){
							break;
						}
					}
				}
			}
		}
		
		Process *tempArr = new Process[numberOfProcs];
		mergeSort(procArr, tempArr, 0, numberOfProcs, sortLong);
		delete tempArr;
		tempArr = NULL;
		/*cout << "Max Array: " << endl;
		for (int i = 0; i < numberOfProcs; i++) {
			for (int j = 0; j < numOfResources; j++) {
				cout << max[i][j] << ' ';
			}
			cout << endl;
		}*/
		pipeArr = new int*[numberOfProcs * 2];
		for (int i = 0; i < numberOfProcs; i++) {
			Process curProc = procArr[i];
			curProc.upPipe = pipeArr[2 * i] = new int[2]; // read
			curProc.downPipe = pipeArr[((2 * i) + 1)] = new int[2]; // write

			int ph1 = pipe(pipeArr[2 * i]);
			if (ph1 == -1) {
				cout << "Error creating read pipe 1." << endl;
				exit(1);
			}
			else {
				//cout << "Read pipe created for Process " << curProc.id << endl;
			}
			int ph2 = pipe(pipeArr[((2 * i) + 1)]);
			if (ph2 == -1) {
				cout << "Error creating write pipe 2." << endl;
				exit(1);
			}
			else {
				//cout << "Write pipe created for Process " << curProc.id << endl;
			}
			int pid = fork();
			if (pid < 0) {
				cout << "Error creating fork." << endl;
				exit(1);
			}
			else if (pid == 0) {
				//cout << "Starting Child Process " << i << ' ' << curProc.id << endl;
				// Start Process Commands
				//cout << 'a' << endl;
				char downBuffer[20];
				close(curProc.upPipe[0]);
				close(curProc.downPipe[1]);
				Node<string> *curInst = curProc.instructionSet.head;
				while (curInst != NULL){
					string temp = curInst->data + '\0';
					int instructionLength = temp.length();
					write(pipeArr[2 * i][1], temp.c_str(), instructionLength + 1);
					read(pipeArr[((2 * i) + 1)][0], downBuffer, sizeof(downBuffer) / sizeof(downBuffer[0]));
					string tempDownRead = downBuffer;
					if (tempDownRead != "greedy" || tempDownRead != "insufficient") {
						curInst = curInst->next;
					}
					else if (tempDownRead == "greedy") {
						curInst = curInst->next;
						cout << "Request exceeded allowed. Process terminating." << endl;
						break;
					}
				}
				close(curProc.upPipe[1]);
				close(curProc.downPipe[0]);
				// End Process Commands
				//cout << "Exiting Child Process " << i << ' ' << curProc.id << endl;
				exit(0);
			}
			else {
				close(pipeArr[2 * i][1]);
				close(pipeArr[(2 * i) + 1][0]);
			}
		}
		
		int ctr = 0;
		char upBuffer[20];
		for (int i = 0; i < numberOfProcs; i++) {
			if (finished[i]) {
				continue;
			}
			//cout << "Start timer: " << timer << "; " << procArr[i].id << endl;
			Process curProc = procArr[i];
			cout << curProc.index << endl;
			int *upPipe = pipeArr[i * 2];
			int *downPipe = pipeArr[(i * 2) + 1];
			string pipeInput = "Back to child\0";
			int instructionLength = pipeInput.length() + 1;
			//close(curProc.writePipe[0]);
			string tempRead;
			do {
				read(upPipe[0], upBuffer, sizeof(upBuffer) / sizeof(upBuffer[0]));
				tempRead = upBuffer;
				if (tempRead != "end") {
					int *opCode = parseInstruction(tempRead, numOfResources);
					int requestStatus;
					switch (opCode[0]) {
					case 1:
						//cout << "Calculate for " << opCode[1] << "s" << endl;
						timer += opCode[1];
						//cout << "Process " << curProc.id << " calculating for (" << opCode[1] << ") seconds." << endl;
						break;
					case 2:
						requestStatus = pretend(opCode, need, curProc.index, numOfResources);
						if (requestStatus == 1) {
							for (int j = 0; j < numOfResources; j++) {
								avail[j] -= opCode[j + 1];
								allocated[curProc.index][j] += opCode[j + 1];
								need[curProc.index][j] -= opCode[j + 1];
							}
							cout << "Process " << curProc.id << " requested resources (" << opCode[1] << ", " << opCode[2] << ", " << opCode[3] << ") successfully." << endl;
						}
						else if(requestStatus == 0) {
							pipeInput = "insufficient\0";
							instructionLength = pipeInput.length();
							write(downPipe[1], pipeInput.c_str(), instructionLength);
							finished[i] = true;
							continue;
						}
						else {
							pipeInput = "greedy\0";
							instructionLength = pipeInput.length();
							write(downPipe[1], pipeInput.c_str(), instructionLength);
							continue;
						}

						timer++;
						break;
					case 3:
						//cout << "UseResource: " << opCode[1] << endl;
						//cout << "Process " << curProc.id << " used resources (" << opCode[1] << ")." << endl;
						timer += opCode[1];
						break;
					case 4:
						//cout << "Release: " << opCode[1] << " " << opCode[2] << " " << opCode[3] << endl;
						for (int j = 0; j < numOfResources; j++) {
							avail[j] += opCode[j + 1];
							allocated[curProc.index][j] -= opCode[j + 1];
							need[curProc.index][j] += opCode[j + 1];
						}
						timer++;
						cout << "Process " << curProc.id << " released resources (" << opCode[1] << ", " << opCode[2] << ", " << opCode[3] << ") successfully." << endl;
						break;
					default:
						cout << "\033[0;32mFunction not recognized\033[0m" << endl;
						break;
					}
					cout << "Current state of available resources:" << endl;
					displayAvail(numOfResources);
					displayNeed(numberOfProcs, numOfResources, curProc.id, curProc.index);
					displayAlloc(numberOfProcs, numOfResources, curProc.id, curProc.index);
					cout << endl;/**/
				}
				else {
					finished[i] = true;
					for (int k = 0; k < numOfResources; k++) {
						if (allocated[curProc.index][k] > 0) {
							avail[k] += allocated[curProc.index][k];
							allocated[curProc.index][k] -= allocated[curProc.index][k];
						}
					}/**/
					cout << "\033[0;32mProcess " << curProc.id << " finished at time " << timer;
					if (timer - curProc.deadline < 0) {
						cout << " with " << (timer - curProc.deadline) * -1 << " seconds left." << endl;
					}
					else if (timer - curProc.deadline == 0) {
						cout << " with 0 seconds left." << endl;
					}
					else {
						cout << " with " << timer - curProc.deadline << " seconds past its deadline." << endl;
					}
					
					cout << "Current state of available resources:" << endl;
					displayAvail(numOfResources);
					displayNeed(numberOfProcs, numOfResources, curProc.id, curProc.index);
					displayAlloc(numberOfProcs, numOfResources, curProc.id, curProc.index);
					cout << "\033[0m" << endl;/**/
				}
				//cout << "Current state of available resources:" << endl;
				//displayAvail(numOfResources);
				write(downPipe[1], pipeInput.c_str(), instructionLength);
			} while (tempRead != "end");
			/*cout << endl << "Process: " << curProc.id << "; Total time: " << timer << endl;
			cout << "CompTime: " << curProc.computation_time << endl;
			cout << "Deadline: " << curProc.deadline << endl;*/
			if (i == numberOfProcs - 1) {
				if (!isFinished()) {
					i = firstUnfinished();
				}
			}
		}
		if (sortLong) {
			sortLong = false;
			timer = 0;
			resetFinished();
		}
	}

	return 0;
}
