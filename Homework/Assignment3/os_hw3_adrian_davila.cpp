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

/* GLOBALS */
int totPageNum;
int maxSegLen;
int pageSize;
int framesPerProc;
int lookAheadWindow;
int minFrames;
int maxFrames;
int totalProcNum;
int totalDiskSize;
int pageBits;
int offsetBits;
enum ReplaceOpt {
	LRU = 1 ,
	FIFO = 2 ,
	/*LFU = 3 ,
	OPT = 4 ,
	WS = 5 ,*/
};
int option;

struct Address {
	string binAddress;
	string hexAddress;
};

struct Request {
	int procId;
	int segInd;
	int pageInd;
	int offset;
	int diskInd;
	bool serviced;

	Address addr;

	Request() {
		procId = -1;
		serviced = false;
	}
};

struct Page {
	int parentProc;
	int segInd;
	int pageInd;
	int offset;
	int frameInd;
	int diskInd;
	bool isSet;

	Page() {
		parentProc = -1;
		frameInd = -1;
		diskInd = -1;
		isSet = false;
	}

	void setPage( Request request ) {
		frameInd = -1;
		isSet = true;
		parentProc = request.procId;
		segInd = request.segInd;
		pageInd = request.pageInd;
		offset = request.offset;
		diskInd = request.diskInd;
	}

	void setPage( int procId , int seg , int page , int diskIndex ) {
		parentProc = procId;
		segInd = seg;
		pageInd = page;
		diskInd = diskIndex;
		isSet = true;
		frameInd = -1;
	}
};

struct PageTable {
	Page page;
};

struct Segment {
	Page *pageTable;
};

struct Frame {
	int parentProc;
	int segInd;
	int pageInd;
	int diskInd;

	Frame() {
		parentProc = -1;
	}

	void setFrame( Request request ) {
		parentProc = request.procId;
		segInd = request.segInd;
		pageInd = request.pageInd;
		diskInd = request.diskInd;
	}
};

Request *disk;
Frame *MainMemory;

struct Process {
	int id;
	int size;
	int currentAlloc;
	Segment *seg;
	List<Request> *requests;
	int totalRequests;
	int *allocFrames;
	int requestsProcessed;

	Process() {
		id = -1;
		currentAlloc = 0;
		requestsProcessed = 0;
		allocFrames = new int[framesPerProc];
		for( int i = 0; i < framesPerProc; i++ ) {
			allocFrames[ i ] = -1;
		}
	}

	void printCurrentAllocOrder() {
		cout << "Proc: " << id << endl;
		for( int i = 0; i < framesPerProc; i++ ) {
			cout << "MainMemIndex: " << allocFrames[ i ] << endl;
		}
	}

	void updateAllocatedFramesLRU( int mainMemInd = -1 ) {
		int targetIndex = -1;
		for( int i = 0; i < framesPerProc - 1; i++ ) {
			if( allocFrames[ i ] == mainMemInd ) {
				targetIndex = i;
				break;
			}
		}
		if( targetIndex != -1 ) {
			for( int i = targetIndex; i < framesPerProc - 1; i++ ) {
				allocFrames[ i ] = allocFrames[ i + 1 ];
			}
			allocFrames[ framesPerProc - 1 ] = mainMemInd;
		}
	}

	void appendToAlloc( int mainMemInd ) {
		allocFrames[ currentAlloc ] = mainMemInd;

	}

	void updateAllocatedFramesFIFO( int mainMemInd ) {
//		mainMemInd = allocFrames[0];
		for( int i = 0; i < framesPerProc - 1; i++ ) {
			allocFrames[ i ] = allocFrames[ i + 1 ];
		}
		allocFrames[ framesPerProc - 1 ] = mainMemInd;
	}

	/*void updateAllocatedFramesLFU( Node<Request> *head, Node<Request> *req ) {
		Node<Request> *curReq;
		int *freqArr = new int[framesPerProc];
		for(int i = 0; i < framesPerProc; i++){
			freqArr[i] = 0;
			curReq = head;
			while(curReq != NULL){
				if(curReq->data.diskInd == MainMemory[allocFrames[i]].diskInd){
					cout << "INDEX FOUND " << curReq->data.diskInd << " : " << MainMemory[allocFrames[i]].diskInd << endl;
					freqArr[i]++;
				}
				if(&curReq->data == &req->data){
					cout << "MATCH FOUND" << endl;
					break;
				}

				curReq = curReq->next;
			}
		}
	}

	void updateAllocatedFramesWS( Node<Request> *req ) {
		Node<Request> *curReq = req;
		for( int i = framesPerProc - 1; i >= 0; i-- ) {
			req = req->prev;
			allocFrames[ i ] = allocFrames[ i + 1 ];
		}
//		allocFrames[ framesPerProc - 1 ] = mainMemInd;
	}*/
};

