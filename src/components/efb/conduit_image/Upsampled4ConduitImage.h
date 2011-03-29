#ifndef EFB_UPSAMPLED4CONDUITIMAGE_H
#define EFB_UPSAMPLED4CONDUITIMAGE_H

// Library sub-component includes
#include "UpsampledConduitImage.h"
#include "Hamming.h"

namespace efb {
    
    //! Upsampling conduit image class which uses 4 bits per 8-bit pixel.
    /**
        This class stores 4 bits per pixel with an approximate error rate of 4.5%.
    */
    class Upsampled4ConduitImage : public UpsampledConduitImage
    {
        //! Get the block coordinates based on the index of the byte we are writing.
        void getBlockCoords( unsigned int &i, unsigned int &j, unsigned int idx)
        {
            j = ((idx/block_size_)*2);
            i = j / 720;
            j = j % 720;
        }
        
        //! Encode block_size_ bytes in a block of pixels. (i,j) indicates the pixel at the start of the block.
        void encodeInBlock( std::deque<byte> data, unsigned int i, unsigned int j )
        {
            // Split the byte into two segments
            byte b=data[0], hi = 0x00, lo = 0x00;
            hi = b >> 4;
            lo = b & 0x0f;
            encodeInPixel(hi, i, j);
            encodeInPixel(lo, i, j+1);
        }
        
        //! Decode block_size_ bytes from a block of pixels. (i,j) indicates the first pixel in the block.
        void decodeFromBlock( std::deque<byte> & data, unsigned int i, unsigned int j )
        {
            byte hi=0x00, lo=0x00;
            hi = decodeFromPixel(i,j);
            lo = decodeFromPixel(i,j+1);
            data.push_back( (hi << 4) | lo );
        }
        
        //! Writes the size of the image (3 bytes) to the last twelve pixels, using Hamming codes
        void writeSize
        (
            unsigned int len    
        )
        {
                // Encode with Hamming (8,4) SECDEC codes.
                hamming::InitEncode(4);
                std::deque<byte> data1,data2,data3,data4,data5,data6;
                data1.push_back( (byte) hamming::Encode( (len >> 0) & 0x0000000f ) );
                data2.push_back( (byte) hamming::Encode( (len >> 4) & 0x0000000f ) );
                data3.push_back( (byte) hamming::Encode( (len >> 8) & 0x0000000f ) );
                data4.push_back( (byte) hamming::Encode( (len >> 12) & 0x0000000f ) );
                data5.push_back( (byte) hamming::Encode( (len >> 16) & 0x0000000f ) );
                data6.push_back( (byte) hamming::Encode( (len >> 20) & 0x0000000f ) );
                //
                encodeInBlock( data1, 719, 708 );
                encodeInBlock( data2, 719, 710 );
                encodeInBlock( data3, 719, 712 );
                encodeInBlock( data4, 719, 714 );
                encodeInBlock( data5, 719, 716 );
                encodeInBlock( data6, 719, 718 );
        }
        
        public :
            
            //! Constructor.
            Upsampled4ConduitImage() :
                UpsampledConduitImage(1,4) // block size is 1 byte, order is 4 bits per pixel
            {}
            
            //! Get the maximum ammount of data (in bytes) that can be stored in this implementation.
            virtual unsigned int getMaxData()
            {
                return (720*720*4)/8 - 6;
            }
            
            //! Check how much data (if any) is stored in the current image.
            virtual unsigned int readSize()
            {
                hamming::InitEncode(4);
                std::deque<byte> data1,data2,data3,data4,data5,data6;
                decodeFromBlock( data1, 719, 708 );
                decodeFromBlock( data2, 719, 710 );
                decodeFromBlock( data3, 719, 712 );
                decodeFromBlock( data4, 719, 714 );
                decodeFromBlock( data5, 719, 716 );
                decodeFromBlock( data6, 719, 718 );
                unsigned int len =
                    0   |   ( hamming::Decode( data1[0] ) << 0)
                        |   ( hamming::Decode( data2[0] ) << 4)
                        |   ( hamming::Decode( data3[0] ) << 8)
                        |   ( hamming::Decode( data4[0] ) << 12)
                        |   ( hamming::Decode( data5[0] ) << 16)
                        |   ( hamming::Decode( data6[0] ) << 20);
                return len;
            }
    };
    
}
    
#endif //EFB_UPSAMPLED4CONDUITIMAGE_H
    