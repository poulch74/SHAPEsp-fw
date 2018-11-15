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

// количество элементов в хидере
#define HDR_CNT 7

// количество колонок со значениями u1 i1 u2 i2 u3 i3
#define VAL_CNT 6

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
	/*
	 static int ccount = 0;
	 int a[6] = {0,1,2,3,4,5};
	 while(!kbhit())
	 {
		cout << (a[(ccount = (ccount == 5 ? 0: ccount))++]) << endl;
		Sleep(1000);
	 }

	exit(0);
	*/
   cout << "arduino html file converter." << endl;
   if(argc == 1)
   {
      cout << "   usage : converter <filename.html>" << endl;
      exit(1);
   }

   ffname = argv[1];
   if(ffname.find(".gz") == string::npos)
   {
      cout << "File extension must be .html !!!" << endl;
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
	ofile << "const static char " << "index" << "_htm[] PROGMEM = { " << endl;

	char c;
	i = 0;
	while(ffile.get(c))
	{
		char str[128];
	   if(ffile.peek()==EOF) _snprintf(str,128,"0x%02x",(unsigned char)c);
		else _snprintf(str,128,"0x%02x,",(unsigned char)c);
		i++;
		ofile << str;
		if(i==16) {i = 0; ofile << endl;}
	}

	ofile << "};" << endl << endl;
	ofile << "#endif" << endl;

   ffile.close();
   ofile.close();
/*
   ffile.open(tmpfile);

   i = 0;

   while(ffile.good())
   {
      getline(ffile,hdr[i],'\n');
      i++;
      if(i==HDR_CNT) break; 
   }

   hdr[7] =  "\"U1\"";
   hdr[8] =  "\"I1\"";
   hdr[9] =  "\"U2\"";
   hdr[10] = "\"I2\"";
   hdr[11] = "\"U3\"";
   hdr[12] = "\"I3\"";

   // пропускаем ненужное
   getline(ffile,str,'\n');
   getline(ffile,str,'\n');

   // погнали клепать файлы
   long long fcnt = 0;
   
   while(ffile.good())
   {
      ofile.open( filename + "_" + to_string(fcnt) + ".csv");
      for(i=0;i<13;i++) ofile << hdr[i] << ';'; ofile << endl;

      int brk = 0;
      int brk_cmp = HDR_CNT;
      while(ffile.good())
      {
         getline(ffile,str,'\n');
         if(!strcmp(str.c_str(),"")) break;
         ofile << str << ';';
         brk++;
         if(brk == brk_cmp)  { ofile << endl << ";;;;;;;"; brk_cmp = VAL_CNT; brk = 0;}
      }
      fcnt++;
      ofile.close();
   }

   ffile.close();
   remove(tmpfile.c_str());
*/
}
