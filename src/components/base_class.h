#ifndef BASE_CLASS_H
#define BASE_CLASS_H

#ifndef __cplusplus
#error a C++ compiler is required
#endif

#define JPEG_INTERNALS

#define cimg_verbosity 0     // Disable modal window in CImg exceptions.
#define cimg_use_jpeg 1
#include "CImg.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <time.h>
#include <sstream>


#define         MASKBITS                0x3F
#define         MASKBYTE                0x80
#define         MASK2BYTES              0xC0
#define         MASK3BYTES              0xE0
#define         MASK4BYTES              0xF0
#define         MASK5BYTES              0xF8
#define         MASK6BYTES              0xFC

typedef unsigned short    Unicode2Bytes;
typedef unsigned int      Unicode4Bytes;
typedef unsigned char     byte;


class base
{
  const std::string cache_dir;
  const std::string temp_dir;
  
  public :
    
    base(std::string cd, std::string td);
    
    void UTF8Encode2BytesUnicode(
      std::vector< Unicode2Bytes >  & input,
      std::vector< byte >           & output
    ) const;
    void UTF8Decode2BytesUnicode(
      std::vector< byte >           & input,
      std::vector< Unicode2Bytes >  & output
    ) const;
    
    unsigned int EncryptPhoto(
      const char*                   & pathtofile
    ) const;
    unsigned int WriteToDSTCoeffs(
    ) const;
    unsigned int ReadFromDSTCoeffs(
    ) const;
    
    virtual ~base() ;
    std::string str ;
    // etc
};

#endif // BASE_CLASS_H