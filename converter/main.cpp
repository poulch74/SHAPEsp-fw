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
	ifstream efile;
   
   char c;
   int i;
	string str;
   string ffname; // full file name
   string filename; // without ext
   string tmpfile;

   setlocale(LC_CTYPE, "");

   cout << "arduino bulma html.gz file converter." << endl;
   if(argc == 1)
   {
      cout << "   usage : converter <filename.html.gz>" << endl;
      exit(1);
   }

	if(argc==4)
	{ 
		// embbeder bulma.css
		if(strcmp(argv[2],"e")==0)
		{
			cout << "embedder code run" << endl;
			ffname = argv[1];
			if(ffname.find(".shtm") == string::npos)
			{
				cout << "File extension must be .shtml !!!" << endl;
				cout << "Check file type!!!" << endl;
				exit(1);
			}
		   cout << "File to parse : " << ffname << endl;
			tmpfile = ffname.substr(0,ffname.find(".shtml"));
			tmpfile+=".html";

		   ffile.open(ffname);
			ofile.open(tmpfile);
			efile.open(argv[3]);
		   while(ffile.good())
		   {
		      getline(ffile,str,'\n');
				if(str.find("<!--#include bulma#-->")!=string::npos)
				{
					ofile << efile.rdbuf() << endl;
				}
				else ofile << str << endl;
			}
		   ffile.close();
		   ofile.close();
			efile.close();
		}
		return;
	}

   ffname = argv[1];
   if(ffname.find(".gz") == string::npos)
   {
      cout << "File extension must be .gz !!!" << endl;
      cout << "Check file type!!!" << endl;
      exit(1);
   }

   // read original gz
   filename = ffname.substr(0,ffname.find(".gz"));

   //tmpfile = "index_html.h";

   cout << "File to parse : " << filename << endl;
	tmpfile = ffname.substr(0,ffname.find(".htm"));
	tmpfile+="_html.h";

   ffile.open(ffname,ios::binary);
   ofile.open(tmpfile);

   ofile << "#ifndef " << "index_html" << "_h" << endl;
   ofile << "#define " << "index_html" << "_h" << endl << endl;

   ffile.seekg (0, ffile.end);
   int length = ffile.tellg();
   ofile << "#define " << "index_htm_sz " << ffile.tellg() << endl << endl;
   ffile.seekg (0, ffile.beg);

   ofile << "const uint8_t " << "index" << "_htm[] PROGMEM = { " << endl;

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
