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
unordered_map<string, string> functionMemo; //for storing (test) function instruction (although knowing it's only going to be one function named 'test' perhaps this is overkill)

//////////////////////////
// FUNCTIONS............//
//////////////////////////

/* 
int test(int a, int b)
{
    int c = 0;
    c = a - b;
    return c;
}

i = test(a, b) <-- function call

key: function name
value MUST consider: 
# of variables declared (not necessarily # of variables in function declaration although in the test cases it is)
instructions for helper function calls using said variables
*/
void functionDeclaration(vector<string> &parsedLine)
{
    cout << parsedLine[1] << ":" << endl; //label
}

/**
 * switch statement that uses first element of vector<string> parsedLine to detect statement type (if, for, arithmetic, return, function call)
 *  
*/
void detectStatement(vector<string> &parsedLine)
{
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
        //ie. int a = 0 versus int a(int b, int c). Checking parsedLine[2] for '=' or '(' (variable declaration vs function declaration)
        break;
    default:
        arithmeticOrFunctionCall(parsedLine);
        // in this case, we must check if parsedLine[1] is '(', '+', '-','*' or alpha in order to determine nature of statement (arithmetic vs function call)
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

void arithmeticOrFunctionCall(vector<string> &parsedLine)
{
    enum arithmeticSigns('+', '-', '*');

    if (parsedLine[1] == '(')
        functionCall(parsedLine);
    else if (parsedLine[1] == arithmeticSigns)
        arithmeticStatement(parsedLine);
    else
        throw std::invalid_argument("parsedLine[1] is not '(' or '+' , '-', '*'. Unable to understand statement");
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

*/
void ifStatement(vector<string> parsedLine)
{

    if (parsedLine.size() == 4)
    {
        cout << "movl   " << variableToRbp[parsedLine[1]] << "(%rbp)"
             << ",%eax" << endl;
        cout << "cmpl   " << variableToRbp[parsedLine[3]] << "(%rbp)"
             << ",%eax" << endl;
        // a < b
        if (parsedLine[2] == "<")
        {
            cout << "jl     .true" << endl;
        }
        // a > b
        if (parsedLine[2] == ">")
        {
            cout << "jg     .true" << endl;
        }
    }
    else
    {
    }

    for (size_t i = 1; i < parsedLine.size(); i++)
    {

        //need to find each var offest
        cout << "movl   " << -4 * offest_a << "(%rbp)"
             << ",%eax" << endl;
        cout << "cmpl   " << -4 * offest_b << "(%rbp)"
             << ",%eax" << endl;

        //a != b
        if (parsedLine[i] == "!" && parsedLine[i + 1] == "=")
        {
            cout << "jne    .true" << endl;
        }
        //a < b
        else if (parsedLine[i] == "<" && parsedLine[i + 1] != "=")
        {
            cout << "jl     .true" << endl;
        }
        //a <= b
        else if (parsedLine[i] == "<" && parsedLine[i + 1] == "=")
        {
            cout << "jle    .true" << endl;
        }
        //a > b
        else if (parsedLine[i] == ">" && parsedLine[i + 1] != "=")
        {
            cout << "jg     .true" << endl;
        }
        //a >= b
        else if (parsedLine[i] == ">" && parsedLine[i + 1] == "=")
        {
            cout << "jge    .true" << endl;
        }
        //a == b
        else
        {
            cout << "je     .true" << endl;
        }
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
    //return a variable
    else
    {
        cout << "movl     " << -4 * var_offest << "(%rbp)"
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
        if (line.empty() || line == "{" || line == "}")
            continue;
        parse(line);
    }
}

int main(int argc, char **argv)
{
    // cout << "pushq %rbp" << '\n';
    // cout << "movq %rsp, %rbp" << '\n';
    readFile(argv[1]);
    // cout << "movl $0, %eax" << '\n';
    // cout << "popq %rbp" << '\n';
    // cout << "ret" << '\n';

    return 0;
}