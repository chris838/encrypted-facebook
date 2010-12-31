#ifndef BASE_CLASS_H
#define BASE_CLASS_H

#ifndef __cplusplus
#error a C++ compiler is required
#endif

#include "DataCarrierImg.h"

#include <cstddef>
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <time.h>
#include <sstream>
#include <bitset>

#include <botan/botan.h>

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

// each PubKey is used to store an (ID, public key) pair,
// an array of these is passed to EncryptPhoto
typedef struct
{
    unsigned char id  [8]; 	// 8-byte Facebook ID
    unsigned char key [256];	// 256-btye (2048-bit) RSA public key
} PubKey;

// each HeadEntry is used to store an (ID, encrypted message key) pair,
// we extract an array of these during DecryptPhoto
typedef struct
{
    unsigned char id  [8];	// 8-byte Facebook ID
    unsigned char key [16];	// 16-byte (128-bit) AES-128 message key (encrypted)
} HeadEntry;

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
      const PubKey		      pubkeys[],
      const unsigned int	      len,
      const char*                   & data_filename,
      const char*                   & dst_filename
    ) const;
    
    // Decoding data from an image
    unsigned int DecryptPhoto(
      const char*                   & src_filename,
      const char*                   & data_filename
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