struct ProcReq {
	int id;
};

/**
 * Virtual Memory Arrays
 */
Process *procArr;

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
	bitset<32> bin( n );

	return bin.to_string();
}

/**
 * Taken from here: http://www.sanfoundry.com/cpp-program-binary-number-into-decimal/
 */
int bin2Dec( string binStr ) {
	int dec = 0 , rem , num , base = 1;
	num = parseInt( binStr );
	while( num > 0 ) {
		rem = num % 10;
		dec = dec + rem * base;
		base = base * 2;
		num = num / 10;
	}
	return dec;
}

// Taken from http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
// trim from start
static inline string &ltrim( string &s ) {
	s.erase( s.begin() , find_if( s.begin() , s.end() ,
	                              not1( ptr_fun<int , int>( isspace ))));
	return s;
}

// Taken from http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
// trim from end
static inline string &rtrim( string &s ) {
	s.erase( find_if( s.rbegin() , s.rend() ,
	                  not1( ptr_fun<int , int>( isspace ))).base() , s.end());
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
		if( procId == procArr[ i ].id ) {
			return i;
		}
	}
	return -1;
}

int findEmptyFrame() {
	for( int i = 0; i < totPageNum; i++ ) {
		if( MainMemory[ i ].parentProc == -1 ) {
			return i;
		}
	}
	return -1;
}

int findFrameByRequest( Request req ) {
	for( int i = 0; i < totPageNum; i++ ) {
		if( MainMemory[ i ].parentProc == req.procId &&
		    MainMemory[ i ].segInd == req.segInd &&
		    MainMemory[ i ].pageInd == req.pageInd ) {
			return i;
		}
	}

	return -1;
}

void displayMainMemory() {
	for( int i = 0; i < totPageNum; i++ ) {
		cout << i << ":\tProc: " << MainMemory[ i ].parentProc << "\t";
		cout << "Seg: " << MainMemory[ i ].segInd << "  \t";
		cout << "Page: " << MainMemory[ i ].pageInd << endl;
	}
}

int findFrameIndByProcess( int id ) {
	for( int i = 0; i < totalProcNum; i++ ) {
		if( MainMemory[ i ].parentProc == id ) {
			return i;
		}
	}
	return -1;
}

void processRequests( Node<Request> *reqNode ) {
	int reqCtr = 0;
	Node<Request> *reqHead = reqNode;
	while( reqNode != NULL ) {
		Request req = reqNode->data;
		reqNode->data.serviced = true;
		int procInd = findProcIndById( req.procId );
		int mmInd = findFrameByRequest( req );
		if( mmInd != -1 ) {
			switch( option ) {
				case LRU:
					procArr[ procInd ].updateAllocatedFramesLRU( mmInd );
					break;
				case FIFO:
					// Do nothing if frame is already in Main Memory
					break;
				/*case LFU: // Look back to see which was used the least
					cout << "LFU" << endl;
					break;
				case OPT: // look forward to see which will be called latest
					cout << "OPT" << endl;
					break;
				case WS: // moving window using min/max
//					procArr[ procInd ].updateAllocatedFramesWS( reqNode );
					break;*/
				default:
					cout << "Invalid option selected." << endl;
					break;
			}
			MainMemory[ mmInd ].setFrame( req );
			cout << "\033[0;32m\tFrame found for Process " << procArr[ procInd ].id << ".\033[0m" << endl;
		} else {
			// Frame is not in MainMemory
			cout << "\033[0;31m*******\033[0m" << endl;

			if( procArr[ procInd ].currentAlloc < framesPerProc ) {
				mmInd = findEmptyFrame();
				MainMemory[ mmInd ].setFrame( req );
				procArr[ procInd ].seg[ req.segInd ].pageTable[ req.pageInd ].frameInd = mmInd;
				cout << "\033[1;33mFree frame filled for Process " << procArr[ procInd ].id << ".\033[0m" << endl;
				procArr[ procInd ].appendToAlloc( mmInd );
				procArr[ procInd ].currentAlloc++;
				procArr[ procInd ].requestsProcessed++;
			} else {
				mmInd = procArr[ procInd ].allocFrames[ 0 ];
				switch( option ) {
					case LRU:
						break;
					case FIFO:
						// Grab the first index in the allocFrames array of the parent process
						break;
					/*case LFU:
						cout << "LFU" << endl;
//						procArr[procInd].updateAllocatedFramesLFU(reqHead, reqNode);
						break;
					case OPT:
						cout << "OPT" << endl;
						break;
					case WS:
						cout << "WS" << endl;
						break;*/
					default:
						cout << "Invalid option selected." << endl;
						break;
				}
				MainMemory[ mmInd ].setFrame( req );
				cout << "\033[0;36m\tFrame replaced for Process " << procArr[ procInd ].id << ".\033[0m" << endl;
				switch( option ) {
					case LRU:
						procArr[ procInd ].updateAllocatedFramesLRU( mmInd );
						break;
					case FIFO:
						procArr[ procInd ].updateAllocatedFramesFIFO( mmInd );
						break;
					/*case LFU:
						cout << "LFU" << endl;
						break;
					case OPT:
						cout << "OPT" << endl;
						break;
					case WS:
						cout << "WS" << endl;
						break;*/
					default:
						cout << "Invalid option selected." << endl;
						break;
				}
			}
		}

		reqCtr++;
		reqNode = reqNode->next;
	}
	for( int i = 0; i < totalProcNum; i++ ) {
		procArr[ i ].printCurrentAllocOrder();
		cout << endl;
	}
	displayMainMemory();
}

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

	cout << "Choose a replacement method: " << endl;
	cout << "\t1. Least Recently Used" << endl;
	cout << "\t2. First In First Out" << endl;
