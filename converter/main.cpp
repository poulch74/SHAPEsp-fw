#include <conio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <clocale>
#include <stdio.h>
#include <conio.h>
#include <windows.h>

using namespace std;

void main(int argc, char *argv[])
{
   ifstream ffile;
   ofstream ofile;

   string hdr[13]; // тут  текстовый заголовок названий колонок
   string str;
   int i;
   string ffname; // full file name
   string filename; // without ext
   string tmpfile;

   setlocale(LC_CTYPE, "");

   cout << "arduino html file converter." << endl;
   if(argc == 1)
   {
      cout << "   usage : converter <filename.html>" << endl;
      exit(1);
   }

   ffname = argv[1];
   if(ffname.find(".gz") == string::npos)
   {
      cout << "File extension must be .gz !!!" << endl;
      cout << "Check file type!!!" << endl;
      exit(1);
   }

   // read original csv
   filename = ffname.substr(0,ffname.find(".gz"));

   tmpfile = "index_html.h";

   cout << "File to parse : " << filename << endl;

   ffile.open(ffname,ios::binary);
   ofile.open(tmpfile);

	ofile << "#ifndef " << "index_html" << "_h" << endl;
	ofile << "#define " << "index_html" << "_h" << endl << endl;

   ffile.seekg (0, ffile.end);
   int length = ffile.tellg();
	ofile << "#define " << "index_htm_sz " << ffile.tellg() << endl << endl;
	ffile.seekg (0, ffile.beg);

	ofile << "const uint8_t " << "index" << "_htm[] PROGMEM = { " << endl;

	char c;
	i = 0;
	while(ffile.get(c))
	{
		char str[32];
	   if(ffile.peek()==EOF) _snprintf_s(str,32,"0x%02x",(unsigned char)c);
		else _snprintf_s(str,32,"0x%02x,",(unsigned char)c);
		i++;
		ofile << str;
		if(i==16) {i = 0; ofile << endl;}
	}

	ofile << "};" << endl << endl;
	ofile << "#endif" << endl;

   ffile.close();
   ofile.close();

}
