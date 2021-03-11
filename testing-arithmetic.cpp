#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>
#include <map>
using namespace std;

//////////////////////////
// GLOBAL VARIABLES.... //
//////////////////////////

//keep track of the variables and their corresponding location to their rbp
map<string, string> variableToRbp;
int offset = 1;
int functionOffset = 1;
int stackCount = 0;        //counts # of 4-bit variables (ints) on stack (in the case of exiting the redzone ie. more than 38 variables in test (6 registers + 32 4-bit spaces within redzone))
bool redZoneBreak = false; // if function breaks redzone (more than 128 bytes used), then redZoneBreak = true.
int openBracket = 0;       // keeps track of # of nested statements

//check if there is jump Label
bool outputLabel = false;
bool isFalseResult = false;

//////////////////////////
// FUNCTIONS............//
//////////////////////////

//int test(int a, int b) {
void functionDeclaration(vector<string> &parsedLine)
{
    //this is the epilogue for test(), for when we enter main(). We must print out the epilogue for test() if it exists.
    if (redZoneBreak)
        cout << "popq %rbp" << '\n'
             << "leave" << endl;
    else
        cout << "popq %rbp" << '\n'
             << "ret" << endl;

    redZoneBreak = false; //resets redzone break detection
    functionOffset = 1;   //resets functionOffset
    int words = parsedLine.size();

    //label
    cout << parsedLine[1] << ":" << endl;

    //counts # of variables needed to be stored
    int varCount = 0;
    for (int i = 3; i < words; i++)
    {
        if (parsedLine[i] == "int")
            varCount++;
        else if (parsedLine[i] == "[") //if function parameter is member of an array, not a standalone int
            varCount--;
        else if (parsedLine[i] == "{")
            openBracket++;
    }
    //prologue
    cout << "pushq %rbp" << '\n'
         << "movq %rsp, %rbp" << endl;

    // putting variables in memory
    for (int i = 1; i <= varCount; i++)
    {
        //i <= 6, meaning all ints stored in registers
        if (i == 1)
        {
            cout << "movl   %edi, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
            functionOffset++;
        }
        if (i == 2)
        {
            cout << "movl   %esi, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
            functionOffset++;
        }
        if (i == 3)
        {
            cout << "movl   %edx, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
            functionOffset++;
        }
        if (i == 4)
        {
            cout << "movl   %ecx, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
            functionOffset++;
        }
        if (i == 5)
        {
            cout << "movl   %r8d, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
            functionOffset++;
        }
        if (i == 6)
        {
            cout << "movl   %r9d, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
            functionOffset++;
        }
        //i > 6, meaning some stored on stack. Default (ie. 6 < i < 32) is within redzone, so no need to move rsp.
        if (i == 32)
        { //breaks redzone
            cout << "subq   $4, %rsp," << endl;
            stackCount++;
            redZoneBreak = true;
        }
        if (i > 32)
        { //outside redzone
            cout << "subq   $" << 4 + (4 * stackCount) << ", %rsp" << endl;
            stackCount++;
        }
    }
}

//i = test(a, b, c, d)
void functionCall(vector<string> &parsedLine)
{
}

/**
 * switch statement that uses first element of vector<string> parsedLine to detect statement type (if, for, arithmetic, return, function call)
 *  
*/
void detectStatement(vector<string> &parsedLine)
{
    //check if need to print jumpLabel before instructions
    jumpLabel(outputLabel, isFalseResult);

    switch (parsedLine[0])
    {
    case "if":
        ifStatement(parsedLine);
        break;
    case "for":
        forStatement(parsedLine);
        break;
    case "return":
        returnStatement(parsedLine);
        break;
    case "int":
        variableOrFunctionDec(parsedLine);
        //ie. int a = 0 versus int a(int b, int c)
        // Checking parsedLine[2] for '=' or '(' (variable declaration vs function declaration)
        break;
    case "}":
        closeBracket(parsedLine);
        //this is important for distinguishing the scope of if, for statements as well as function declarations
    default:
        arithmeticOrFunctionCall(parsedLine);
        //ie. i = 2 + 2 vs i = test(a, b, c)
        // in this case, we must check if parsedLine[3] is '(' or '+' '-' '*' (function call vs arithetic)
    }
}

void variableOrFunctionDec(vector<string> &parsedLine)
{
    if (parsedLine[2] == '=')
        variableDeclarations(parsedLine);
    else if (parsedLine[2] == '(')
        functionDeclaration(parsedLine);
    else
        throw std::invalid_argument("parsedLine[2] is not '=' or '('. Unable to understand statement");
}

void closeBracket(vector<string> &parsedLine)
{
    openBracket--;
}

void arithmeticOrFunctionCall(vector<string> &parsedLine)
{
    enum arithmeticSigns('+', '-', '*');

    if (parsedLine[3] == '(')
        functionCall(parsedLine);
    else if (parsedLine[3] == arithmeticSigns)
        arithmeticStatement(parsedLine);
    else
        throw std::invalid_argument("parsedLine[3] is not '(' or '+' , '-', '*'. Unable to understand statement");
}

