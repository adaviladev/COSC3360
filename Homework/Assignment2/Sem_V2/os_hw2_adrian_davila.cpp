#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

using namespace std;

int parseInt(string str) {
    int holder;
    stringstream ss(str);
    ss >> holder;

    return holder;
}

int main(int argc, char *argv[]) {
    /*if (!argv[1] || !argv[2]) {
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
    cout << "End data file: " << endl;*/

//    system("bison test.lex");
//    system("g++ lex.yy.c -o lexout");
   /*system("clear");
    system("clear");
    system("bison -y -d calc1.y");
    system("flex calc1.l");
    system("g++ -c y.tab.c lex.yy.c");
    system("g++ lex.yy.o  y.tab.o -o interpret");*/
//    system("./lexOut");

//    cout << "end." << endl;

    system("clear");
    system("clear");
    system("bison -y -d calc1.y");
    system("flex calc1.l");
    system("g++ -c y.tab.c lex.yy.c");
    system("g++ lex.yy.o  y.tab.o -o calc");
//    system("5 + 3\n");
    system("./calc");

    return 0;
}