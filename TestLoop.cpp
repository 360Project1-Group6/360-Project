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

void iterateMap()
{
    for (const auto &x : variableToRbp)
    {
        cout << "key: " << x.first << " Value: " << x.second << ' ';
    }
    cout << '\n';
}

//Tracking the # of comparison in for loop
int numOfCmpl = 0;
bool isCmpl = false;

//store the increment instruction before print the false label
vector<string> incrementStr;
bool isIncrement = false;
bool isFalseResult_for = false;

//count the # of close bracket
int closeBracket = 0;

//Find the close bracket for printing labels
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

//For statement step1: initial the index
void forStatement(vector<string> parsedLine)
{
    //int i = 0;
    if (parsedLine.size() == 4)
    {
        cout << "movl " << '$' << parsedLine[4] << ", " << variableToRbp[parsedLine[2]] << "(%rbp)" << endl;
    }
    //int j = i + 1
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

/*for statement step2: compare
* compare two object and print the jump instruction
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

/*
*for statement step3: increse the index
*store the increment string until false label appear
*/
void increment(vector<string> parsedLine)
{
    //a++
    string temp = "addl $1, " + variableToRbp[parsedLine[0]];
    incrementStr.push_back(temp);
    isFalseResult_for = true;
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
    else if (!opCode.empty())
    {
        cout << "movl " << variableToRbp[parsedLine[2]] << ", %eax\n";
        cout << "movl " << variableToRbp[parsedLine[4]] << ", %edx\n";
        cout << opCode << " %edx, %eax\n";
        cout << "movl "
             << "%eax, " << variableToRbp[parsedLine[0]] << '\n';
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
*/
void arithOpWithArr(vector<string> parsedLine)
{
    std::string opCode;
    isAdd(parsedLine) == true ? opCode = "addl" : opCode = "subl";

    //c[a] = c[b];
    if ((parsedLine[1] == "[" && !isdigit(parsedLine[2][0])) && (parsedLine[6] == "[" && !isdigit(parsedLine[7][0]) && parsedLine.size() < 10))
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
    vector<string> temp = filterVec(parsedLine);
    for (int i = 1; i < temp.size(); i += 2)
    {
        cout << "movl $" << temp[i + 1] << ", " << -4 * offset << "(%rbp)\n";
        variableToRbp[temp[i]] = to_string(-4 * offset) + "(%rbp)";
        offset++;
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

    for (const auto &letter : line)
    {
        if (isalnum(letter) || letter == '_')
        {
            word += letter;
        }
        else if (isspace(letter) || letter == ';' || letter == '(' || letter == ')' || letter == ',')
        {
            if (word == "main")
            {
                return;
            }
            if (!word.empty())
                parsedLine.push_back(word);
            word = "";
        }
        else if (letter == '=' || letter == '+' || letter == '-' || letter == '*' || letter == '[' || letter == ']')
        {
            if (letter == '[')
            {
                isArray = true;
            }
            if (!word.empty())
            {
                parsedLine.push_back(word);
            }
            //covert char to string before pushing to vector
            parsedLine.push_back(string(1, letter));
            word = "";
        }
        else if (isArray && (letter == '{' || letter == '}'))
        {
            if (letter == '{')
                isDeclaration = true;
            if (!word.empty())
                parsedLine.push_back(word);
            parsedLine.push_back(string(1, letter));
            word = "";
        }
    }
    if (!parsedLine.empty())
    {
        findCloseBracket(parsedLine);

        if (isCmpl)
        {
            compare_for(parsedLine);
            isCmpl = false;
        }
        if (isIncrement)
        {
            increment(parsedLine);
            isIncrement = false;
        }

        //print the false label
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

        if (isArray && isDeclaration)
        {
            arrayDeclartion(parsedLine);
        }
        else if (parsedLine[3] == "[" || parsedLine[5] == "[" || parsedLine[1] == "[")
        {
            arithOpWithArr(parsedLine);
        }
        else if (parsedLine[0] == "int")
        {
            variableDeclarations(parsedLine);
        }
        else if (parsedLine[1] == "=")
        {
            arithmeticOperation(parsedLine);
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