//movl    value, offset(%rbp)
void variableDeclarations(vector<string> parsedLine)
{
    //int a = 0, b = 0, c = 0;
    for (int i = 1; i < parsedLine.size(); i += 2)
    {
        cout << "movl   " << parsedLine[i + 1] << ", " << -4 * offset << "(%rbp)\n";
        offset++;
    }
}

/*
 When comparison result is false, jump to .false
*/
void ifStatement(vector<string> parsedLine)
{
    checkIfVarType(parsedLine);

    //parsedLine[0] == if, paresdLine[1] == var1, paresdLine[2] == op, paresdLine[3] == var2
    if (parsedLine.size() == 4)
    {

        // a < b
        if (parsedLine[2] == "<")
        {
            cout << "jge     .false" << endl;
        }
        // a > b
        if (parsedLine[2] == ">")
        {
            cout << "jle     .false" << endl;
        }
    }
    //parsedLine[0] == if, paresdLine[1] == var1, paresdLine[2] == op1, paresdLine[3] == "=", pasedLine[4]== var2
    else
    {

        // a <= b
        if (parsedLine[2] == "<")
        {
            cout << "jg    .false" << endl;
        }
        // a >= b
        else if (parsedLine[2] == ">")
        {
            cout << "jl    .false" << endl;
        }
        // a != b
        else if (parsedLine[2] == "!")
        {
            cout << "je     .false" << endl;
        }
        // a == b
        else
        {
            cout << "jne     .false" << endl;
        }
    }

    //note there is a jump label for the next two instruction
    outputLabel = true;
}

//Check each operand of the ifstatement and output corresponding instruction
void checkIfVarType(vector<string> parsedLine)
{

    bool isArrayVar1 = false;
    bool isArrayVar2 = false;
    int var1Index = 0;
    int var2Index = 0;
    string arrayVar1, arrayVar2;

    //parsedLine[1],ParsedLine[3] are operands
    if (parsedLine.size() == 4)
    {
        //check if op1 and op2 are array variables
        for (int i = 0; i < parsedLine[1].size(); i++)
        {
            if (parsedLine[1][i] == '[')
            {
                isArrayVar1 = true;
                var1Index = i + 1;
            }
        }
        for (int j = 0; j < parsedLine[3].size(); j++)
        {
            if (parsedLine[3][j] == '[')
            {
                isArrayVar2 = true;
                var2Index = j + 1;
            }
        }

        //op1 and op2 are not array variables
        if (!isArrayVar1 && !isArrayVar2)
        {
            cout << "movl   " << variableToRbp[parsedLine[1]] << "(%rbp), %eax" << endl;

            //if op2 is a number
            if (isdigit(parsedLine[3]))
            {
                cout << "cmpl   "
                     << "$" << parsedLine[3] << "(%rbp), %eax" << endl;
            }
            else
            {
                cout << "cmpl   " << variableToRbp[parsedLine[3]] << "(%rbp), %eax" << endl;
            }
        }
        //op1 is a array variable, op2 is a number or variable
        else if (isArrayVar1 && !isArrayVar2)
        {
            //arrayVar1 is the index number of the array variable
            for (int i = var1Index; i < parsedLine[1].size() - 1; i++)
            {
                arrayVar1 = arrayVar1 + parsedLine[1][i];
            }

            cout << "movl " << variableToRbp[arrayVar1] << "(%rbp), %eax" << endl;
            cout << "cltp" << endl;
            cout << "movl " << variableToRbp[parsedLine[1]] << "(%rbp,%rax,4), %eax" << endl;

            //if op2 is a number
            if (isdigit(parsedLine[3]))
            {
                cout << "cmpl   "
                     << "$" << parsedLine[3] << "(%rbp), %eax" << endl;
            }
            else
            {
                cout << "cmpl   " << variableToRbp[parsedLine[3]] << "(%rbp), %eax" << endl;
            }
        }
        //op1 and op2 are array variables
        else
        {
            //arrayVar1 is the index number of the array variable 1
            for (int i = var1Index; i < parsedLine[1].size() - 1; i++)
            {
                arrayVar1 = arrayVar1 + parsedLine[1][i];
            }

            //print the array var1
            cout << "movl " << variableToRbp[arrayVar1] << "(%rbp), %eax" << endl;
            cout << "cltp" << endl;
            cout << "movl " << variableToRbp[parsedLine[1]] << "(%rbp,%rax,4), %edx" << endl;

            //arrayVar2 is the index number of the array variable 2
            for (int j = var2Index; i < parsedLine[3].size() - 1; j++)
            {
                arrayVar2 = arrayVar2 + parsedLine[3][j];
            }
            //print array var2
            cout << "movl " << variableToRbp[arrayVar2] << "(%rbp), %eax" << endl;
            cout << "cltp" << endl;
            cout << "movl " << variableToRbp[parsedLine[3]] << "(%rbp,%rax,4), %eax" << endl;
            //compare op1 and op2
            cout << "cmpl %edx, %eax" << endl;
        }
    }

    //parsedLine[1],ParsedLine[4] are operands
    else
    {

        //check if op1 and op2 are array variables
        for (int i = 0; i < parsedLine[1].size(); i++)
        {
            if (parsedLine[1][i] == '[')
            {
                isArrayVar1 = true;
                var1Index = i + 1;
            }
        }
        for (int j = 0; j < parsedLine[4].size(); j++)
        {
            if (parsedLine[4][j] == '[')
            {
                isArrayVar2 = true;
                var2Index = j + 1;
            }
        }

        //op1 and op2 are not array variables
        if (!isArrayVar1 && !isArrayVar2)
        {
            cout << "movl   " << variableToRbp[parsedLine[1]] << "(%rbp), %eax" << endl;

            //if op2 is a number
            if (isdigit(parsedLine[4]))
            {
                cout << "cmpl   "
                     << "$" << parsedLine[4] << "(%rbp), %eax" << endl;
            }
            else
            {
                cout << "cmpl   " << variableToRbp[parsedLine[4]] << "(%rbp), %eax" << endl;
            }
        }
        //op1 is a array variable, op2 is a number or variable
        else if (isArrayVar1 && !isArrayVar2)
        {
            //arrayVar1 is the index number of the array variable
            for (int i = var1Index; i < parsedLine[1].size() - 1; i++)
            {
                arrayVar1 = arrayVar1 + parsedLine[1][i];
            }

            cout << "movl " << variableToRbp[arrayVar1] << "(%rbp), %eax" << endl;
            cout << "cltp" << endl;
            cout << "movl " << variableToRbp[parsedLine[1]] << "(%rbp,%rax,4), %eax" << endl;

            //if op2 is a number
            if (isdigit(parsedLine[4]))
            {
                cout << "cmpl   "
                     << "$" << parsedLine[4] << "(%rbp), %eax" << endl;
            }
            else
            {
                cout << "cmpl   " << variableToRbp[parsedLine[4]] << "(%rbp), %eax" << endl;
            }
        }
        //op1 and op2 are array variables
        else
        {
            //arrayVar1 is the index number of the array variable 1
            for (int i = var1Index; i < parsedLine[1].size() - 1; i++)
            {
                arrayVar1 = arrayVar1 + parsedLine[1][i];
            }

            //print the array var1
            cout << "movl " << variableToRbp[arrayVar1] << "(%rbp), %eax" << endl;
            cout << "cltp" << endl;
            cout << "movl " << variableToRbp[parsedLine[1]] << "(%rbp,%rax,4), %edx" << endl;

            //arrayVar2 is the index number of the array variable 2
            for (int j = var2Index; i < parsedLine[4].size() - 1; j++)
            {
                arrayVar2 = arrayVar2 + parsedLine[4][j];
            }
            //print array var2
            cout << "movl " << variableToRbp[arrayVar2] << "(%rbp), %eax" << endl;
            cout << "cltp" << endl;
            cout << "movl " << variableToRbp[parsedLine[4]] << "(%rbp,%rax,4), %eax" << endl;
            //compare op1 and op2
            cout << "cmpl %edx, %eax" << endl;
        }
    }
}

