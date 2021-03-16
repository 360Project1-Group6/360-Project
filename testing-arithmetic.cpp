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

int regCount = 0;          //counts # of registers in use
int redZoneCount = 0;      //counts amount of Red Zone used in units of 4 bytes
bool redZoneBreak = false; // if function breaks redzone (more than 128 bytes used), then redZoneBreak = true.
int openBracket = 0;       // keeps track of # of nested statements

//check if there is jump Label
bool isFalseResult_if = false;
int closeBracket = 0;

//note the start position of comparison in for statement
int numOfCmpl = 0;
bool isCmpl = false;
bool isIncrement = false;
vector<string> incrementStr;
bool isFalseResult_for = false;

//////////////////////////
// FUNCTIONS............//
//////////////////////////

//int test(int a[1], int b, int c[3]) {
void functionDeclaration(vector<string> &parsedLine)
{
    //this is the epilogue for test(), for when we enter main(). We must print out the epilogue for test() if it exists.
    if (redZoneBreak)
        cout << "popq %rbp" << '\n'
             << "leave" << endl;
    else
        cout << "popq %rbp" << '\n'
             << "ret" << endl;

    functionOffset = 1;   //resets functionOffset
    regCount = 0;         //resets registers
    redZoneCount = 0;     //resets redzone data usage counter
    redZoneBreak = false; //resets redzone break detection

    int words = parsedLine.size(); //gives me a warning if I don't do this...?

    //label
    cout << parsedLine[1] << ":" << endl;

    //prologue
    cout << "pushq %rbp" << '\n'
         << "movq %rsp, %rbp" << endl;

    //printing...
    for (int i = 3; i < words; i++)
    {
        if (parsedLine[i] == "int" && parsedLine[i + 2] == "[") //if parameter is member of array...
        {
            cout << "movq   ";
            switch (regCount) //if registers are available
            {
            case 0:
                cout << "%rdi, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 2;
                regCount++;
                break;
            case 1:
                cout << "%rsi, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 2;
                regCount++;
                break;
            case 2:
                cout << "%rdx, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 2;
                regCount++;
                break;
            case 3:
                cout << "%rcx, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 2;
                regCount++;
                break;
            case 4:
                cout << "%r8, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 2;
                regCount++;
                break;
            case 5:
                cout << "%r9, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 2;
                regCount++;
                break;
            default:
                redZoneCount += 2;
                break;
            }
            i += 4; //skip to next parameter
        }
        else if (parsedLine[i] == "int") //parameter is a regular int variable
        {
            cout << "movl   ";
            switch (regCount) //if registers are available
            {
            case 0:
                cout << "%edi, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 1;
                regCount++;
                break;
            case 1:
                cout << "%esi, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 1;
                regCount++;
                break;
            case 2:
                cout << "%edx, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 1;
                regCount++;
                break;
            case 3:
                cout << "%ecx, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 1;
                regCount++;
                break;
            case 4:
                cout << "%r8d, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 1;
                regCount++;
                break;
            case 5:
                cout << "%r9d, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 1;
                regCount++;
                break;
            default:
                redZoneCount += 1;
                break;
            }
            i++; //skip to next parameter
        }
        else if (parsedLine[i] == "{") //open bracket
            openBracket++;
    }

    if (redZoneCount > 32) //if function exits redzone, need to move %rsp
    {
        redZoneBreak = true;
        cout << "subq   $" << 4 + (4 * (redZoneCount - 32)) << ", %rsp" << endl;
    }
}

//i = test(a, b, c, d)
// if parameter is int, use movl X(%register). if it is array member, use leaq Y(%register), where Y is lowest offset in array (ie offset of first element in array)
void functionCall(vector<string> &parsedLine)
{
    int functCallRegCounter = 0;     //counts # of registers used in function call
    int functCallRedZoneCounter = 0; //counts # of 8-byte (quadword) units needed for addq $X, %rsp after registers are filled
    int words = parsedLine.size();
    for (int i = 4; i < words; i++)
    {
        if (variableToRbp.count(parsedLine[i] + '0') > 0) //for array members; if variableToRbp[n0] exists then....
        {
            cout << "leaq   ";
            cout << variableToRbp[(parsedLine[i] + '0')]; //address of first member of array
            switch (functCallRegCounter)
            {
            case 0:
                cout << ", %r9" << endl;
                functCallRegCounter++;
            case 1:
                cout << ", %r8" << endl;
                functCallRegCounter++;
            case 2:
                cout << ", %rcx" << endl;
                functCallRegCounter++;
            case 3:
                cout << ", %rdx" << endl;
                functCallRegCounter++;
            case 4:
                cout << ", %rsi" << endl;
                functCallRegCounter++;
            case 5:
                cout << ", %rax" << endl; //temporary
                functCallRegCounter++;
            default:
                cout << ", %rdi" << endl;
                cout << "pushq   %rdi" << endl; //pushes additional variables to stack (always the whole quadword!)
                functCallRedZoneCounter++;
            }
            cout << "movq   %rax, %rdi" << endl; //prepare for function call to return to %edi (%rdi contains %edi)
        }

        else if (variableToRbp.count(parsedLine[i]) > 0) //for int parameters
        {
            cout << "movl   ";
            cout << variableToRbp[(parsedLine[i])]; //address of int
            switch (functCallRegCounter)
            {
            case 0:
                cout << ", %r9d" << endl;
                functCallRegCounter++;
            case 1:
                cout << ", %r8d" << endl;
                functCallRegCounter++;
            case 2:
                cout << ", %ecx" << endl;
                functCallRegCounter++;
            case 3:
                cout << ", %edx" << endl;
                functCallRegCounter++;
            case 4:
                cout << ", %esi" << endl;
                functCallRegCounter++;
            case 5:
                cout << ", %eax" << endl; //temporary
                functCallRegCounter++;
            default:
                cout << ", %edi" << endl;
                cout << "pushq   %rdi" << endl; //pushes additional variables to stack (the WHOLE quadword!)
                functCallRedZoneCounter++;
            }
            cout << "movq   %eax, %edi" << endl; //prepare for function call to return to %edi
        }
    }
    //begin function call
    cout << "call   " << parsedLine[2] << endl;
    if (functCallRedZoneCounter > 0)
        cout << "addq   $" << functCallRedZoneCounter * 8 << ", %rsp" << endl; //rsp shift
    cout << "movl   %eax, " << variableToRbp[0] << endl;                       //assigns return of function to variable
}

