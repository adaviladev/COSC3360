#include <iostream>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include <algorithm>
#include <functional>
#include <locale>
#include <sys/fcntl.h>

#include "List.h"
#include "Variable.h"
#include "Problem.h"

using namespace std;

#define SHMSIZE sizeof(Problem)*20

//<editor-fold desc="Shared Memory">
/*
 * Shared memory
 */
key_t key = 837023;
int shmid;
int shmProbid;
Variable *smVarArr;
Problem *shm, *smProbArr;
static Problem *smProb;
static Variable *smVar;
static int *smInt;
static string *smString;
int prev = 123;
//</editor-fold>
/*
 * Semaphore Data
 */

//<editor-fold desc="Semaphore">
/*
 * Taken from:
 * http://web.archive.org/web/20150524005553/http://advancedlinuxprogramming.com/alp-folder/alp-ch05-ipc.pdf
 */
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};

/*
 * Taken from:
 * http://web.archive.org/web/20150524005553/http://advancedlinuxprogramming.com/alp-folder/alp-ch05-ipc.pdf
 */
/* Obtain a binary semaphore’s ID, allocating if necessary. */
int binary_semaphore_allocation (key_t key, int sem_flags)
{
	return semget (key, 1, sem_flags);
}

/*
 * Taken from:
 * http://web.archive.org/web/20150524005553/http://advancedlinuxprogramming.com/alp-folder/alp-ch05-ipc.pdf
 */
/* Deallocate a binary semaphore. All users must have finished their
use. Returns -1 on failure. */
int binary_semaphore_deallocate (int semid)
{
	union semun ignored_argument;
	return semctl (semid, 1, SETVAL, ignored_argument);
}

/*
 * Taken from:
 * http://web.archive.org/web/20150524005553/http://advancedlinuxprogramming.com/alp-folder/alp-ch05-ipc.pdf
 */
int binary_semaphore_initialize (int semid)
{
	union semun argument;
	unsigned short values[1];
	values[0] = 1;
	argument.array = values;
	return semctl (semid, 0, SETALL, argument);
}

/*
 * Taken from:
 * http://web.archive.org/web/20150524005553/http://advancedlinuxprogramming.com/alp-folder/alp-ch05-ipc.pdf
 */
int binary_semaphore_wait (int semid)
{
	struct sembuf operations[1];
	operations[0].sem_num = 0;
	operations[0].sem_op = -1;
	operations[0].sem_flg = SEM_UNDO;
	return semop (semid, operations, 1);
}

/*
 * Taken from:
 * http://web.archive.org/web/20150524005553/http://advancedlinuxprogramming.com/alp-folder/alp-ch05-ipc.pdf
 */
int binary_semaphore_post (int semid)
{
	struct sembuf operations[1];
	operations[0].sem_num = 0;
	operations[0].sem_op = 1;
	operations[0].sem_flg = SEM_UNDO;
	return semop (semid, operations, 1);
}

//sem_t sem;
sem_t *smSemArr;
int pshared = 1;
unsigned int value = 1;
//</editor-fold>

int probCtr = 0;
int varCtr = 0;

//<editor-fold desc="String Manipulation">
int parseInt( string str ) {
	int holder;
	stringstream ss( str );
	ss >> holder;

	return holder;
}
//</editor-fold>

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

int findProblemInd( Problem *probArr , string name ) {
	for( int i = 0; i < probCtr; i++ ) {
		if( probArr[ i ].name == name ) {
			return i;
		}
	}
	return -1;
}

Variable findVariable( string name ) {
	for( int i = 0; i < probCtr; i++ ) {
		if( smVarArr[ i ].name == name ) {
			return smVarArr[ i ];
		}
	}
	return Variable();
}

