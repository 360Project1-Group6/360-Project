#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>
#include <map>
using namespace std;

//keep track of the varibles and their corresponding location to their rbp
map<string, string> variableToRbp;
int offset = 1;

//[0] = dest, [1] = '=', [2] = op1, [3] = operation, [4] = op2
void arithmeticOperation(vector<string> parsedLine)
{

    if (parsedLine[3] == "*")
    {
        cout << "movl    " << variableToRbp[parsedLine[2]] << ", %eax\n";
        cout << "imull    " << variableToRbp[parsedLine[4]] << ", %eax\n";
        cout << "movl    "
             << "%eax, " << variableToRbp[parsedLine[0]] << '\n';
        return;
    }
    std::string opCode;
    parsedLine[3] == "+" ? opCode = "addl" : opCode = "subl";

    if (!opCode.empty())
    {
        cout << "movl    " << variableToRbp[parsedLine[2]] << ", %edx\n";
        cout << "movl    " << variableToRbp[parsedLine[4]] << ", %eax\n";
        cout << opCode << "    %edx, %eax\n";
        cout << "movl    "
             << "%eax, " << variableToRbp[parsedLine[0]] << '\n';
    }
}

void variableDeclarations(vector<string> parsedLine)
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
    for (int i = 1; i < temp.size(); i += 2)
    {
        cout << "movl  " << temp[i + 1] << ", " << -4 * offset << "(%rbp)\n";
        variableToRbp[temp[i]] = to_string(-4 * offset) + "(%rbp)";
        offset++;
    }
}

void parse(const string &line)
{
    vector<string> parsedLine;
    string word;
    for (const auto &letter : line)
    {

        if (isalnum(letter))
        {
            word += letter;
        }
        else if (isspace(letter) || letter == ';' || letter == '(' || letter == ')')
        {
            if (word == "main")
            {
                return;
            }
            if (!word.empty())
                parsedLine.push_back(word);
            word = "";
        }
        else if (letter == '=' || letter == '+' || letter == '-' || letter == '*')
        {
            //covert char to string before pushing to vector
            parsedLine.push_back(string(1, letter));
        }
    }
    if (!parsedLine.empty())
    {
        if (parsedLine[0] == "int")
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
