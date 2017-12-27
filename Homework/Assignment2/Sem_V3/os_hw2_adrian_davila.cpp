#include <iostream>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/mman.h>

#include <algorithm>
#include <functional>
#include <locale>
#include <sys/fcntl.h>
#include <iomanip>

#include "List.h"
#include "Variable.h"
#include "Problem.h"
//#include "Arithmetic.h"
#include "Arithmetic.cpp"

using namespace std;

#define SHMSIZE sizeof(Problem)*20

//<editor-fold desc="Shared Memory">
/*
 * Shared memory
 */
key_t key = IPC_PRIVATE;
int semId;
int shmid;
int shmProbid;
Variable *smVarArr , *varArr;
Problem *shm , *smProbArr , *probArr;
static Problem *smProb;
static Variable *smVar;
static int *smInt;
static string *smString;
int prev = 123;
struct shmProblem {
	int maxSem;
	Problem problemArr[20];
};
//</editor-fold>
/*
 * Semaphore Data
 */

//<editor-fold desc="Semaphore">

//sem_t sem;
sem_t *smSemArr;

int pshared = 1;

unsigned int value = 1;
//</editor-fold>

int probCtr = 0;
int probArrInd = 0;
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

int findProblemInd( Problem *probArr , string name ) {
	for( int i = 0; i < probCtr; i++ ) {
		if( strncmp( &probArr[ i ].name[ 0 ] , name.c_str() , strlen( &probArr[ i ].name[ 0 ] )) == 0 ) {
			return i;
		}
	}
	return -1;
}

int findVariableInd( Variable *varArr , string name ) {
	for( int i = 0; i < varCtr; i++ ) {
//		if( strncmp(&varArr[ i ].name[0], name.c_str(), strlen(&varArr[ i ].name[0]) ) == 0) {
//			return i;
//		}
		if( varArr[ i ].name == name ) {
			return i;
		}
	}
	return -1;
}

void displayProbArr( Problem *arr , int ctr ) {
	cout << endl << setw( 6 ) << "Name" << setw( 20 ) << "Equation" << setw( 20 ) << "SubbedEq" << setw( 8 ) << "Value" << endl;
	for( int i = 0; i < ctr; i++ ) {
		cout << setw( 6 ) << arr[ i ].name;
		cout << setw( 20 ) << arr[ i ].equation;
		cout << setw( 20 ) << arr[ i ].subbedEquation;
		cout << setw( 8 ) << arr[ i ].value << endl;
	}
}

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

void addDependency( shmProblem *shm , int waitProb , int sigProb ) {
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
//					cout << "\nTemp: " << temp;
		int operandInd = findProblemInd( shm->problemArr , temp );
		if( operandInd >= 0 ) {
//			if( shm->problemArr[ operandInd ].isConcurrent ) {
			addDependency( shm , probInd , operandInd );
//			}
		}
	}
}

void buildDependencies( shmProblem *shm ) {
	for( int i = 0; i < probCtr; i++ ) {
//		if( shm->problemArr[ i ].isConcurrent ) {
		checkForDependencies( shm , i );
//		}
	}
	int beforeCoInd = -1;
	int afterCoInd = -1;
	for( int i = 0; i < probCtr; i++ ) {
//		if( shm->problemArr[ i ].isConcurrent ) {
		beforeCoInd = i - 1;
		afterCoInd = -1;
		for( int j = i; j < probCtr; j++ ) {
//				if( !shm->problemArr[ j ].isConcurrent ) {
			afterCoInd = j;
			break;
//				}
		}
		for( i; i < probCtr; i++ ) {
//				if( !shm->problemArr[ i ].isConcurrent ) {
			break;
//				}
			if( beforeCoInd != -1 ) {
				addDependency( shm , i , beforeCoInd );
			}
			if( afterCoInd != -1 ) {
				addDependency( shm , afterCoInd , i );
			}
		}
	}
//	}
}

/*int checkOpImportance( string op ) {
	if( op == "(" ) {
		return 4;
	} else if( op == ")" ) {
		return 3;
	} else if( op == "*" ) {
		return 2;
	} else if( op == "/" ) {
		return 2;
	} else if( op == "+" ) {
		return 1;
	} else if( op == "-" ) {
		return 1;
	} else {
		return -1;
	}
}*/