//print the jumpLabel if there is a false result
void jumpLabel(bool outputLabel, bool isFalseResult)
{

    //output the jumpLabel for the next instruction
    if (outputLabel && !isFalseResult)
    {
        isFalseResult = true;
    }
    //output the jumpLabel
    else if (outputLabel && isFalseResult)
    {
        cout << ".false" << endl;
        outputLabel = false;
        isFalseResult = false;
    }
}

void returnStatement(vector<string> parsedLine)
{
    //return a specific number
    if (isdigit(parsedLine[1]))
    {
        cout << "movl    "
             << "$" << parsedLine[1] << ",%eax" << endl;
    }
    //return the value of variable
    else
    {
        cout << "movl     " << variableToRbp[parsedLine[1]] << "(%rbp)"
             << ",%eax" << endl;
    }
}

/**
 * Splits line into separate words and stores them in vector<string> parsedLine
 * 
*/
void parse(const string &line)
{
    vector<string> parsedLine;
    string word;
    for (const auto &letter : line)
    {

        if (isalpha(letter))
        {
            word += letter;
        }
        else if (isspace(letter) || letter == ';' || letter == '(' || letter == ')')
        {
            if (!word.empty())
                parsedLine.push_back(word);
            word = "";
        }
    }
}

/**
 *Retrieve input codes by line
 *Send the line to parse() to determine its operation 
*/
void readFile(const string &txtFile)
{
    string line;
    ifstream textFile(txtFile);
    while (getline(textFile, line))
    {
        if (line.empty())
            continue;
        parse(line);
    }
}

int main(int argc, char **argv)
{
    // cout << "pushq %rbp" << '\n';
    // cout << "movq %rsp, %rbp" << '\n';
    readFile(argv[1]);

    //epilogue for main()
    if (redZoneBreak)
        cout << "popq %rbp" << '\n'
             << "leave" << endl;
    else
        cout << "popq %rbp" << '\n'
             << "ret" << endl;

    return 0;
}