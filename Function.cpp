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
            // cout << "1. index str   "<< var1Index << " array string "<< arrayVar1 << endl;
        }
        //check if op2 is a array variables
        else if (parsedLine[i] == "[" && isArrayVar1)
        {
            isArrayVar2 = true;
            var2Index = parsedLine[i + 1];
            arrayVar2 = parsedLine[i - 1];
            // cout << "2. index str "<< var2Index << " array string "<< arrayVar2 << endl;
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
        cout << "cltp" << endl;
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
        cout << "cltp" << endl;
        cout << "movl " << variableToRbp[arrayVar1 + "counter"] << "(%rbp,%rax,4), %edx" << endl;

        //print array var2
        cout << "movl " << variableToRbp[var2Index] << ", %eax" << endl;
        cout << "cltp" << endl;
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
 * 		a[min_inx] = a[i];
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
        cout << "movl %edx, " << variableToRbp[parsedLine[0] + "counter"] << "(%rbp,%rax,4)\n";
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

    if (parsedLine.size() == 6 && isdigit(parsedLine[5][0]))
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
    //for ( int i = 0;
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
    //for ( int j = i + 1
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
            if (word == "main")
            {
                return;
            }
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
            if (letter == '{')
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

        if (isArray && isDeclaration && !isCompare)
        {
            arrayDeclartion(parsedLine);
        }
        else if (parsedLine[3] == "[" || parsedLine[5] == "[" || parsedLine[1] == "[" || parsedLine[4] == "[")
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
