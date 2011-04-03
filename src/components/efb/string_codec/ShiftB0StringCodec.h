#ifndef EFB_SHIFTB0STRINGCODEC_H
#define EFB_SHIFTB0STRINGCODEC_H

// eFB Library sub-component includes
#include "IStringCodec.h"

// Useful mask bytes for UTF8 enc/decoding
#define         MASKBITS                0x3F
#define         MASKBYTE                0x80
#define         MASK2BYTES              0xC0
#define         MASK3BYTES              0xE0
#define         MASK4BYTES              0xF0
#define         MASK5BYTES              0xF8
#define         MASK6BYTES              0xFC

namespace efb {
    
    // Typdef for unicode code points
    typedef unsigned short  Unicode2Bytes;
    
    //! UTF8 codec which shifts characters by 0xB0 (176) to avoid problem characters.
    class ShiftB0StringCodec : public IStringCodec
    {
        public :
            std::string binaryToFbReady( std::vector<byte>& data) const
            {
                
                std::string output;
                // If not an even number of bytes, we pad with 0x08 byte AND prepend special padding character which is unicode: 0x10F000
                if (data.size()%2 != 0) {
                    data.push_back((byte)0x08);
                    output.push_back((byte)(MASK4BYTES | (0x10F000 >> 18)));
                    output.push_back((byte)(MASKBYTE | (0x10F000 >> 12 & MASKBITS)));
                    output.push_back((byte)(MASKBYTE | (0x10F000 >> 6 & MASKBITS)));
                    output.push_back((byte)(MASKBYTE | (0x10F000 & MASKBITS)));
                }
                std::vector<Unicode2Bytes> input( data.begin(), data.end() );
                for(unsigned int i=0; i < input.size(); i++)
                   {
                      // First we add an offset of 0x00b0
                      // so no dealing with control or null characters
                      unsigned int in = ((unsigned int) input[i]) + 0x00b0;
                      
                      // 0xxxxxxx
                      if(in < 0x80) output.push_back((byte)in);
                      
                      // 110xxxxx 10xxxxxx
                      else if(in < 0x800)
                      {
                         output.push_back((byte)(MASK2BYTES | (in >> 6)));
                         output.push_back((byte)(MASKBYTE | (in & MASKBITS)));
                      }
                      else if ( (in >= 0xd800) & (in <= 0xdfff) )
                      {  
                        // this character range is not valid (they are surrogate pair codes) so we shift 1 bit to the left and use those code points instead
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
                return output;
            }
            
            std::vector<byte> fbReadyToBinary( std::string& input ) const
            {
            bool padded = false;
            std::vector<Unicode2Bytes> output;
            for(unsigned int i=0; i < input.size();)
            {
                unsigned int ch;
                
                // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                if((input[i] & MASK4BYTES) == MASK4BYTES)
                {
                    // check for valid continuation bytes
                    if (input.size() <= i+3) throw exn;
                    if (input[i+1] & ~MASKBITS != 0x8000) throw exn;
                    if (input[i+2] & ~MASKBITS != 0x8000) throw exn;
                    if (input[i+3] & ~MASKBITS != 0x8000) throw exn;
                    //
                    if (input[i] > 0xf2 && input[i] < 0x100) 
                    {
                        // this would have been a surrogate character
                        // shift it back in to range
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
                    // check for overlong sequence
                    if (ch < 0x1 << 16) throw exn;
                    i += 4;
                }
               
                // 1110xxxx 10xxxxxx 10xxxxxx
                else if((input[i] & MASK3BYTES) == MASK3BYTES)
                {
                    // check for valid continuation bytes
                    if (input.size() <= i+2) throw exn;
                    if (input[i+1] & ~MASKBITS != 0x8000) throw exn;
                    if (input[i+2] & ~MASKBITS != 0x8000) throw exn;
                    //
                    ch = ((input[i] & 0x0F) << 12) | (
                          (input[i+1] & MASKBITS) << 6)
                         | (input[i+2] & MASKBITS);
                    // check for surrogate character
                    if (ch >= 0xd000 && ch <= 0xdfff) throw exn;
                    // check for overlong sequence
                    if (ch < 0x1 << 11) throw exn;
                    i += 3;
                }
               
                // 110xxxxx 10xxxxxx
                else if((input[i] & MASK2BYTES) == MASK2BYTES)
                {
                    // check for valid continuation byte
                    if (input.size() <= i+1) throw exn;
                    if (input[i+1] & ~MASKBITS != 0x8000) throw exn;
                    //
                    ch = ((input[i] & 0x1F) << 6) | (input[i+1] & MASKBITS);
                    // check for overlong sequence
                    if (ch < 0x1 << 7) throw exn;
                    i += 2;
                }
               
                // 0xxxxxxx
                else if(input[i] < MASKBYTE)
                {
                    ch = input[i];
                    if (ch < 176) throw exn;
                    i += 1;
                }
                
                // Bad UTF8 start byte, possibly unexpected continuation byte
                else
                {
                    throw exn;
                }
                
                // check for padding character
                if (ch == 0x10F000) {
                    if (!padded) padded = true;
                    // Otherwise, we already have seen a padding character
                    else throw exn;
                }
                else if (ch >= 176 && ch <= (0x01<<16 + 175))
                {
                    output.push_back( (Unicode2Bytes) (ch - 0xb0) ); // subtract 0xb0 offset 
                }
                else // bad byte decoded
                {
                    throw exn;
                }
               
            }
            
            std::vector<byte> data(output.begin(), output.end());
            // If we found a padding character, remove last byte
            if (padded) {
                if ( data.size() > 1) data.pop_back();
                else throw exn;
            }
            return data;
        }
    };

}

#endif //EFB_SHIFTB0STRINGCODEC_H