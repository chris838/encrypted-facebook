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
      // 00000000 (null char)
      if(input[i] == 0x00)
      {
        // output two bytes for null char, 0x7f 0x7e
         output.push_back((byte) 0x7f);
         output.push_back((byte) 0x7e);
      }
      // 01111111 (need to deal with to avoid confusion with null)
      else if(input[i] == 0x7f)
      {
        // outputs two bytes, 0x7f 0x7f
         output.push_back((byte) 0x7f);
         output.push_back((byte) 0x7f);
      }
      // 0xxxxxxx
      else if(input[i] < 0x80)
      {
         output.push_back((byte)input[i]);
      }
      // 110xxxxx 10xxxxxx
      else if(input[i] < 0x800)
      {
         output.push_back((byte)(MASK2BYTES | (input[i] >> 6)));
         output.push_back((byte)(MASKBYTE | (input[i] & MASKBITS)));
      }
      // 1110xxxx 10xxxxxx 10xxxxxx
      else if(input[i] < 0x10000)
      {
         output.push_back((byte)(MASK3BYTES | (input[i] >> 12)));
         output.push_back((byte)(MASKBYTE | (input[i] >> 6 & MASKBITS)));
         output.push_back((byte)(MASKBYTE | (input[i] & MASKBITS)));
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
      Unicode2Bytes ch;

      // 1110xxxx 10xxxxxx 10xxxxxx
      if((input[i] & MASK3BYTES) == MASK3BYTES)
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
      // 01111111 [01111111 OR 01111110]
      // Needs special treatment - could be 0x7f or a null char 0x00
      else if(input[i] == 0x7f)
      {
        if (input[i+1] == 0x7f) ch = input[i];
        else if (input[i+1] == 0x7e) ch = (Unicode2Bytes) 0x0000;
        i += 2;
      }
      // 0xxxxxxx
      else if(input[i] < MASKBYTE)
      {
         ch = input[i];
         i += 1;
      }

      output.push_back(ch);
   }
}



base::~base()
{
  std::cout << "base::destructor\n" ;
}