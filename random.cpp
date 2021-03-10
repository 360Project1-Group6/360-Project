/*
PROJECT 1 for CSCI36000 under the instruction of Xiaojie Zhang for CUNY Hunter College
Date: March 3rd, 2021
By: Zhi Dong Dong, Wen Bo, Shinnosuke Takahashi (Group 6)

This program takes a txt file with basic arithmetic functions in C++ as input, and outputs an x86 translation to the screen
*/

#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

void variableDeclaration(string &code, int loc_rbp) //eg. int a = 0 , b = 1;
{
}

void ifStatement(); //eg. if (a >= 10);

void forStatement(); //eg. for (int i = 0; i < 5; i++);

void arithStatement(); //eg. a = b + 1;

void retStatement(); //eg. return a;

void callStatement(); //eg, foo(a);

//readFile function return the code string by using input file name
void readFile(const string &fileName, string &code)
{
    string readline, testStr;
    ifstream infile(fileName);

    while (getline(infile, readline))
    {
        cout << readline << endl; //outputs given code to screen
        testStr = readline;
        //get the return string
        code += readline;
    }

    infile.close();
}

int main(int argc, char **argv)
{
    if (argc != 2)
        throw std::invalid_argument("Please compile with file name");

    //start of translation below...
    bool redZoneBreak = false; //this memos whether or not we have moved beyond red zone, meaning that we have to end with "leave" instead of "popq %rbp"

    //translation header
    cout << "pushq %rbp" << endl;
    cout << "movq %rsp, %rbp" << endl;

    //code parsing here
    string code = "";
    readFile(argv[1], code);

    //translation footer
    if (redZoneBreak)
        cout << "leave" << '\n'
             << "ret" << endl;
    else
        cout << "popq %rbp" << '\n'
             << "ret" << endl;

    return 0;
}
