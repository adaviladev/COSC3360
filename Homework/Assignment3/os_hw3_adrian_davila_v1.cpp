#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <bitset>

#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/mman.h>

#include <math.h>
#include <algorithm>
#include <functional>
#include <locale>
#include <sys/fcntl.h>
#include <iomanip>

#include "List.h"

using namespace std;

struct Address {
	string binAddress;
	string hexAddress;
};

struct Process {
	int id;
	int size;
};

struct Request {
	int procId;
	int segInd;
	int pageInd;
	int offset;
	Address addr;
};

struct ProcReq {
	int id;
	int totalRequests;
	List< Request > *requests;
};

struct FrameEntry {
	int pageNumber;
	FrameEntry *prev;
	FrameEntry *next;
};

/* GLOBALS */
int totPageNum;
int maxSegLen;
int pageSize;
int framesPerProc;
int lookAheadWindow;
int minFrames;
int maxFrames;
int totalProcNum;
int totalSize;
int segBits;
int pageBits;
int offsetBits;

/**
 * Virtual Memory Arrays
 */
Request *mainMemPageFramesArr;
ProcReq *procReqArr;

//<editor-fold desc="Shared Memory">
/**
 * Shared memory
 */
key_t key = IPC_PRIVATE;
int semId;
struct shmProblem {
	int maxSem;
//	Problem problemArr[20];
};
//</editor-fold>
bool isBuilt = false;

/*
 * Semaphore Data
 */

int parseInt( string str ) {
	int holder;
	stringstream ss( str );
	ss >> holder;

	return holder;
}

string hex2Bin( string hexStr ) {
	stringstream ss;
	ss << hex << hexStr;
	unsigned int n;
	ss >> n;
	bitset< 8 > bin( n );

	return bin.to_string();
}

// Taken from http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
// trim from start
static inline string &ltrim( string &s ) {
	s.erase( s.begin() , find_if( s.begin() , s.end() ,
	                              not1( ptr_fun< int , int >( isspace ))));
	return s;
}

// Taken from http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
// trim from end
static inline string &rtrim( string &s ) {
	s.erase( find_if( s.rbegin() , s.rend() ,
	                  not1( ptr_fun< int , int >( isspace ))).base() , s.end());
	return s;
}

// Taken from http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
// trim from both ends
static inline string &trim( string &s ) {
	return ltrim( rtrim( s ));
}

/*void displayProbArr( Problem *arr , int ctr ) {
	cout << endl << setw( 6 ) << "Name" << setw( 20 ) << "Equation" << setw( 20 ) << "SubbedEq" << setw( 8 ) << "Value" << endl;
	for( int i = 0; i < ctr; i++ ) {
		cout << setw( 6 ) << arr[ i ].name;
		cout << setw( 20 ) << arr[ i ].equation;
		cout << setw( 20 ) << arr[ i ].subbedEquation;
		cout << setw( 8 ) << arr[ i ].value << endl;
	}
}*/

void deallocateSems() {
	system( "ipcrm sem 56328241" );
}

void semWait( int id ) {
	struct sembuf sb;
	sb.sem_flg = 0;
	sb.sem_num = id;
	sb.sem_op = -1;
	semop( semId , &sb , 1 );
}

void semSignal( int id ) {
	struct sembuf sb;
	sb.sem_flg = 0;
	sb.sem_num = id;
	sb.sem_op = 1;
	semop( semId , &sb , 1 );
}

int findProcIndById( int procId ) {
	for( int i = 0; i < totalProcNum; i++ ) {
		if( procId == procReqArr[ i ].id ) {
			return i;
		}
	}
	return -1;
}

/*void addDependency( shmProblem *shm , int waitProb , int sigProb ) {
	int maxSems = shm->maxSem;
	int i = shm->problemArr[ waitProb ].numSemWaits++;
	int j = shm->problemArr[ sigProb ].numSemSignals++;
	shm->problemArr[ waitProb ].semWaits[ i ] = maxSems;
	shm->problemArr[ sigProb ].semSignals[ j ] = maxSems;
	shm->maxSem++;
}

void checkForDependencies( shmProblem *shm , int probInd ) {
	stringstream ss( shm->problemArr[ probInd ].equation );
	string temp;
	while( ss >> temp ) {
		int operandInd = findProblemInd( shm->problemArr , temp );
		if( operandInd >= 0 ) {
			addDependency( shm , probInd , operandInd );
		}
	}
}

void buildDependencies( shmProblem *shm ) {
	for( int i = 0; i < probCtr; i++ ) {
		checkForDependencies( shm , i );
	}
	int beforeCoInd = -1;
	int afterCoInd = -1;
	for( int i = 0; i < probCtr; i++ ) {
		beforeCoInd = i - 1;
		afterCoInd = -1;
		for( int j = i; j < probCtr; j++ ) {
			afterCoInd = j;
			break;
		}
		for( i; i < probCtr; i++ ) {
			break;
		}
	}
}*/