/*	cout << "\t3. Least Frequently Used" << endl;
	cout << "\t4. OPT" << endl;
	cout << "\t5. Working Set" << endl;*/

	cin >> option;
//	option = 3;

	totalDiskSize = 0;
	string codeLine;
	string temp;
	stringstream ss;
	int requesPos;
	int endReqPos;
	int processCtr = 0;
	int requestCtr = 0;
	List<Process> *processList = new List<Process>();
	List<Request> *requestList = new List<Request>();

	while( getline( finCode , codeLine )) {
		requesPos = codeLine.find( "0x" );
		endReqPos = codeLine.find( "-1" );
		if( endReqPos >= 0 ) {
			continue;
		}
		if( !isBuilt ) {
			temp = codeLine;
			totPageNum = parseInt( temp );
			MainMemory = new Frame[totPageNum];
			getline( finCode , codeLine );
			temp = codeLine;
			maxSegLen = parseInt( temp );
			pageBits = ( int ) log2(( double ) maxSegLen );
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
			procArr = new Process[totalProcNum];
			for( int i = 0; i < totalProcNum; i++ ) {
				getline( finCode , codeLine );

				ss << codeLine;
				ss >> temp;
				Process tempProc;
				tempProc.id = parseInt( temp );
				procArr[ i ].id = tempProc.id;
				procArr[ i ].totalRequests = 0;
				procArr[ i ].requests = new List<Request>();
				ss >> temp;
				tempProc.size = parseInt( temp );
				procArr[ i ].size = tempProc.size;
				totalDiskSize += tempProc.size;
				processList->insertAtEnd( tempProc );
			}
			disk = new Request[totalDiskSize];
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
			procArr[ ind ].requests->insertAtEnd( tempReq );
			requestList->insertAtEnd( tempReq );

			requestCtr++;
		}
	}

	for( int i = 0; i < totalProcNum; i++ ) {
		Node<Request> *curReq = procArr[ i ].requests->head;
		while( curReq != NULL ) {
//			cout << curReq->data.addr.binAddress << endl;
			string bin = curReq->data.addr.binAddress;
			string tempOffsetBits = bin.substr( bin.size() - offsetBits );
			bin = bin.substr( 0 , bin.size() - offsetBits );
			string tempPageBits = bin.substr( bin.size() - pageBits );
			string tempSegBits = bin.substr( 0 , bin.size() - pageBits );

			curReq->data.segInd = bin2Dec( tempSegBits );
			curReq->data.pageInd = bin2Dec( tempPageBits );
			curReq->data.offset = bin2Dec( tempOffsetBits );

//			cout << "\t" << curReq->data.segInd << " " << curReq->data.pageInd << " " << curReq->data.offset << endl;

			curReq = curReq->next;
		}
		procArr[ i ].seg = new Segment[maxSegLen];
		for( int j = 0; j < maxSegLen; j++ ) {
			procArr[ i ].seg[ j ].pageTable = new Page[pageSize];
		}
	}

	Node<Request> *curReq = requestList->head;
	while( curReq != NULL ) {
		string bin = curReq->data.addr.binAddress;
		string tempOffsetBits = bin.substr( bin.size() - offsetBits );
		bin = bin.substr( 0 , bin.size() - offsetBits );
		string tempPageBits = bin.substr( bin.size() - pageBits );
		string tempSegBits = bin.substr( 0 , bin.size() - pageBits );

		curReq->data.segInd = bin2Dec( tempSegBits );
		curReq->data.pageInd = bin2Dec( tempPageBits );
		curReq->data.offset = bin2Dec( tempOffsetBits );
		curReq = curReq->next;
	}

	bool isUnique = true;
	int uniqueCtr = 0;
	curReq = requestList->head;
	List<Request> *uniqueRequests = new List<Request>();
	while( curReq != NULL ) {
		isUnique = true;
		Node<Request> *tempReq = uniqueRequests->head;
		while( tempReq != NULL ) {
			if( curReq->data.procId == tempReq->data.procId &&
			    curReq->data.addr.hexAddress == tempReq->data.addr.hexAddress ) {
				isUnique = false;
				break;
			}
			tempReq = tempReq->next;
		}
		if( isUnique ) {
			uniqueRequests->insertAtEnd( curReq->data );
		}
		curReq = curReq->next;
	}
	disk = new Request[totalDiskSize];
	Node<Request> *tempUnique = uniqueRequests->head;
	int diskInd = 0;
	while( tempUnique != NULL ) {
//		memcpy(disk[diskInd], &tempUnique->data, sizeof(Request));
		disk[ diskInd ] = tempUnique->data;
		disk[ diskInd ].diskInd = diskInd;
		/*disk[diskInd].addr.hexAddress = tempUnique->data.addr.hexAddress;
		cout << tempUnique->data.addr.hexAddress << endl;
		cout << "DiskIndex: " << diskInd << endl;
		cout << disk[diskInd].procId << " " << disk[diskInd].procId << " " << disk[diskInd].addr.hexAddress << endl;*/
		tempUnique->data.diskInd = diskInd;
		tempUnique->data.pageInd = diskInd % maxSegLen;
		tempUnique->data.segInd = diskInd / maxSegLen;
		diskInd++;
		tempUnique = tempUnique->next;
	}

