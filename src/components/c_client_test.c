#include "c_wrapper.h"
#include <stdio.h>

base* b;

int init_lib() {  
  b = create_base();
  return 1;
}

/* (after encoding) Converts a binary btye stream to valid UTF8 bytes */
const char* lib_binToUTF8(const char str[], unsigned int len)
{
  return binToUTF8( b,str, len);
}

/* Converts a stream of UTF8 chars to their 2-byte code points (ready to decode) */
const char* lib_UTF8ToBin(const char str[], unsigned int len)
{
  return UTF8ToBin( b,str, len);
}

int close_lib() {
  destroy_object(b) ;
  return 1;
}