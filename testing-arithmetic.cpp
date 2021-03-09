#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>
using namespace std;

/**
 * Can ignore '{}' and '[]'
*/
void parse(const string &line, vector<int> &stk)
{
    vector<string> parsedLine;
    string word;
    for (const auto &letter : line)
    {
        //() is irrelevant when parsing
        //int a = 0, b = 0, c = 0;
        if (letter == '(' || letter == ')')
            continue;

        if (isalpha(letter))
        {
            word += letter;
        }
        else if (isspace(letter) || letter == ';')
        {
            if (!word.empty())
                parsedLine.push_back(word);
            word = "";
        }
        word += letter;
        if (isdigit(word[0]))
        {
            if (!stk.size())
                stk.push_back(stoi(word));
        }
        cout << letter << ' ';
    }
}

void readFile(const string &txtFile)
{
    string line;
    ifstream textFile(txtFile);
    vector<int> stk;
    //Vector indexing will start at 1, formula will be vector.length()*4
    stk.push_back(-999);
    while (getline(textFile, line))
    {
        if (line.empty() || line == "{" || line == "}")
            continue;
        parse(line, stk);
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