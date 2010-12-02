#include "base_class.h"
#include <iostream>

base::base()
{
  std::cout << "base::constructor\n" ;
}

void base::UTF8Encode2BytesUnicode(
    std::vector< Unicode2Bytes >  & input,
    std::vector< byte >           & output
) const
{
   for(unsigned int i=0; i < input.size(); i++)
   {
      // First we add an offset of 0x00b0
      // so no dealing with control or null characters
      unsigned int in = ((unsigned int) input[i]) + 0x00b0;
      
      // 0xxxxxxx
      if(in < 0x80)
      {
         output.push_back((byte)in);
      }
      // 110xxxxx 10xxxxxx
      else if(in < 0x800)
      {
         output.push_back((byte)(MASK2BYTES | (in >> 6)));
         output.push_back((byte)(MASKBYTE | (in & MASKBITS)));
      }
      else if ( (in >= 0xd800) & (in <= 0xdfff) )
      {  
        // this character range is not valid (they are surrogate pair codes)
        // so we shift 1 bit to the left and use those code points instead
         output.push_back((byte)(MASK4BYTES | (in >> 17)));
         output.push_back((byte)(MASKBYTE   | (in >> 11 & MASKBITS)));
         output.push_back((byte)(MASKBYTE   | (in >> 5 & MASKBITS)));
         output.push_back((byte)(MASKBYTE   | (in << 1 & MASKBITS)));
        
      }
      // 1110xxxx 10xxxxxx 10xxxxxx
      else if(in < 0x10000)
      {
         output.push_back((byte)(MASK3BYTES | (in >> 12)));
         output.push_back((byte)(MASKBYTE | (in >> 6 & MASKBITS)));
         output.push_back((byte)(MASKBYTE | (in & MASKBITS)));
      }
      // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
      else if(in < 0x10ffff)
      {
         output.push_back((byte)(MASK4BYTES | (in >> 18)));
         output.push_back((byte)(MASKBYTE | (in >> 12 & MASKBITS)));
         output.push_back((byte)(MASKBYTE | (in >> 6 & MASKBITS)));
         output.push_back((byte)(MASKBYTE | (in & MASKBITS)));
      }
   }
}

void base::UTF8Decode2BytesUnicode(
    std::vector< byte >           & input,
    std::vector< Unicode2Bytes >  & output
) const
{
   for(unsigned int i=0; i < input.size();)
   {
      unsigned int ch;

      // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
      if((input[i] & MASK4BYTES) == MASK4BYTES)
      {
        if (input[i] > 0xf2) // this is a surrogate character
        {                   
          ch = ((input[i+0] & 0x07)     << 17) |
               ((input[i+1] & MASKBITS) << 11) |
               ((input[i+2] & MASKBITS) <<  5) |
               ((input[i+3] & MASKBITS) >>  1);
        } else {
          ch = ((input[i+0] & 0x07)     << 18) |
               ((input[i+1] & MASKBITS) << 12) |
               ((input[i+2] & MASKBITS) <<  6) |
               ((input[i+3] & MASKBITS));
        }
         i += 4;
      }
      // 1110xxxx 10xxxxxx 10xxxxxx
      else if((input[i] & MASK3BYTES) == MASK3BYTES)
      {
         ch = ((input[i] & 0x0F) << 12) | (
               (input[i+1] & MASKBITS) << 6)
              | (input[i+2] & MASKBITS);
         i += 3;
      }
      // 110xxxxx 10xxxxxx
      else if((input[i] & MASK2BYTES) == MASK2BYTES)
      {
         ch = ((input[i] & 0x1F) << 6) | (input[i+1] & MASKBITS);
         i += 2;
      }
      // 0xxxxxxx
      else if(input[i] < MASKBYTE)
      {
         ch = input[i];
         i += 1;
      }
      
      // subtract 0xb0 offest
      output.push_back( (Unicode2Bytes) (ch - 0xb0) );
   }
}



base::~base()
{
  std::cout << "base::destructor\n" ;
}