#ifndef BASE_CLASS_H
#define BASE_CLASS_H

#ifndef __cplusplus
#error a C++ compiler is required
#endif

#define JPEG_INTERNALS

#define cimg_verbosity 0     // Disable modal window in CImg exceptions.
#define cimg_use_jpeg 1
#include "CImg.h"

#include <cstddef>
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <time.h>
#include <sstream>
#include <bitset>
#include <blitz/array.h> 

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
    
    // Encoding/decoding data from/to UTF-8 strings
    void UTF8Encode2BytesUnicode(
      std::vector< Unicode2Bytes >  & input,
      std::vector< byte >           & output
    ) const;
    void UTF8Decode2BytesUnicode(
      std::vector< byte >           & input,
      std::vector< Unicode2Bytes >  & output
    ) const;
    
    // Encoding data within an image
    unsigned int EncryptPhoto(
      const char*                   & data_filename,
      const char*                   & dst_filename
    ) const;
    void EncodeInImage(
      cimg_library::CImg<short int>	 & img,
      std::vector<char>		         & data
    ) const;
    void EncodeInBlock(
	cimg_library::CImg<short int> & img,
	unsigned int 			y0,
	unsigned int 			x0,
	unsigned char 			a,
	unsigned char 			b,
	unsigned char 			c
    ) const;
    
    // Decoding data from an image
    unsigned int DecryptPhoto(
      const char*                   & src_filename,
      const char*                   & data_filename
    ) const;
    void DecodeFromImage(
      cimg_library::CImg<short int>	 & img,
      std::vector<char>		         & data
    ) const;
    void DecodeFromBlock(
	cimg_library::CImg<short int> & img,
	unsigned int 			y0,
	unsigned int 			x0,
        std::vector<char>		& data
    ) const;
    
  // Wavelet transforms for image encoding/decoding
    void Haar2D_DWT(
	cimg_library::CImg<short int> & img,
	unsigned int 			y0,
	unsigned int 			x0
    ) const;
    void Haar2D_DWTi(
	cimg_library::CImg<short int> & img,
	unsigned int 			y0,
	unsigned int 			x0
    ) const;
    
    // Calculate bit error rate
    unsigned int CalculateBER(
      const char*                   & file1,
      const char*                   & file2
    ) const;
    
    // Perform FEC encoding/decoding
    unsigned int ReedSolomonEncoder(
      std::string	& message,
      std::string	& fec
    ) const;
    unsigned int ReedSolomonDecoder(
      std::string	& message_plus_errors,
      std::string	& fec,
      std::string	& message
    ) const;
    
    virtual ~base() ;
    std::string str ;
};

#endif // BASE_CLASS_H