//int checkOpImportance(char op){
//	cout << "checking Importance of: " << op << endl;
//	switch(op){
//		case '(':
//			return 4;
//		case ')':
//			return 3;
//		case '*':
//			return 2;
//		case '/':
//			return 2;
//		case '+':
//			return 1;
//		case '-':
//			cout << "minus" << endl;
//			return 1;
//		default:
//			return -1;
//	}
//}

/*int performOperation( int op1 , int op2 , string op ) {
	if( op == "*" ) {
		return op1 * op2;
	} else if( op == "/" ) {
		return op1 / op2;
	} else if( op == "+" ) {
		return op1 + op2;
	} else if( op == "-" ) {
		return op1 - op2;
	} else {
		return -1;
	}
}*/

//int performOperation(int op1, int op2, char op){
//	switch(op){
//		case '*':
//			return op1 * op2;
//		case '/':
//			return op1 / op2;
//		case '+':
//			return op1 + op2;
//		case '-':
//			return op1 - op2;
//		default:
//			return -1;
//	}
//}

/*int evaluate( char *expression ) {
	cout << "Expression: " << expression << endl;
//	return 0;
	List< int > *operandStack = new List< int >();
	List< string > *operatorStack = new List< string >();
	string expStr = expression;
	stringstream ss( expStr );
	string temp;
	while( ss >> temp ) {
		cout << "hit " << temp << endl;
		int importance = checkOpImportance( temp );
		if( importance == 3 ) {
			while( checkOpImportance( operatorStack->top()->data ) != 4 ) {
				int operand2 = operandStack->pop()->data;
				int operand1 = operandStack->pop()->data;
				string operation = operatorStack->pop()->data;
				operandStack->insertAtEnd( performOperation( operand2 , operand1 , operation ));
				cout << "parenthesis" << endl;
			}
			operatorStack->pop()->data;
//			continue;
		} else if( importance != -1 ) {
			while( importance > checkOpImportance( operatorStack->top()->data )) {
				cout << "qwer" << endl;
				int operand2 = operandStack->pop()->data;
				int operand1 = operandStack->pop()->data;
				string operation = operatorStack->pop()->data;
				cout << "asdf" << endl;
				operandStack->insertAtEnd( performOperation( operand2 , operand1 , operation ));
				cout << "zxcv" << endl;
			}
//			continue;
		}
		if( isdigit( temp[ 0 ] )) {
			operandStack->insertAtEnd( parseIntExpression( temp ));
		}
		cout << "end: " << temp << endl;
	}

	cout << endl << endl;
	while( operatorStack->head != NULL ) {
		int operand2 = operandStack->pop()->data;
		int operand1 = operandStack->pop()->data;
		string operation = operatorStack->pop()->data;
		operandStack->insertAtEnd( performOperation( operand2 , operand1 , operation ));
	}
	return operandStack->head->data;
}*/

//int evaluate( char *expression ) {
////	cout << "Expression: " << expression << endl;
////	return 2;
//	List<int> *operandStack = new List<int>();
//	List<char> *operatorStack = new List<char>();
//	while(*expression != '\0'){
//		if(*expression == ' '){
//			expression++;
//			continue;
//		}
//		/*while(*expression != ' '){
//			expression++;
//			if(*expression == 0){
//				break;
//			}
//		}*/
//
//		if(*expression == '\0'){
//			break;
//		}
//		int importance = checkOpImportance(*expression);
//		if(importance == 3){
//			while(checkOpImportance(operatorStack->top()->data) != 4){
//				int operand2 = operandStack->pop()->data;
//				int operand1 = operandStack->pop()->data;
//				char operation = operatorStack->pop()->data;
//				operandStack->insertAtEnd(performOperation(operand2, operand1, operation));
//			}
//			operatorStack->pop()->data;
//			expression++;
//			continue;
//		} else if (importance != -1){
//			if(operatorStack->top() != NULL) {
//				while( importance > checkOpImportance( operatorStack->top()->data )) {
//					int operand2 = operandStack->pop()->data;
//					int operand1 = operandStack->pop()->data;
//					char operation = operatorStack->pop()->data;
//					operandStack->insertAtEnd( performOperation( operand2 , operand1 , operation ));
//				}
//			} else {
//				int operand2 = operandStack->pop()->data;
//				int operand1 = operandStack->pop()->data;
//				char operation = *expression;
//				operandStack->insertAtEnd( performOperation( operand2 , operand1 , operation ));
//			}
//			expression++;
//			continue;
//		}
//		if((*expression >= 48) && *expression <= 57){
//			operandStack->insertAtEnd(atoi(expression));
//			cout << operandStack->last->data << endl;
//		}
//		while(isdigit(*expression)){
//			expression++;
//		}
//	}
//
//	while(operatorStack->head != NULL){
//		int operand2 = operandStack->pop()->data;
//		int operand1 = operandStack->pop()->data;
//		char operation = operatorStack->pop()->data;
//		operandStack->insertAtEnd(performOperation(operand2, operand1, operation));
//	}
//	return operandStack->head->data;
//}

