#include <iostream>
#include <fstream>
#include <string>
using namespace std;

void readFile(const string &txtFile)
{
    string line;
    ifstream textFile(txtFile);

    while (getline(textFile, line))
    {
        if (line.empty())
            continue;
        cout << line << '\n';
    }
}

int main(int argc, char **argv)
{

    readFile(argv[1]);

    return 0;
}