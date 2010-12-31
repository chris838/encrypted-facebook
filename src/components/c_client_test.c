#include "c_wrapper.h"
#include <stdio.h>

base* b;

int init_lib(const char* cache_dir, const char* temp_dir) {  
  b = create_base( cache_dir,temp_dir );
  return 1;
}

/* (after encrypting) Converts a binary btye stream to valid UTF8 bytes */
const char* lib_binToUTF8(const char str[], unsigned int len)
{
  return binToUTF8( b,str, len);
}

/* Converts a stream of UTF8 chars to their 2-byte code points (ready to decrypt) */
const char* lib_UTF8ToBin(const char str[], unsigned int len)
{
  return UTF8ToBin( b,str, len);
}

/* Takes the full path to a photo and generates a (temporary) encrypted version, ready to upload */
const unsigned int lib_EncryptPhoto( const char pubkeys[], const unsigned int len, const char data[], const char dst[])
{
  return EncryptPhoto( b,pubkeys,len,data,dst );
}

const unsigned int lib_DecryptPhoto(const char src[], const char data[])
{
  return DecryptPhoto( b,src,data);
}

const unsigned int lib_CalculateBER(const char f1[], const char f2[])
{
  return CalculateBER( b,f1,f2);
}

int close_lib() {
  destroy_object(b) ;
  return 1;
}