int main( int argc , char *argv[] ) {
//	deallocateSems();
//	return 0;
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

	semId = semget( key , 50 , IPC_CREAT | 0666 );

	string codeLine , dataLine;
	shmProblem *shmProb;
	List<string> *inputVar = new List<string>();
	List<string> *concurrent = new List<string>();
	List<string> *probList = new List<string>();

	int newProc = codeLine.find( "=" );
	while( getline( finPreCode , codeLine )) {
		newProc = codeLine.find( "=" );
		if( newProc >= 0 ) {
			probCtr++;
		}
	}

	while( getline( finCode , codeLine )) {
		codeLine = trim( codeLine );
		if( codeLine == "{" || codeLine == "}" ) {
			continue;
		}
		int inputPos = codeLine.find( "input" );
		int internPos = codeLine.find( "internal" );
		int conPos = codeLine.find( "cobegin" );
		int readPos = codeLine.find( "read" );
		newProc = codeLine.find( "=" );
		int writePos = codeLine.find( "write" );
		if( readPos >= 0 ) {
			continue;
		}
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
			List<int> *dataVal = new List<int>();
			while( getline( finData , dataLine )) {
				if( dataLine == "" ) {
					continue;
				}
				commaPos = dataLine.find( "," );
				if( commaPos >= 0 ) {
					while( dataLine != "" ) {
						string temp = dataLine.substr( 0 , commaPos );
						dataVal->insertAtEnd( parseInt( temp ));
						commaPos = dataLine.find( "," );
						dataLine = dataLine.substr( commaPos + 1 );
						commaPos = dataLine.find( "," );
						if( commaPos < 0 ) {
							dataVal->insertAtEnd( parseInt( dataLine ));
							dataLine = "";
						}
					}
				} else {
					stringstream ss( dataLine );
					int temp;
					while( ss >> temp ) {
						dataVal->insertAtEnd( temp );
					}
				}
			}
			varArr = new Variable[varCtr];
			Node<string> *curVar = inputVar->head;
			Node<int> *curVal = dataVal->head;
			for( int i = 0; i < varCtr; i++ ) {
				varArr[ i ].name = curVar->data;
				varArr[ i ].value = curVal->data;
				curVar = curVar->next;
				curVal = curVal->next;
			}
			inputVar->destroyList( inputVar->head );
		} else if( internPos >= 0 ) {
			int varStartPos = codeLine.find( " " );
			int pPos;
			codeLine = codeLine.substr( varStartPos );
			int commaPos = codeLine.find( ',' );
			int innerProbCtr = 0;
			while( codeLine != ";" ) {
				pPos = codeLine.find( "p" );
				probList->insertAtEnd( codeLine.substr( pPos , commaPos - 1 ));
				codeLine = codeLine.substr( commaPos );
				innerProbCtr++;
			}
//			probCtr = innerProbCtr;
			probArr = new Problem[innerProbCtr];
			shmProb = new shmProblem();
//			shmProb->problemArr = new Problem[probCtr];
			for( int i = 0; i < innerProbCtr; i++ ) {
//				probArr[ innerProbCtr ].semInd = i;
				probArr[ innerProbCtr ].isSolved = false;
			}
		} else if( conPos >= 0 ) {
			int endPos;
			while( getline( finCode , codeLine )) {
				codeLine = trim( codeLine );
				endPos = codeLine.find( "coend" );
				if( endPos >= 0 ) {
					break;
				}

				concurrent->insertAtEnd( codeLine );
			}
			Node<string> *cur = concurrent->head;
			while( cur != NULL ) {
				int eqPos = cur->data.find( "=" );
				string temp = cur->data.substr( 0 , eqPos );
				strcpy( &probArr[ probArrInd ].name[ 0 ] , trim( temp ).c_str());
				temp = cur->data.substr( eqPos + 1 );
				int openParen = temp.find( "(" );
				stringstream holder;
				while( openParen >= 0 ) {
					int closeParen = temp.find( ")" );
					string parenStmt = temp.substr( 0 , closeParen );
					while( holder << parenStmt ) {
						holder << " ";
					}
					string expHolder = temp.substr( openParen , 1 );
					temp = temp.substr( openParen + 1 );
					holder << expHolder;
					holder << " ";
				}
				temp = temp.substr( 0 , temp.length() - 1 );
				strcpy( &probArr[ probArrInd ].equation[ 0 ] , trim( temp ).c_str());
				probArr[ probArrInd ].isConcurrent = true;

				cur = cur->next;
				probArrInd++;
			}
		} else {
			int eqPos = codeLine.find( "=" );
			if( eqPos >= 0 ) {
				int eqPos = codeLine.find( "=" );
				string temp = codeLine.substr( 0 , eqPos );
				const char *str = trim( temp ).c_str();
				strcpy( &probArr[ probArrInd ].name[ 0 ] , str );
				temp = codeLine.substr( eqPos + 1 );
				temp = trim( temp );
				stringstream holder;
				stringstream ss( temp );
				string expHolder;
				while( ss >> expHolder ) {
					expHolder = trim( expHolder );
					if( expHolder[ 0 ] == '(' ) {
						holder << " ( ";
						expHolder = expHolder.substr( 1 );
						holder << expHolder << " ";
					} else if( expHolder[ expHolder.size() - 1 ] == ')' ) {
						expHolder = expHolder.substr( 0 , expHolder.size() - 1 );
						holder << expHolder;
						holder << " ) ";
					} else {
						holder << expHolder << " ";
					}
				}

				temp = holder.str();
				temp = trim( temp );
				temp = temp.substr( 0 , temp.size() - 1 );
				strcpy( &probArr[ probArrInd ].equation[ 0 ] , trim( temp ).c_str());
				probArr[ probArrInd ].isConcurrent = false;
//				probArr[ probArrInd ].semInd = probArrInd;
				probArrInd++;
			}
		}
	}

