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
//[0] = dest, [1] = '=', [2] = op1, [3] = operation, [4] = op2
void arithmeticOperation(vector<string> parsedLine)
{
    if (parsedLine[3] == "*")
    {
        cout << "movl " << variableToRbp[parsedLine[2]] << ", %eax\n";
        cout << "imull " << variableToRbp[parsedLine[4]] << ", %eax\n";
        cout << "movl "
             << "%eax, " << variableToRbp[parsedLine[0]] << '\n';
        return;
    }
    std::string opCode;
    parsedLine[3] == "+" ? opCode = "addl" : opCode = "subl";

    if (!opCode.empty())
    {
        cout << "movl " << variableToRbp[parsedLine[2]] << ", %edx\n";
        cout << "movl " << variableToRbp[parsedLine[4]] << ", %eax\n";
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

void arithOpWithArr(vector<string> parsedLine)
{
    std::string opCode;
    isAdd(parsedLine) == true ? opCode = "addl" : opCode = "subl";
    if (parsedLine[5] == "[" && !isdigit(parsedLine[6][0]))
    {
        cout << "movl " << variableToRbp[parsedLine[6]] << ", %eax\n";
        cout << "cltq\n";
        cout << "movl " << variableToRbp[parsedLine[4] + "counter"] << "(%rbp,%rax,4), %edx\n";
        cout << "movl " << variableToRbp[parsedLine[2]] << ", %eax\n";
        cout << opCode << " %edx, %eax\n";
        cout << "movl "
             << "%eax," << variableToRbp[parsedLine[0]] << '\n';
    }
    else if (parsedLine[3] == "[" && !isdigit(parsedLine[4][0]))
    {
        cout << "movl " << variableToRbp[parsedLine[4]] << ", %eax\n";
        cout << "cltq\n";
        cout << "movl " << variableToRbp[parsedLine[2] + "counter"] << "(%rbp,%rax,4), %edx\n";
        cout << "movl " << variableToRbp[parsedLine[7]] << ", %eax\n";
        cout << opCode << " %edx, %eax\n";
        cout << "movl "
             << "%eax," << variableToRbp[parsedLine[0]] << '\n';
    }
    else if (parsedLine[5] == "[")
    {
        cout << "movl " << variableToRbp[parsedLine[4] + parsedLine[6]] << ", %edx\n";
        cout << "movl " << variableToRbp[parsedLine[2]] << ", %eax\n";
        cout << opCode << " %edx, %eax\n";
        cout << "movl "
             << "%eax," << variableToRbp[parsedLine[0]] << '\n';
    }
    else if (parsedLine[3] == "[")
    {
        cout << "movl " << variableToRbp[parsedLine[2] + parsedLine[4]] << ", %edx\n";
        cout << "movl " << variableToRbp[parsedLine[7]] << ", %eax\n";
        cout << opCode << " %edx, %eax\n";
        cout << "movl "
             << "%eax," << variableToRbp[parsedLine[0]] << '\n';
    }
}

vector<string> filterVec(vector<string> parsedLine)
{
    //only variables and their associated value will be in the array
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

void arrayDeclartion(vector<string> parsedLine)
{
    vector<string> temp = filterVec(parsedLine);
    offset += stoi(temp[2]) - 1;
    int tempOffset = offset;
    variableToRbp[parsedLine[1] + "counter"] = to_string(offset * -4 - 4);
    for (int i = 3, counter = offset * -4 - 4, j = 0; i < temp.size(); ++i, counter += 4, ++j)
    {
        cout << "movl $" << temp[i] << ", " << counter << "(%rbp)\n";
        variableToRbp[temp[1] + to_string(j)] = to_string(counter) + "(%rbp)";
    }
}

void parse(const string &line)
{
    vector<string> parsedLine;
    string word;
    bool isArray = false;
    for (const auto &letter : line)
    {
        if (isalnum(letter))
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
            if (!word.empty())
                parsedLine.push_back(word);
            parsedLine.push_back(string(1, letter));
            word = "";
        }
    }
    if (!parsedLine.empty())
    {
        if (parsedLine[2] == "[")
        {
            arrayDeclartion(parsedLine);
        }
        else if (parsedLine[3] == "[" || parsedLine[5] == "[")
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
    }
}

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
    cout << "pushq %rbp" << '\n';
    cout << "movq %rsp, %rbp" << '\n';
    readFile(argv[1]);
    cout << "movl $0, %eax" << '\n';
    cout << "popq %rbp" << '\n';
    cout << "ret" << '\n';

    return 0;
}