int main( int argc , char *argv[] ) {
//	deallocateSems();
//	return 0;
	if( !argv[ 1 ] ) {
		exit( 0 );
	}

	string codeFile = argv[ 1 ];

	fstream finCode( codeFile.c_str());
	if( finCode.peek() == EOF) {
		return 0;
	}

//	semId = semget( key , 50 , IPC_CREAT | 0666 );


	int totalSize = 0;
	string codeLine;
	string temp;
	stringstream ss;
	int requesPos;
	int endReqPos;
	int processCtr = 0;
	int requestCtr = 0;
	List< Process > *processList = new List< Process >();
	List< Request > *requestList = new List< Request >();

	while( getline( finCode , codeLine )) {
		requesPos = codeLine.find( "0x" );
		endReqPos = codeLine.find( "-1" );
		if( endReqPos >= 0 ) {
			continue;
		}
		if( !isBuilt ) {
			temp = codeLine;
			totPageNum = parseInt( temp );
			mainMemPageFramesArr = new Request[totPageNum];
			getline( finCode , codeLine );
			temp = codeLine;
			maxSegLen = parseInt( temp );
			segBits = ( int ) log2( (double) maxSegLen );
			getline( finCode , codeLine );
			temp = codeLine;
			pageSize = parseInt( temp );
			offsetBits = ( int ) log2(( double ) pageSize );
			getline( finCode , codeLine );
			temp = codeLine;
			framesPerProc = parseInt( temp );
			getline( finCode , codeLine );
			temp = codeLine;
			lookAheadWindow = parseInt( temp );
			getline( finCode , codeLine );
			temp = codeLine;
			minFrames = parseInt( temp );
			getline( finCode , codeLine );
			temp = codeLine;
			maxFrames = parseInt( temp );
			getline( finCode , codeLine );
			temp = codeLine;
			totalProcNum = parseInt( temp );
			procReqArr = new ProcReq[totalProcNum];
			for( int i = 0; i < totalProcNum; i++ ) {
				getline( finCode , codeLine );

				ss << codeLine;
				ss >> temp;
				Process tempProc;
				tempProc.id = parseInt( temp );
				procReqArr[ i ].id = tempProc.id;
				procReqArr[ i ].totalRequests = 0;
				procReqArr[ i ].requests = new List< Request >();
				ss >> temp;
				tempProc.size = parseInt( temp );
				totalSize += tempProc.size;
				processList->insertAtEnd( tempProc );
			}
			isBuilt = true;
		} else if( requesPos >= 0 ) {
			Request tempReq;
			ss << codeLine;
			ss >> temp;
			tempReq.procId = parseInt( temp );
			ss >> temp;
			tempReq.addr.hexAddress = temp;
			tempReq.addr.binAddress = hex2Bin( temp );

			int ind = findProcIndById( tempReq.procId );
			if( ind == -1 ) {
				cout << "Invalid Address requested. Segmentation fault" << endl;
				exit( 1 );
			}
			procReqArr[ ind ].requests->insertAtEnd( tempReq );

			requestCtr++;
		}
	}

	Node< Process > *curNode = processList->head;
	Process *procArr = new Process[totalSize];
	int procIndCtr = 0;
	while( curNode != NULL ) {
		procArr[ procIndCtr ].id = curNode->data.id;
		procArr[ procIndCtr ].size = curNode->data.size;

		curNode = curNode->next;
		procIndCtr++;
	}
//	processList->display();
//	processList->destroyList();

//	requestList->display();

	for( int i = 0; i < totalProcNum; i++ ) {
		int pid = fork();
		if( pid < 0 ) {
			cout << "Error creating fork. Terminating program." << endl;
			exit( 1 );
		} else if( pid == 0 ) {
			Node<Request> *curReq = procReqArr[i].requests->head;
			while( curReq != NULL ){
				curReq = curReq->next;
			}
			exit( 0 );
		}
		procReqArr[ i ].requests->display();
	}

	return 0;
}