//	int diskNum = 0;
//	int segNum, pageNum;
//	for(int i = 0; i < totalProcNum; i++){
//		for( int j = 0; j < procArr[i].size; j++){
//			cout << "qwer" << endl;
//			segNum = diskNum / maxSegLen;
//			pageNum = diskNum % maxSegLen;
//			cout << "asdf " << segNum << " " << pageNum << endl;
//			procArr[i].seg[segNum].pageTable[pageNum].setPage(i, segNum, pageNum, diskNum);
//			cout << "zxcv" << endl;
//			diskNum++;
//			cout << diskNum << endl;
//		}
//	}
//	cout << "Page table set with " << diskNum << " entries." << endl;
	/**
	 *
	 */
	curReq = requestList->head;
	while( curReq != NULL ) {
		int procInd = findProcIndById( curReq->data.procId );
		for(int i = 0; i < totalDiskSize; i++){
			if(disk[i].procId == curReq->data.procId && disk[i].addr.hexAddress == curReq->data.addr.hexAddress){
				curReq->data.diskInd = i;
				break;
			}
		}
		procArr[ procInd ].seg[ curReq->data.segInd ].pageTable[ curReq->data.pageInd ].setPage( curReq->data );
		curReq = curReq->next;
	}

	processRequests( requestList->head );

//	procArr[0].requests->display();
//	procArr[1].requests->display();
//	procArr[2].requests->display();
	return 0;
}



/*Node<Request> *curReq = requestList->head;
	List<Request> *uniqueRequests = new List<Request>();
	bool isUnique = true;
	int uniqueCtr = 0;
	while( curReq != NULL ) {
		isUnique = true;
		Node<Request> *tempReq = uniqueRequests->head;
		while( tempReq != NULL ) {
			if( curReq->data.addr.hexAddress == tempReq->data.addr.hexAddress ) {
				isUnique = false;
			}
			tempReq = tempReq->next;
		}
		if( isUnique ) {
			cout << uniqueCtr++ << endl;
			uniqueRequests->insertAtEnd( curReq->data );
		}
		curReq = curReq->next;
	}
	disk = new Address[uniqueCtr];
	Node<Request> *tempUnique = uniqueRequests->head;
	int diskInd = 0;
	while( tempUnique != NULL ) {
		disk[diskInd++] = tempUnique->data.addr;
		tempUnique = tempUnique->next;
	}

	for(int i = 0; i < uniqueCtr; i++){
		cout << disk[i].binAddress << " : " << disk[i].hexAddress << endl;
	}

	Node<Process> *curNode = processList->head;
	Process *procArr = new Process[totalDiskSize];
	int procIndCtr = 0;
	while( curNode != NULL ) {
		procArr[ procIndCtr ].id = curNode->data.id;
		procArr[ procIndCtr ].size = curNode->data.size;

		curNode = curNode->next;
		procIndCtr++;
	}*/