int main( int argc , char *argv[] ) {
	if( !argv[ 1 ] || !argv[ 2 ] ) {
		exit( 0 );
	}

	string codeFile = argv[ 1 ];
	string dataFile = argv[ 2 ];

	fstream finPreCode( codeFile.c_str());
	fstream finCode( codeFile.c_str());
	fstream finData( dataFile.c_str());
	if( finCode.peek() == EOF || finData.peek() == EOF) {
		return 0;
	}

//	int semId = binary_semaphore_allocation(key, 0666 | IPC_CREAT );
//	int sem = binary_semaphore_initialize(semId);

	smInt = ( int * ) mmap( NULL , sizeof *smInt , PROT_READ | PROT_WRITE , MAP_SHARED | MAP_ANONYMOUS , -1 , 0 );
	smString = ( string * ) mmap( NULL , sizeof *smString , PROT_READ | PROT_WRITE , MAP_SHARED | MAP_ANONYMOUS , -1 , 0 );
	smProb = ( Problem * ) mmap( NULL , sizeof *smProb , PROT_READ | PROT_WRITE , MAP_SHARED | MAP_ANONYMOUS , -1 , 0 );
	smVar = ( Variable * ) mmap( NULL , sizeof *smVar , PROT_READ | PROT_WRITE , MAP_SHARED | MAP_ANONYMOUS , -1 , 0 );

	*smInt = 1;

	smVarArr = ( Variable * ) shmat( shmid , ( void * ) 0 , 0 );


	string codeLine , dataLine;
	List< string > *inputVar = new List< string >();
	List< string > *probList = new List< string >();
	while( getline( finPreCode , codeLine )) {
		int eqPos = codeLine.find( "=" );
		if( eqPos >= 0 ) {
			probList->insertAtEnd( trim( codeLine ));
			probCtr++;
		}
	}
	smProbArr = new Problem[probCtr];
//	smProbArr = shm;
	Node< string > *cur = probList->head;
	int ind = 0;
	Problem *tempProb;
//	tempProb = shm;
	while( cur != NULL ) {
		tempProb = new Problem();
		int delimPos = cur->data.find( "=" );
		string temp = cur->data.substr( 0 , delimPos );
		string newName = trim( temp );
		temp = cur->data.substr( delimPos + 1 );
		string newEq = trim( temp );
		tempProb->setProperties( newName , newEq );
		tempProb->semInd = ind;
		smProbArr[ ind ].setProperties( newName , newEq );
		cur = cur->next;
//		tempProb++;
		ind++;
	}
//	shm = smProbArr;
	while( getline( finCode , codeLine )) {
		codeLine = trim( codeLine );
		int inputPos = codeLine.find( "input" );
		int internPos = codeLine.find( "internal" );
		int conPos = codeLine.find( "cobegin" );
		int newProc = codeLine.find( "=" );
		if( inputPos >= 0 ) {
			int varStartPos = codeLine.find( " " );
			codeLine = codeLine.substr( varStartPos + 1 );
			codeLine = codeLine.substr( 0 , codeLine.length() - 1 );
			int commaPos = codeLine.find( "," );
			while( codeLine != "" ) {
				string temp = codeLine.substr( 0 , commaPos );
				inputVar->insertAtEnd( temp );
				commaPos = codeLine.find( "," );
				codeLine = codeLine.substr( commaPos + 1 );
				commaPos = codeLine.find( "," );
				varCtr++;
				if( commaPos < 0 ) {
					inputVar->insertAtEnd( codeLine );
					codeLine = "";
					varCtr++;
				}
			}
			List< int > *dataVal = new List< int >();
			while( getline( finData , dataLine )) {
				commaPos = dataLine.find( "," );
				if( commaPos >= 0 ) {

				} else {
					stringstream ss( dataLine );
					int temp;
					while( ss >> temp ) {
						dataVal->insertAtEnd( temp );
					}
				}
			}
			smVarArr = new Variable[varCtr];
			Node< string > *curVar = inputVar->head;
			Node< int > *curVal = dataVal->head;
			for( int i = 0; i < varCtr; i++ ) {
				smVarArr[ i ].name = curVar->data;
				smVarArr[ i ].value = curVal->data;
				curVar = curVar->next;
				curVal = curVal->next;
			}
		} else if( internPos >= 0 ) {
			int varStartPos = codeLine.find( " " );
			int pPos;
			codeLine = codeLine.substr( varStartPos );
			int commaPos = codeLine.find( ',' );
			int innerProbCtr = 0;
			while( codeLine != ";" ) {
				pPos = codeLine.find( "p" );
//				smVarArr[ probCtr ].proc = codeLine.substr( pPos , commaPos - 1 );
				smProbArr[ innerProbCtr ].semInd = innerProbCtr;
				smProbArr[ innerProbCtr ].isSolved = false;
				codeLine = codeLine.substr( commaPos );
				innerProbCtr++;
			}
//			smSemArr = new sem_t[innerProbCtr];
//			for( int i = 0; i < innerProbCtr; i++ ) {
//				sem_init( &smSemArr[ i ] , pshared , value );
//			}
		} else if( conPos >= 0 ) {
			int endPos;
			while( getline( finCode , codeLine )) {
				codeLine = trim( codeLine );
				endPos = codeLine.find( "coend" );
				if( endPos >= 0 ) {
					break;
				}
				int pid = fork();
				if( pid < 0 ) {
					cout << "Error creating fork." << endl;
					exit( 1 );
				} else if( pid == 0 ) {
					(*smInt)++;
					string name = codeLine.substr( 0 , codeLine.find( "=" ));
					name = trim( name );
					int index = findProblemInd( smProbArr , name );
					if( index == -1 ) {
						cout << "Problem " << name << " not initialised." << endl;
						exit( 1 );
					}
//					sem_wait( &smSemArr[ prob.semInd ] );

//					cout << smProbArr[ index ].name << " | waited : " << smProbArr[ index ].equation << endl;

//					*smInt = smProbArr[index].semInd;
					( *smInt ) = smProbArr[ index ].semInd + 3;
//					 *smString = smProbArr[ index ].name;
					smProbArr[ 0 ].equation = "test";
					cout << "hit " << getpid() << endl;
//					sem_post( &smSemArr[ probCtr - shm[index].semInd ] );
//					cout << "Problem " << smProbArr[ index ].name << " finished on " << smProbArr[ index ].semInd << endl;
					cout << "post: " << binary_semaphore_post(semId) << endl;

					exit( 0 );
				} else {
//					smVarArr[ 0 ].val = smVarArr[ 0 ].val + 1;

//					wait( NULL );
					printf( "%d\n" , *smInt );
//					cout << *smString << endl;
				}
//				concurrent->insertAtEnd( codeLine );
			}
//		} else if( newProc >= 0 ) {

		}
	}

//	cout << "parent end" << endl;
//	cout << varCtr << endl;
//	for( int i = 0; i < varCtr; i ++ ) {
//		cout << smVarArr[ i ].name << ' ' << &smVarArr[ i ] << endl;
//	}
//	for( int i = 0; i < probCtr; i++ ) {
//		cout << &shm[ i ] << ' ' << shm[0][ i ].name << endl;
//		cout << &smProbArr[ i ] << ' ' << smProbArr[ i ].name << ';' << endl;
//	}

	binary_semaphore_wait(semId);
	cout << "gate 1" << endl;
	binary_semaphore_wait(semId);
	cout << "gate 2" << endl;
	binary_semaphore_wait(semId);
	cout << "gate 3" << endl;
	cout << "parent waited" << endl;
//	binary_semaphore_deallocate(semId);
//	shmdt(smInt);
//	shmdt(smString);
//	shmdt(smProb);
//	shmdt(smVar);
//	shmctl(shmid, IPC_RMID, 0);

	return 0;
}