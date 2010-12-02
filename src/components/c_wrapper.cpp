#include "c_wrapper.h"
#include "base_class.h"

#include <bitset>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

base* create_base()
{
  return new base() ;
}

/* Convert stream of bytes to groups of two - each is a 16-bit unsigned short.
   Create a short vector and input to encoder function.
   Get back an unsigned char vector, convert to a stream of bytes, return. */
const char* binToUTF8( const base* This, const char str[] , unsigned const int len)
{
  vector<unsigned short> in;
  vector<unsigned char> out;
  
  /* Cycle through the chars and store them in a vector<short>
    ***NOT*** null terminated - may contain '\0' chars */
  const unsigned short* its = (const unsigned short*) str;
  unsigned int i;
  for (i=0; 1+(2*i) < len; i++) in.push_back( its[i] );
  if ( (2*i) < len) in.push_back( (unsigned short) str[ len-1 ] );
  
  /* Pass to conversion function */
  This->UTF8Encode2BytesUnicode(in, out);
  
  /* Convert back to char */
  char* s = new char[ out.size() + 1 ];
  for (i=0; i<out.size(); i++) s[i] = (char) out[i];
  s[i] = '\0';
  
  /* Write out to a log file */
  ofstream myfile;
  myfile.open ("/home/chris/Desktop/log.txt");
  myfile << "Original input: " << str << " length: " << len << '\n';
  myfile << "Input\n";
  for (unsigned int i = 0; i < in.size(); i++) myfile << (int) in[i] << '\n';
  myfile << "Output\n";
  for (unsigned int i = 0; i < out.size(); i++) myfile << (int) out[i] << '\n';
  myfile << "Final output: " << s << "\n";
  myfile.close(); 
  
  return (char*) s;
}

const char* UTF8ToBin( const base* This, const char str[] , unsigned const int len)
{
  vector<unsigned char> in;
  vector<unsigned short> out;
  
  /* Need to work out how many bytes there are, not chars */
  unsigned int len2 = 0;
  for (unsigned int i=0; i<len;i++) {
      // 1110xxxx 10xxxxxx 10xxxxxx
      if((str[len2] & MASK3BYTES) == MASK3BYTES) len2 += 3;
      // 110xxxxx 10xxxxxx
      else if((str[len2] & MASK2BYTES) == MASK2BYTES) len2 += 2;
      // 0xxxxxxx
      else if(str[len2] < MASKBYTE) len2 += 1;
  }
  
  /* Copy char* to vector */
  for (unsigned int i=0; i < len2; i++) in.push_back( str[i] );
  
  /* Pass to conversion function */
  This->UTF8Decode2BytesUnicode(in, out);
  
  /* Convert back to char */
  char* s = new char[ 2*out.size() + 1 ];
  unsigned int i;
  for (i=0; i<out.size(); i++) {
    s[2*i +0] = (char) out[i];
    s[2*i +1] = (char) (out[i] >> 8);
  }
  s[2*i] = '\0';
   
  /* Write out to a log file */
  ofstream myfile;
  myfile.open ("/home/chris/Desktop/log2.txt");
  myfile << "Original input: " << str << '\n';
  myfile << "Input\n";
  for (unsigned int i = 0; i < in.size(); i++) myfile << (int) in[i] << '\n';
  myfile << "Output\n";
  for (unsigned int i = 0; i < out.size(); i++) myfile << (int) out[i] << '\n';
  myfile << "Final output: " << s << "\n";
  myfile.close(); 
  
  return s;
}

void destroy_object( base* This )
{
  delete This ;
}