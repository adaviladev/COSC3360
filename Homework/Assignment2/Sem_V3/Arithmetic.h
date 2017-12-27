#ifndef SEM_V3_ARITHMETIC_H
#define SEM_V3_ARITHMETIC_H

#include <string>
#include "List.h"

class Arithmetic {
	static int addLists(int op1, int op2);
	static int subLists(int op1, int op2);
	static int multLists(int op1, int op2);
	static int divLists(int op1, int op2);
	static int parseIntExpression( string str );
	static int checkOpImportance( string op );
public:

	static int evaluate(string);
	static int performOperation( List<string> *stack );
};


#endif //SEM_V3_ARITHMETIC_H