/*	for( int i = 0; i < varCtr; i ++ ) {
		cout << varArr[ i ].name << ' ' << endl;
	}
	for( int i = 0; i < probCtr; i ++ ) {
		cout << smProbArr[ i ].name << " = " << smProbArr[ i ].equation << "; " << smProbArr[ i ].isSolved << " "
		     << smProbArr[ i ].isConcurrent << " " << smProbArr[ i ].semInd << endl;
	}*/
	int pid;

	Problem *shm;

	int fd = shm_open( "/cosc728" , O_RDWR | O_CREAT , S_IRUSR | S_IWUSR );
	if( fd == -1 ) {
		cout << "Error opening memory." << endl;
		exit( 1 );
	}
	if( ftruncate( fd , sizeof( struct shmProblem )) == 1 ) {
		cout << "Error setting size." << endl;
		exit( 1 );
	}
	shmProb = ( shmProblem * ) mmap( 0 , sizeof( struct shmProblem ) , PROT_READ | PROT_WRITE , MAP_SHARED , fd , 0 );
	if( shmProb == MAP_FAILED) {
		cout << "Mapping failed." << endl;
		exit( 1 );
	}

	memset( shmProb , 0 , sizeof( shmProblem ));

	for( int i = 0; i < probCtr; i++ ) {
		strcpy( &shmProb->problemArr[ i ].name[ 0 ] , &probArr[ i ].name[ 0 ] );
		strcpy( &shmProb->problemArr[ i ].equation[ 0 ] , &probArr[ i ].equation[ 0 ] );
		shmProb->problemArr[ i ].isConcurrent = probArr[ i ].isConcurrent;
		shmProb->problemArr[ i ].isSolved = probArr[ i ].isSolved;
		shmProb->problemArr[ i ].value = probArr[ i ].value;
	}
	buildDependencies( shmProb );
/*	for(int i = 0; i < probCtr; i++){
		cout << shmProb->problemArr[i].name << endl;
		cout << "\tSignals:" << endl;
		for(int j = 0; j < shmProb->problemArr[i].numSemSignals; j++){
			cout << "\t" << shmProb->problemArr[i].semSignals[j] << " ";
		}
		cout << "\n\tWaits:" << endl;
		for(int j = 0; j < shmProb->problemArr[i].numSemWaits; j++){
			cout << "\t" << shmProb->problemArr[i].semWaits[j] << " ";
		}
		cout << endl;
	}*/