/**
 * switch statement that uses first element of vector<string> parsedLine to detect statement type (if, for, arithmetic, return, function call)
 *  
*/
void detectStatement(vector<string> &parsedLine)
{

    findCloseBracket(parsedLine);
    //check if need a jumpLabel before output instructions
    //print the jumpLabel for the ifstatement when there is a false result
    if (isFalseResult_if && (closeBracket > 0))
    {
        cout << ".False_if" << endl;
        isFalseResult_if = false;
        closeBracket--;
    }

    //check if need a comparison label for the forStatement
    if (isCmpl)
    {
        compare_for(parsedLine);
        isCmpl = false;
        break;
    }

    if (isIncrement)
    {
        increment(parsedLine);
        isIncrement = false;
        break;
    }

    if (isFalseResult_for && (closeBracket > 0))
    {
        cout << incrementStr[numOfCmpl - 1] << endl;
        cout << "jmp .compare" << numOfCmpl << endl;
        cout << ".false_for" << numOfCmpl << endl;

        numOfCmpl--;
        closeBracket--;

        if (numOfCmpl == 0)
        {
            isFalseResult_for = "false";
        }
    }

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
    if (parsedLine[2] == "=")
        variableDeclarations(parsedLine);
    else if (parsedLine[2] == "(")
        functionDeclaration(parsedLine);
    else
        throw std::invalid_argument("parsedLine[2] is not '=' or '('. Unable to understand statement");
}

void closeBracket(vector<string> &parsedLine)
{
    openBracket--;
}

void findCloseBracket(vector<string> parsedLine)
{
    for (int i = 0; i < parsedLine.size(); i++)
    {
        if (parsedLine[i] == "}")
        {
            closeBracket++;
        }
    }
}

