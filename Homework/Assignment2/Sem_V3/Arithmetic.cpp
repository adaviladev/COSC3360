//
// Created by hackeysack09 on 11/13/2016.
//

#include <iostream>
#include <sstream>
#include "Arithmetic.h"

using namespace std;

int Arithmetic::addLists( int op1 , int op2 ) {

	return op1 + op2;
}

int Arithmetic::subLists( int op1 , int op2 ) {

	return op1 - op2;
}

int Arithmetic::multLists( int op1 , int op2 ) {

	return op1 * op2;
}

int Arithmetic::divLists( int op1 , int op2 ) {

	return ( op2 != 0 ) ? op1 / op2 : 0;
}

int Arithmetic::checkOpImportance( string op ) {
	if( op == "(" ) {
		return 4;
	} else if( op == ")" ) {
		return 3;
	} else if( op == "*" || op == "/" ) {
		return 2;
	} else if( op == "+" || op == "-" ) {
		return 1;
	} else {
		return - 1;
	}
}

int Arithmetic::parseIntExpression( string str ) {
	int holder;
	stringstream ss( str );
	ss >> holder;

	return holder;
}

int Arithmetic::performOperation( List<string> *stack ) {
	List<int> *holder = new List<int>();
	Node<string> *curNode = stack->head;
	string strVal;
	int intVal = 0;
	int importance;
	while( curNode != NULL ) {
		strVal = curNode->data;
		importance = checkOpImportance( strVal );
		if( importance < 1 ) {
			holder->insertAtEnd(atoi(strVal.c_str()));
		} else {
			int op1 = holder->top()->data;
			holder->pop();
			int op2 = holder->top()->data;
			holder->pop();
			if(strVal == "+"){
				intVal = op2 + op1;
			} else if( strVal == "-" ){
				intVal = op2 - op1;
			} else if( strVal == "*" ){
				intVal = op2 * op1;
			} else if( strVal == "/" ){
				if(op1 == 0){
					intVal = 0;
				} else {
					intVal = op2 / op1;
				}
			}
			holder->insertAtEnd(intVal);
		}
		curNode = curNode->next;
	}

	return intVal;
}

int Arithmetic::evaluate( string expression ) {
	List<string> *opStack = new List<string>(); // used to hold the operators
	List<string> *postFixStack = new List<string>(); // used to hold the postfix expression
	stringstream ss( expression );

	int importance;
	string data;
	string letter;
	while( ss >> letter ) {
		data = letter;
		if( data == "(" ) {
			opStack->insertAtEnd( data );
			continue;
		} else if( data == ")" ) {
			while( opStack->top()->data !=
			       "(" ) {        // keep popping from opStack and pushing to the postFixStack until open paren is found.
				postFixStack->insertAtEnd( opStack->top()->data );
				opStack->pop();
			}
			if( opStack->top() != NULL ) {
				opStack->pop();
			}
			continue;
		}

		importance = checkOpImportance( letter );
		if( importance == - 1 ) {
			postFixStack->insertAtEnd( data );
		} else {
			if( opStack->top() == NULL ) {
				opStack->insertAtEnd( letter );
			} else {
				int impCheck = checkOpImportance( opStack->top()->data );
				Node<string> *curTop = opStack->top();
				while(( curTop != NULL ) && ( curTop->data != "(" ) && ( importance <= impCheck )) {
					postFixStack->insertAtEnd( opStack->top()->data );
					opStack->pop();
					curTop = opStack->top();
					impCheck = checkOpImportance( curTop->data );
				}
				opStack->insertAtEnd( letter );
			}
		}
	}

	while( opStack->top() != NULL ) {
		postFixStack->insertAtEnd( opStack->top()->data );
		opStack->pop();
	}

	int result = performOperation( postFixStack );
	return result;
}