//	for( int i = 0; i < varCtr; i++){
//		cout << varArr[i].name << " " << varArr[i].value << endl;
//	}

	for( int i = 0; i < probCtr; i++ ) {
		if( shmProb->problemArr[ i ].isConcurrent ) {
			pid = fork();
			if( pid == -1 ) {
				cout << "Error creating fork." << endl;
				exit( 1 );
			} else if( pid == 0 ) {
				for( int j = 0; j < shmProb->problemArr[ i ].numSemWaits; j++ ) {
					semWait( shmProb->problemArr[ i ].semWaits[ j ] );
				}

//				cout << "---------- START CRITICAL SECTION FOR " << shmProb->problemArr[ i ].name << " ----------" << endl;
				stringstream ss( shmProb->problemArr[ i ].equation );
				string temp;
				stringstream holder;
//				cout << "\nequation: |" << shmProb->problemArr[ i ].equation << "|" << endl;
				while( ss >> temp ) {
					int operandInd = findProblemInd( shmProb->problemArr , temp );
					int varInd = findVariableInd( varArr , temp );
					if( operandInd >= 0 ) {
						holder << " ";
						holder << shmProb->problemArr[ operandInd ].value;
					} else if( varInd >= 0 ) {
						holder << " ";
						holder << varArr[ varInd ].value;
					} else {
						holder << " " << temp;
					}
				}
				string newEq = holder.str();
				newEq = trim( newEq );
				strcpy( &shmProb->problemArr[ i ].subbedEquation[ 0 ] , newEq.c_str());
//				cout << "Problem " << shmProb->problemArr[ i ].name << ": " << shmProb->problemArr[ i ].equation << "; " << shmProb->problemArr[ i ].subbedEquation << endl;
				char tempChar[200];
				memset( &tempChar[ 0 ] , 0 , sizeof( tempChar ));
				strcpy( &tempChar[ 0 ] , newEq.c_str());
				shmProb->problemArr[ i ].value = Arithmetic::evaluate( newEq );
				shmProb->problemArr[ i ].isSolved = true;
				displayProbArr( shmProb->problemArr , probCtr );
//				cout << "----------- END CRITICAL SECTION FOR " << shmProb->problemArr[ i ].name << " -----------" << endl;
				for( int j = 0; j < shmProb->problemArr[ i ].numSemSignals; j++ ) {
					semSignal( shmProb->problemArr[ i ].semSignals[ j ] );
				}
				exit( 0 );
			}
		}
	}



	/*
	 * Go through problems and look for dependencies
	 * Set up semWaits and semSignals
	 */
	for( int i = 0; i < probCtr; i++ ) {
		if( shmProb->problemArr[ i ].isConcurrent ) {
			continue;
		}
		for( int j = 0; j < shmProb->problemArr[ i ].numSemWaits; j++ ) {
			semWait( shmProb->problemArr[ i ].semWaits[ j ] );
		}
		stringstream ss( shmProb->problemArr[ i ].equation );
		string temp;
		stringstream holder;
//				cout << "\nequation: |" << shmProb->problemArr[ i ].equation << "|" << endl;
//		cout << "test: " << endl;
//		string myString = "-45";
//		int value = atoi(myString.c_str());
//		cout << "\t" << value << endl;
//		cout << "End test" << endl;
		while( ss >> temp ) {
			int operandInd = findProblemInd( shmProb->problemArr , temp );
			int varInd = findVariableInd( varArr , temp );
//			cout << operandInd << " : " << varInd << endl;
			if( operandInd >= 0 ) {
				holder << " ";
				holder << shmProb->problemArr[ operandInd ].value;
			} else if( varInd >= 0 ) {
				holder << " ";
				holder << varArr[ varInd ].value;
			} else {
				holder << " " << temp;
			}
		}
		string newEq = holder.str();
		newEq = trim( newEq );
		strcpy( &shmProb->problemArr[ i ].subbedEquation[ 0 ] , newEq.c_str());
		char tempChar[200];
		memset( &tempChar[ 0 ] , 0 , sizeof( tempChar ));
		strcpy( &tempChar[ 0 ] , newEq.c_str());
		shmProb->problemArr[ i ].value = Arithmetic::evaluate( newEq );
		displayProbArr( shmProb->problemArr , probCtr );
		shmProb->problemArr[ i ].isSolved = true;
		for( int j = 0; j < shmProb->problemArr[ i ].numSemSignals; j++ ) {
			semSignal( shmProb->problemArr[ i ].semSignals[ j ] );
		}
	}

	for( int i = 0; i < shmProb->maxSem; i++ ) {
		semctl( semId , i , IPC_RMID , 0 );
	}
	cout << "deallocated" << endl;
	return 0;
}