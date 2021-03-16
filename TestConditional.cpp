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

//check if there is jump Label
bool isFalseResult_if = false;
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

        //output the false label for if statement
        if (isFalseResult_if && (closeBracket > 0))
        {
            cout << ".False_if" << endl;
            isFalseResult_if = false;
            closeBracket--;
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
        else if (parsedLine[0] == "if")
        {
            ifStatement(parsedLine);
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

/*********************************************      INSERT CODE HERE        *****************************************************************

*********************************************************************************************************************************************/

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