void arithmeticOrFunctionCall(vector<string> &parsedLine)
{
    enum arithmeticSigns('+', '-', '*');

    if (parsedLine[3] == "(")
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
If Statement only cotains two conditions: a > b and a < b
When comparison result is false, jump to .false_if
*/
void ifStatement(vector<string> parsedLine)
{

    //check the variable type of a and b, print related instructions
    checkIfVarType(parsedLine);

    //parsedLine[0] == if, paresdLine[1] == op1, paresdLine[2] == comparator, paresdLine[3] == op2

    // a < b
    if (parsedLine[2] == "<")
    {
        cout << "jge .false_if" << endl;
    }
    // a > b
    if (parsedLine[2] == ">")
    {
        cout << "jle .false_if" << endl;
    }

    //note there is a jump label for the next two instruction
    isFalseResult_if = true;
}

//Check each operand type in the ifstatement and output corresponding instruction
void checkIfVarType(vector<string> parsedLine)
{

    bool isArrayVar1 = false;
    bool isArrayVar2 = false;
    int var1Index = 0;
    int var2Index = 0;
    string arrayVar1 = "";
    string arrayVar2 = "";
    string arrayVar1_index = "";
    string arrayVar2_index = "";

    //parsedLine[1],ParsedLine[3] are operands

    //check if op1 is a array variables
    for (int i = 0; i < parsedLine[1].size(); i++)
    {
        //note there is a array variable, store its index position
        if (parsedLine[1][i] == '[')
        {
            isArrayVar1 = true;
            var1Index = i + 1;
        }
        //remove [] and store the array var string
        if (parsedLine[1][i] != '[' || parsedLine[1][i] != ']')
        {
            arrayVar1 += parsedLine[1][i];
        }
    }

    //check if op2 is a array variable
    for (int j = 0; j < parsedLine[3].size(); j++)
    {
        //note there is a array variable, store its index position
        if (parsedLine[3][j] == '[')
        {
            isArrayVar2 = true;
            var2Index = j + 1;
        }
        //remove [] and store the array var string
        if (parsedLine[3][j] != '[' || parsedLine[3][j] != ']')
        {
            arrayVar2 += parsedLine[3][j];
        }
    }

    //op1 and op2 are not array variables
    if (!isArrayVar1 && !isArrayVar2)
    {
        cout << "movl " << variableToRbp[parsedLine[1]] << "(%rbp), %eax" << endl;

        //if op2 is a number
        if (isdigit(parsedLine[3][0]))
        {
            cout << "cmpl "
                 << "$" << parsedLine[3] << "(%rbp), %eax" << endl;
        }
        else
        {
            cout << "cmpl " << variableToRbp[parsedLine[3]] << "(%rbp), %eax" << endl;
        }
    }

    //op1 is a array variable, op2 is a number or variable
    else if (isArrayVar1 && !isArrayVar2)
    {
        //arrayVar1 is the index number of the array variable
        for (int i = var1Index; i < parsedLine[1].size() - 1; i++)
        {
            arrayVar1_index += parsedLine[1][i];
        }

        cout << "movl " << variableToRbp[arrayVar1_index] << "(%rbp), %eax" << endl;
        cout << "cltp" << endl;
        cout << "movl " << variableToRbp[arrayVar1] << "(%rbp,%rax,4), %eax" << endl;

        //if op2 is a number
        if (isdigit(parsedLine[3][0]))
        {
            cout << "cmpl "
                 << "$" << parsedLine[3] << "(%rbp), %eax" << endl;
        }
        else
        {
            cout << "cmpl " << variableToRbp[parsedLine[3]] << "(%rbp), %eax" << endl;
        }
    }

    //op1 and op2 are both array variables
    else
    {
        //arrayVar1 is the index number of the array variable 1
        for (int i = var1Index; i < parsedLine[1].size() - 1; i++)
        {
            arrayVar1_index += parsedLine[1][i];
        }

        //print the array var1
        cout << "movl " << variableToRbp[arrayVar1_index] << "(%rbp), %eax" << endl;
        cout << "cltp" << endl;
        cout << "movl " << variableToRbp[arrayVar1] << "(%rbp,%rax,4), %edx" << endl;

        //arrayVar2 is the index number of the array variable 2
        for (int j = var2Index; j < parsedLine[3].size() - 1; j++)
        {
            arrayVar2_index += parsedLine[3][j];
        }

        //print array var2
        cout << "movl " << variableToRbp[arrayVar2_index] << "(%rbp), %eax" << endl;
        cout << "cltp" << endl;
        cout << "movl " << variableToRbp[arrayVar2] << "(%rbp,%rax,4), %eax" << endl;

        //compare op1 and op2
        cout << "cmpl %edx, %eax" << endl;
    }
}

// //print the jumpLabel for the ifstatement when there is a false result
// void jumpLabel(bool outputLabel, bool isFalseResult)
// {

//     //output the jumpLabel for the next instruction
//     if (!outputLabel && isFalseResult)
//     {
//         outputLabel = true;
//     }
//     //output the jumpLabel
//     else if (outputLabel && isFalseResult)
//     {
//         cout << ".false_if" << endl;
//         outputLabel = false;
//         isFalseResult = false;
//     }
// }

void forStatement(vector<string> parsedLine)
{

    //for int i = 0;
    if (parsedLine.size() == 4)
    {
        cout << "movl " << '$' << parsedLine[4] << ", " << variableToRbp[parsedLine[2]] << "(%rbp)" << endl;
    }
    //j = i + 1
    else
    {
        cout << "movl " << variableToRbp[parsedLine[2]] << "(%rbp), %eax" << endl;
        cout << "addl $" << parsedLine[4] << ", %eax" << endl;
        cout << "movl "
             << "%eax, " << variableToRbp[parsedLine[0]] << "(%rbp)" << endl;
    }

    numOfCmpl++;

    cout << ".compare" << numOfCmpl << endl;

    isCmpl = true;
}

/*

*/
void compare_for(vector<string> parsedLine)
{

    cout << "movl " << variableToRbp[parsedLine[0]] << ", %eax" << endl;
    cout << "cmpl "
         << "$" << parsedLine[2] << ", %eax" << endl;

    if (parsedLine[1] == "<")
    {
        cout << "jge .false_for" << numOfCmpl << endl;
    }
    else
    {
        cout << "jle .false_for" << numOfCmpl << endl;
    }
    isIncrement = true;
}

void increment(vector<string> parsedLine)
{
    //a++
    string temp = "addl $1, " + variableToRbp[parsedLine[0]];
    incrementStr.push_back(temp);
    isFalseResult_for = true;
}

void returnStatement(vector<string> parsedLine)
{
    //return a specific number
    if (isdigit(parsedLine[1][0]))
    {
        cout << "movl "
             << "$" << parsedLine[1] << ", %eax" << endl;
    }
    //return the value of variable
    else
    {
        cout << "movl " << variableToRbp[parsedLine[1]] << "(%rbp)"
             << ", %eax" << endl;
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