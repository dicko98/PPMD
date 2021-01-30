// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <string>
#include <iostream>
#include <fstream>
#include "ppmd_coder.h"
using namespace std;
int main()
{
    while (1)
    {
        int orderSize;
        int memSize;
        cout << "Enter order size (2..16): \n";
        cin >> orderSize;
        cout << "Enter memory in MB: \n";
        cin >> memSize;
        cin.ignore();
        string unos;
        cout << "Enter text you wish to compress: \n";
        getline(cin, unos);
        ofstream input("i.txt");
        input << unos;
        input.close();

        PPMD_Coder ppmd("i.txt", "o.txt", orderSize, memSize);
        ppmd.Compress();

        cout << "\nNow you can see compressed text: \n \n";

        ifstream output("o.txt");
        string out;
        ofstream realcompress("realoutput.txt");
        getline(output, out);
        for (int i = 0; i < out.length(); i++)
            if (i > 16)
            {
                cout << out[i];
                realcompress << out[i];
            }

        realcompress.close();
        output.close();
        cout << "\n \n";
        struct stat stat_buf;
        string o = "realoutput.txt";
        stat(o.c_str(), &stat_buf);
        cout << "\nSize of compressed file is: ";
        cout << stat_buf.st_size;

        string i = "i.txt";
        stat(i.c_str(), &stat_buf);
        cout << "\nSize of original file is: ";
        cout << stat_buf.st_size;
        cout << "\n";

        PPMD_Coder ppmdDec("o.txt", "o2.txt", orderSize, memSize);
        ppmdDec.Uncompress();

        cout << "\nAnd here is text after decompression: \n \n";
        ifstream uncomp("o2.txt");
        string uncompOut;
        while (getline(uncomp, uncompOut)) {
            cout << uncompOut;
        }
        uncomp.close();

        cout << "\n\n\n\n";
    }
    
    system("pause");
}

