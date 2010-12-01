#ifndef BASE_CLASS_H
#define BASE_CLASS_H

#ifndef __cplusplus
#error a C++ compiler is required
#endif

#include <string>
#include <vector>

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
  public :
    base() ;
    
    void UTF8Encode2BytesUnicode(
      std::vector< Unicode2Bytes >  & input,
      std::vector< byte >           & output
    ) const;
    void UTF8Decode2BytesUnicode(
      std::vector< byte >           & input,
      std::vector< Unicode2Bytes >  & output
    ) const;
    
    virtual ~base() ;
    std::string str ;
    // etc
};

#endif // BASE_CLASS_H