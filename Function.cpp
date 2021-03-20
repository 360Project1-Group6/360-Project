#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>
#include <map>
#include <algorithm>
using namespace std;

//keep track of the varibles and their corresponding location to their rbp
map<string, string> variableToRbp;
int offset = 1;
string comparator;

//Tracking the # of comparison in for loop
int numOfCmpl = 0;

//store the increment instruction before print the false label
vector<string> incrementStr;

//check if there is a false result label
bool isFalseResult_for = false;

//check if the comparison is finished
bool endCmpl = false;
bool endFor = false;

//check if there is jump Label
bool isFalseResult_if = false;
bool endIf = false;

int functionOffset = 1;    //counts offset within function
int regCount = 0;          //counts # of registers in use
int redZoneCount = 0;      //counts amount of Red Zone used in units of 4 bytes
bool redZoneBreak = false; // if function breaks redzone (more than 128 bytes used), then redZoneBreak = true.
int openBracket = 0;       // keeps track of # of nested statements
int functCount = 0;        // keeps track of how many functions exist besides main()
int leafRegCount = 0;      //counts # of registers used in leaf function (if applicable)

//////////////////////////
// FUNCTIONS............//
//////////////////////////

//int test(int a, int b, int c, int d, int e[3], int f, int g, int h[2]){
//int main(){
void functionDeclaration(vector<string> &parsedLine)
{
    //this is the epilogue for test(), for when we enter main(). We must print out the epilogue for test() if it exists.
    if (redZoneBreak && functCount > 0)
        cout << "popq   %rbp" << '\n'
             << "leave" << endl;
    else if (functCount > 0)
        cout << "popq   %rbp" << '\n'
             << "ret" << endl;

    //label
    cout << parsedLine[1] << ":" << endl;

    //prologue
    cout << "pushq   %rbp" << '\n'
         << "movq   %rsp, %rbp" << endl;

    if (regCount == 5) //this is for main() in the case of existence of leaf functions (test1)
        cout << "subq   $64, %rsp" << endl;

    leafRegCount = regCount; //stores regCount for previous function

    functionOffset = 1;   //resets functionOffset
    regCount = 0;         //resets registers
    redZoneCount = 0;     //resets redzone data usage counter
    redZoneBreak = false; //resets redzone break detection
    functCount++;         //function counter updated

    int words = parsedLine.size(); //gives me a warning if I don't do this...?

    //printing...
    for (int i = 3; i < words; i++)
    {
        if (parsedLine[i] == "int" && parsedLine[i + 2] == "[") //if parameter is member of array...
        {
            switch (regCount) //if registers are available
            {
            case 0:
                functionOffset++;
                cout << "movq   %rdi, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset++;
                regCount++;
                break;
            case 1:
                functionOffset++;
                cout << "movq   %rsi, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset++;
                regCount++;
                break;
            case 2:
                functionOffset++;
                cout << "movq   %rdx, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset++;
                regCount++;
                break;
            case 3:
                functionOffset++;
                cout << "movq   %rcx, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset++;
                regCount++;
                break;
            case 4:
                functionOffset++;
                cout << "movq    %r8, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset++;
                regCount++;
                break;
            case 5:
                functionOffset++;
                cout << "movq    %r9, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset++;
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
            switch (regCount) //if registers are available
            {
            case 0:
                cout << "movl   %edi, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 1;
                regCount++;
                break;
            case 1:
                cout << "movl   %esi, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 1;
                regCount++;
                break;
            case 2:
                cout << "movl   %edx, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 1;
                regCount++;
                break;
            case 3:
                cout << "movl   %ecx, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 1;
                regCount++;
                break;
            case 4:
                cout << "movl   %r8d, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
                functionOffset += 1;
                regCount++;
                break;
            case 5:
                cout << "movl   %r9d, " << -4 * functionOffset - 16 << "(%rbp)" << endl;
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

//MUST REVERSE ORDER ie. load f then e then d then c then b then a
//i = test(a,b,c,d,e,f,g,h)
// if parameter is int, use movl X(%register). if it is array member, use leaq Y(%register), where Y is lowest offset in array (ie offset of first element in array)
void functionCall(vector<string> &parsedLine)
{
    int functCallRegCounter = 0;     //counts # of registers used in function call
    int functCallRedZoneCounter = 0; //counts # of 8-byte (quadword) units needed for addq $X, %rsp after registers are filled

    string outputString;     //will print this to screen (registers)
    string postOutputString; //will print this to screen (do not fit in registers)

    int words = parsedLine.size();
    for (int i = 3; i < words; i++)
    {
        if (variableToRbp.count(parsedLine[i] + '0') > 0) //for array members; if variableToRbp[n0] exists then....
        {
            switch (functCallRegCounter)
            {
            case 5:
                outputString.insert(0, ("leaq   " + variableToRbp[(parsedLine[i] + '0')] + ", %r9" + '\n'));
                functCallRegCounter++;
                break;
            case 4:
                outputString.insert(0, ("leaq   " + variableToRbp[(parsedLine[i] + '0')] + ", %r8" + '\n'));
                functCallRegCounter++;
                break;
            case 3:
                outputString.insert(0, ("leaq   " + variableToRbp[(parsedLine[i] + '0')] + ", %rcx" + '\n'));
                functCallRegCounter++;
                break;
            case 2:
                outputString.insert(0, ("leaq   " + variableToRbp[(parsedLine[i] + '0')] + ", %rdx" + '\n'));
                functCallRegCounter++;
                break;
            case 1:
                outputString.insert(0, ("leaq   " + variableToRbp[(parsedLine[i] + '0')] + ", %rsi" + '\n'));
                functCallRegCounter++;
                break;
            case 0:
                outputString.insert(0, ("leaq   " + variableToRbp[(parsedLine[i] + '0')] + ", %rax" + '\n'));
                functCallRegCounter++;
                break;
            default:
                postOutputString.insert(0, ("leaq   " + variableToRbp[(parsedLine[i] + '0')] + ", %rdi" + '\n' + "pushq   %rdi" + '\n'));
                functCallRedZoneCounter++;
                break;
            }
        }

        else if (variableToRbp.count(parsedLine[i]) > 0) //for int parameters
        {
            switch (functCallRegCounter)
            {
            case 5:
                outputString.insert(0, ("movl   " + variableToRbp[parsedLine[i]] + ", %r9d" + '\n'));
                functCallRegCounter++;
                break;
            case 4:
                outputString.insert(0, ("movl   " + variableToRbp[parsedLine[i]] + ", %r8d" + '\n'));
                functCallRegCounter++;
                break;
            case 3:
                outputString.insert(0, ("movl   " + variableToRbp[parsedLine[i]] + ", %ecx" + '\n'));
                functCallRegCounter++;
                break;
            case 2:
                outputString.insert(0, ("movl   " + variableToRbp[parsedLine[i]] + ", %edx" + '\n'));
                functCallRegCounter++;
                break;
            case 1:
                outputString.insert(0, ("movl   " + variableToRbp[parsedLine[i]] + ", %esi" + '\n'));
                functCallRegCounter++;
                break;
            case 0:
                outputString.insert(0, ("movl   " + variableToRbp[parsedLine[i]] + ", %eax" + '\n'));
                functCallRegCounter++;
                break;
            default:
                postOutputString.insert(0, ("movl   " + variableToRbp[parsedLine[i]] + ", %edi" + '\n' + "pushq   %rdi" + '\n'));
                functCallRedZoneCounter++;
                break;
            }
        }
    }
    cout << outputString;
    outputString = "";
    cout << postOutputString;
    postOutputString = "";
    cout << "movq   %eax, %edi" << endl; //prepare for function call to return to %eax
    //begin function call
    cout << "call   " << parsedLine[2] << endl;
    if (functCallRedZoneCounter > 0)
        cout << "addq   $" << functCallRedZoneCounter * 8 << ", %rsp" << endl; //rsp shift
    cout << "movl   %eax, " << variableToRbp[parsedLine[0]] << endl;           //assigns return of function to variable
}

/*
* When isFalseResult_if is true
* if there is a close bracket, record the position of ending if statement
*/
void ifEnd(bool isFalseResult_if, vector<string> parsedLine)
{
    if (isFalseResult_if)
    {
        for (int i = 0; i < parsedLine.size(); i++)
        {
            if (parsedLine[i] == "}")
            {
                endIf = true;
            }
        }
    }
}

/*
* When endCmpl is true
* if there is a close bracket, record the position of end
*/
void cmplEnd(bool endCmpl, vector<string> parsedLine)
{
    if (endCmpl)
    {
        for (int i = 0; i < parsedLine.size(); i++)
        {
            if (parsedLine[i] == "}")
            {
                endFor = true;
            }
        }
    }
}

//Check each operand type in the ifstatement and output corresponding instruction
void checkIfVarType(vector<string> parsedLine)
{

    bool isArrayVar1 = false;
    bool isArrayVar2 = false;
    int numOfArr = 0;
    string var1Index;
    string var2Index;
    string arrayVar1 = "";
    string arrayVar2 = "";

    for (int i = 0; i < parsedLine.size(); i++)
    {
        //check if op1 is a array variables
        if (parsedLine[i] == "[" && !isArrayVar1)
        {
            isArrayVar1 = true;
            var1Index = parsedLine[i + 1];
            arrayVar1 = parsedLine[i - 1];
        }
        //check if op2 is a array variables
        else if (parsedLine[i] == "[" && isArrayVar1)
        {
            isArrayVar2 = true;
            var2Index = parsedLine[i + 1];
            arrayVar2 = parsedLine[i - 1];
        }
    }

    //op1 and op2 are not array variables
    if (!isArrayVar1 && !isArrayVar2)
    {
        cout << "movl " << variableToRbp[parsedLine[2]] << ", %eax" << endl;

        //if op2 is a number
        if (isdigit(parsedLine[3][0]))
        {
            cout << "cmpl "
                 << "$" << parsedLine[3] << ", %eax" << endl;
        }
        else
        {
            cout << "cmpl " << variableToRbp[parsedLine[3]] << ", %eax" << endl;
        }
    }

    //op1 is a array variable, op2 is a number or variable
    else if (isArrayVar1 && !isArrayVar2)
    {

        cout << "movl " << variableToRbp[var1Index] << ", %eax" << endl;
        cout << "cltq" << endl;
        cout << "movl " << variableToRbp[arrayVar1 + "counter"] << "(%rbp,%rax,4), %eax" << endl;

        //if op2 is a number
        if (isdigit(parsedLine[3][0]))
        {
            cout << "cmpl "
                 << "$" << parsedLine[3] << ", %eax" << endl;
        }
        else
        {
            cout << "cmpl " << variableToRbp[parsedLine[3]] << ", %eax" << endl;
        }
    }

    //op1 and op2 are both array variables
    else
    {

        //print the array var1
        cout << "movl " << variableToRbp[var1Index] << ", %eax" << endl;
        cout << "cltq" << endl;
        cout << "movl " << variableToRbp[arrayVar1 + "counter"] << "(%rbp,%rax,4), %edx" << endl;

        //print array var2
        cout << "movl " << variableToRbp[var2Index] << ", %eax" << endl;
        cout << "cltq" << endl;
        cout << "movl " << variableToRbp[arrayVar2 + "counter"] << "(%rbp,%rax,4), %eax" << endl;

        //compare op1 and op2
        cout << "cmpl %edx, %eax" << endl;
    }
}

void ifStatement(vector<string> parsedLine)
{

    //check the variable type of a and b, print related instructions
    checkIfVarType(parsedLine);

    //parsedLine[0] == if, paresdLine[1] == op1, paresdLine[2] == comparator, paresdLine[3] == op2

    // a < b
    if (comparator == "<")
    {
        cout << "jge .false_if" << endl;
    }
    // a > b
    if (comparator == ">")
    {
        cout << "jle .false_if" << endl;
    }
    // a >= b
    if (comparator == ">=")
    {
        cout << "jl .false_if" << endl;
    }
    // a <= b
    if (comparator == "<=")
    {
        cout << "jg .false_if" << endl;
    }
    // a == b
    if (comparator == "==")
    {
        cout << "jne .false_if" << endl;
    }
    // a != b
    if (comparator == "!=")
    {
        cout << "je .false_if" << endl;
    }

    //note there is a jump label for the next two instruction
    isFalseResult_if = true;
}

void iterateMap()
{
    for (const auto &x : variableToRbp)
    {
        cout << "key: " << x.first << " Value: " << x.second << ' ';
    }
    cout << '\n';
}

/**
 * Cases Considered: 
 * a = b;
 * a = b + c;
 * a = b - c;
 * a = b * c;
*/
void arithmeticOperation(vector<string> parsedLine)
{
    std::string opCode;
    parsedLine[3] == "+" ? opCode = "addl" : opCode = "subl";
    if (parsedLine.size() < 4)
    {
        cout << "movl " << variableToRbp[parsedLine[2]] << ", %eax\n";
        cout << "movl %eax, " << variableToRbp[parsedLine[0]] << "\n";
    }
    else if (parsedLine[3] == "*")
    {
        cout << "movl " << variableToRbp[parsedLine[2]] << ", %eax\n";
        cout << "imull " << variableToRbp[parsedLine[4]] << ", %eax\n";
        cout << "movl "
             << "%eax, " << variableToRbp[parsedLine[0]] << '\n';
        return;
    }
    else
    {
        if (isdigit(parsedLine[4][0]))
        {
            cout << "movl " << variableToRbp[parsedLine[2]] << ", %eax\n";
            cout << opCode << " $" << parsedLine[4] << ", %eax\n";
            cout << "movl "
                 << "%eax, " << variableToRbp[parsedLine[0]] << '\n';
        }
        else
        {
            cout << "movl " << variableToRbp[parsedLine[2]] << ", %eax\n";
            cout << "movl " << variableToRbp[parsedLine[4]] << ", %edx\n";
            cout << opCode << " %edx, %eax\n";
            cout << "movl "
                 << "%eax, " << variableToRbp[parsedLine[0]] << '\n';
        }
    }
}

bool isAdd(vector<string> parsedLine)
{
    for (const auto x : parsedLine)
    {
        if (x == "+")
            return true;
    }
    return false;
}

/**
 * Cases Considered: 
 * c[a] = c[b];
 * a = a + d[b];
 * a = d[b] + a;
 * a = a + d[num];
 * a = d[num] + a;
 * a = d[b]
 *      a[min_inx] = a[i];
        a[i] = temp;
*/
void arithOpWithArr(vector<string> parsedLine)
{
    std::string opCode;
    isAdd(parsedLine) == true ? opCode = "addl" : opCode = "subl";

    if (parsedLine.size() == 6 && parsedLine[1] == "[" && !isdigit(parsedLine[5][0]))
    {
        cout << "movl " << variableToRbp[parsedLine[2]] << ", %eax\n";
        cout << "cltq\n";
        cout << "movl " << variableToRbp[parsedLine[5]] << ", %edx\n";
        cout << "movl %edx, " << variableToRbp[parsedLine[0] + "counter"] << "(%rbp,%rax,4), %edx\n";
    }
    else if (parsedLine.size() == 7 && parsedLine[4] == "[")
    {
        cout << "movl " << variableToRbp[parsedLine[5]] << ", %eax\n";
        cout << "cltq\n";
        cout << "movl " << variableToRbp[parsedLine[3] + "counter"] << "(%rbp,%rax,4), %edx\n";
        cout << "movl %eax"
             << ", " << -4 * offset << "(%rbp)\n";
        variableToRbp[parsedLine[1]] = to_string(-4 * offset) + "(%rbp)";
        offset++;
    }
    //c[a] = c[b];
    else if ((parsedLine[1] == "[" && !isdigit(parsedLine[2][0])) && (parsedLine[6] == "[" && !isdigit(parsedLine[7][0]) && parsedLine.size() < 10))
    {
        cout << "movl " << variableToRbp[parsedLine[7]] << ", %eax\n";
        cout << "cltq\n";
        cout << "movl " << variableToRbp[parsedLine[5] + "counter"] << "(%rbp,%rax,4), %edx\n";
        cout << "movl " << variableToRbp[parsedLine[2]] << ", %eax\n";
        cout << "cltq\n";
        cout << "%edx, " << variableToRbp[parsedLine[0] + "counter"] << "(%rbp,%rax,4)\n";
    }
    //a = a + d[b];
    else if (parsedLine[5] == "[" && !isdigit(parsedLine[6][0]))
    {
        cout << "movl " << variableToRbp[parsedLine[6]] << ", %eax\n";
        cout << "cltq\n";
        cout << "movl " << variableToRbp[parsedLine[4] + "counter"] << "(%rbp,%rax,4), %edx\n";
        cout << "movl " << variableToRbp[parsedLine[2]] << ", %eax\n";
        cout << opCode << " %edx, %eax\n";
        cout << "movl "
             << "%eax," << variableToRbp[parsedLine[0]] << '\n';
    }
    //a = d[b] + a;
    else if (parsedLine[3] == "[" && !isdigit(parsedLine[4][0]))
    {
        cout << "movl " << variableToRbp[parsedLine[4]] << ", %eax\n";
        cout << "cltq\n";
        cout << "movl " << variableToRbp[parsedLine[2] + "counter"] << "(%rbp,%rax,4), %eax\n";
        if (parsedLine.size() > 6)
        {
            cout << "movl " << variableToRbp[parsedLine[7]] << ", %edx\n";
            cout << opCode << " %edx, %eax\n";
        }
        cout << "movl "
             << "%eax," << variableToRbp[parsedLine[0]] << '\n';
    }
    //a = a + d[num];
    else if (parsedLine[5] == "[")
    {
        cout << "movl " << variableToRbp[parsedLine[4] + parsedLine[6]] << ", %edx\n";
        cout << "movl " << variableToRbp[parsedLine[2]] << ", %eax\n";
        cout << opCode << " %edx, %eax\n";
        cout << "movl "
             << "%eax," << variableToRbp[parsedLine[0]] << '\n';
    }
    //a = d[num] + a;
    else if (parsedLine[3] == "[")
    {
        cout << "movl " << variableToRbp[parsedLine[2] + parsedLine[4]] << ", %eax\n";
        cout << "movl " << variableToRbp[parsedLine[7]] << ", %edx\n";
        cout << opCode << " %edx, %eax\n";
        cout << "movl "
             << "%eax," << variableToRbp[parsedLine[0]] << '\n';
    }
}

//returns a vector containing only alphanumeric values
vector<string> filterVec(vector<string> parsedLine)
{
    vector<string> temp;
    for (const auto &word : parsedLine)
    {
        if (isalnum(word[0]))
        {
            temp.push_back(word);
        }
    }
    return temp;
}

void variableDeclarations(vector<string> parsedLine)
{
    std::string opCode;
    isAdd(parsedLine) == true ? opCode = "addl" : opCode = "subl";

    if (parsedLine.size() > 5 && isdigit(parsedLine[5][0]))
    {
        cout << "movl " << variableToRbp[parsedLine[3]] << ", %eax\n";
        cout << opCode << " $" << parsedLine[5] << ", %eax\n";
        cout << "movl %eax"
             << ", " << -4 * offset << "(%rbp)\n";
        variableToRbp[parsedLine[1]] = to_string(-4 * offset) + "(%rbp)";
        offset++;
    }
    else
    {
        vector<string> temp = filterVec(parsedLine);
        for (int i = 1; i < temp.size(); i += 2)
        {
            cout << "movl $" << temp[i + 1] << ", " << -4 * offset << "(%rbp)\n";
            variableToRbp[temp[i]] = to_string(-4 * offset) + "(%rbp)";
            offset++;
        }
    }
}

/**
 * Maps array[index]: value in variableToRbp
 * Access elements through concatenating array name + index
 * E.g., To assign the value of array A at index 0 - 
 * you would do something like int b = variableToRbp[A0];
*/
void arrayDeclartion(vector<string> parsedLine)
{
    //subtract 1 b/c address at current offset not yet used
    //offset starts at 1
    //int d[4] = {1,4,6,7};
    offset += stoi(parsedLine[3]) - 1;
    vector<string> temp = filterVec(parsedLine);
    int tempOffset = offset;
    variableToRbp[parsedLine[1] + "counter"] = to_string(offset * -4);
    for (int i = 3, counter = offset * -4, j = 0; i < temp.size(); ++i, counter += 4, ++j)
    {
        cout << "movl $" << temp[i] << ", " << counter << "(%rbp)\n";
        variableToRbp[temp[1] + to_string(j)] = to_string(counter) + "(%rbp)";
    }
    offset++;
}

void forStatement(vector<string> parsedLine)
{
    vector<string> initialStr;

    int cmplIndex = 0;
    //int i = 0;
    if ((parsedLine[6]) != "+")
    {
        for (int i = 2; i < 6; i++)
        {
            initialStr.push_back(parsedLine[i]);
        }
        variableDeclarations(initialStr);
        cmplIndex = 7;
        initialStr.clear();
    }
    //int j = i + 1
    else
    {

        for (int i = 2; i < 8; i++)
        {
            initialStr.push_back(parsedLine[i]);
        }
        variableDeclarations(initialStr);
        cmplIndex = 9;
        initialStr.clear();
    }

    numOfCmpl++;

    //Start comparison
    cout << ".compare" << numOfCmpl << ":" << endl;

    cout << "movl " << variableToRbp[parsedLine[3]] << ", %eax" << endl;
    cout << "cmpl "
         << "$" << parsedLine[cmplIndex] << ", %eax" << endl;

    if (comparator == "<")
    {
        cout << "jge .false_for" << numOfCmpl << endl;
        endCmpl = true;
    }
    else if (comparator == ">")
    {
        cout << "jle .false_for" << numOfCmpl << endl;
        endCmpl = true;
    }
    else if (comparator == ">=")
    {
        cout << "jl .false_for" << numOfCmpl << endl;
        endCmpl = true;
    }
    else if (comparator == "<=")
    {
        cout << "jg .false_for" << numOfCmpl << endl;
        endCmpl = true;
    }

    //store the increment string until finish loop
    string temp = "";

    temp = "addl $1, " + variableToRbp[parsedLine[3]];
    incrementStr.push_back(temp);

    isFalseResult_for = true;
}

//ie. int a = 0 versus int a(int b, int c)
void variableOrFunctionDec(vector<string> &parsedLine)
{
    if (parsedLine[2] == "=")
        variableDeclarations(parsedLine);
    else if (parsedLine[2] == "(")
        functionDeclaration(parsedLine);
    else
    {
        throw std::invalid_argument("parsedLine[2] is not '=' or '('. Unable to understand statement");
    }
}

//ie. i = 2 + 2 vs i = test(a, b, c) vs. min_inx = i
void arithmeticOrFunctionCall(vector<string> &parsedLine)
{
    if (parsedLine[3] == "(")
        functionCall(parsedLine);
    else if (parsedLine[1] == "=")
        arithmeticOperation(parsedLine);
    else if (parsedLine.size() == 3)
        arithmeticOperation(parsedLine);
    else
    {
        cout << parsedLine.size();
        throw std::invalid_argument("parsedLine[3] is not '(' or '+' , '-', '*'. Unable to understand statement2");
    }
}

/**
 * Variable Name will be only alphamueric and can only contain the special character '_'
 * Characters ';', ',', '{', '(' and ')' will not be passed into the vector
*/
void parse(const string &line)
{
    vector<string> parsedLine;
    string word;
    bool isArray = false;
    bool isDeclaration = false;
    bool isCompare = false;

    std::size_t found = line.find("==");
    if (found != std::string::npos)
    {
        comparator = "==";
        isCompare = true;
    }
    found = line.find(">");
    if (found != std::string::npos)
    {
        comparator = ">";
        isCompare = true;
    }
    found = line.find("<");
    if (found != std::string::npos)
    {
        comparator = "<";
        isCompare = true;
    }
    found = line.find(">=");
    if (found != std::string::npos)
    {
        comparator = ">=";
        isCompare = true;
    }
    found = line.find("<=");
    if (found != std::string::npos)
    {
        comparator = "<=";
        isCompare = true;
    }
    found = line.find("!=");
    if (found != std::string::npos)
    {
        comparator = "!=";
        isCompare = true;
    }
    for (const auto &letter : line)
    {
        if (isalnum(letter) || letter == '_')
        {
            word += letter;
        }
        else if (isspace(letter) || letter == ';' || letter == '(' || letter == ')' || letter == ',')
        {
            if (!word.empty())
                parsedLine.push_back(word);

            if (letter == '(' || letter == ')')
            {
                parsedLine.push_back(string(1, letter));
            }
            word = "";
        }
        else if (letter == '=' || letter == '+' || letter == '-' || letter == '*' || letter == '[' || letter == ']' || letter == '{' || letter == '}')
        {
            if (letter == '[')
            {
                isArray = true;
            }
            if (!word.empty())
            {
                parsedLine.push_back(word);
            }
            if (letter == '}')
                isDeclaration = true;
            //covert char to string before pushing to vector
            parsedLine.push_back(string(1, letter));
            word = "";
        }
    }
    if (!parsedLine.empty())
    {

        //output the false label for if statement
        if (isFalseResult_if && endIf)
        {

            cout << ".false_if"
                 << ":" << endl;
            isFalseResult_if = false;
            endIf = false;
        }

        ifEnd(isFalseResult_if, parsedLine);

        //check the position of ending loop
        cmplEnd(endCmpl, parsedLine);

        if (endCmpl && endFor && !isFalseResult_if)
        {
            //print increment string
            cout << incrementStr[numOfCmpl - 1] << endl;
            cout << "jmp .compare" << numOfCmpl << endl;

            endFor = false;

            //print jump label
            if (isFalseResult_for)
            {
                cout << ".false_for" << numOfCmpl << ":" << endl;
                numOfCmpl--;
            }

            if (numOfCmpl == 0)
            {
                endCmpl = false;
                isFalseResult_for = false;
            }
        }

        if (isArray && isDeclaration)
        {
            arrayDeclartion(parsedLine);
        }
        else if (isArray)
        {
            arithOpWithArr(parsedLine);
        }
        else if (parsedLine[0] == "int")
        {
            variableOrFunctionDec(parsedLine);
        }
        else if (parsedLine[1] == "=")
        {
            arithmeticOrFunctionCall(parsedLine);
        }
        else if (parsedLine[0] == "if")
        {
            ifStatement(parsedLine);
        }
        else if (parsedLine[0] == "for")
        {
            forStatement(parsedLine);
        }
    }
}

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
    // iterateMap();
}

int main(int argc, char **argv)
{
    cout << "pushq %rbp" << '\n';
    cout << "movq %rsp, %rbp" << '\n';
    readFile(argv[1]);
    cout << "movl $0, %eax" << '\n';
    cout << "popq %rbp" << '\n';
    cout << "ret" << '\n';

    return 0;
}
