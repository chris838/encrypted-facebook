#ifndef EFB_UPSAMPLED3CONDUITIMAGE_H
#define EFB_UPSAMPLED3CONDUITIMAGE_H

// Library sub-component includes
#include "UpsampledConduitImage.h"
#include "Hamming.h"

namespace efb {
    
//! Upsampling conduit image class which uses 3 bits per 8-bit pixel.
    /**
        This class stores 3 bits per pixel with an approximate error rate of 0.015%. TODO - check this.
    */
    class Upsampled3ConduitImage : public UpsampledConduitImage
    {   
        public :
            //! Constructor.
            Upsampled3ConduitImage() :
                UpsampledConduitImage(3,3) // block size is 3 bytes, order is 3 bits per pixel
            {}
            
            //! Get the maximum ammount of data (in bytes) that can be stored in this implementation.
            virtual unsigned int getMaxData()
            {
                return (720*720*3)/8 - 6;
            }
            
            //! Check how much data (if any) is stored in the current image.
            virtual unsigned int readSize()
            {
                hamming::InitEncode(4);
                std::deque<byte> data1, data2;
                decodeFromBlock(data1, 719, 704 );
                decodeFromBlock(data2, 719, 712 );
                //
                unsigned int len =
                    0   |   ( hamming::Decode( data1[0] ) << 0)
                        |   ( hamming::Decode( data1[1] ) << 4)
                        |   ( hamming::Decode( data1[2] ) << 8)
                        |   ( hamming::Decode( data2[0] ) << 12)
                        |   ( hamming::Decode( data2[1] ) << 16)
                        |   ( hamming::Decode( data2[2] ) << 20);
                return len;
            }
            
        private :
            //! Get the block coordinates based on the index of the byte we are writing.
            void getBlockCoords( unsigned int &i, unsigned int &j, unsigned int idx)
            {
                j = ((idx/block_size_)*8);
                i = j / 720;
                j = j % 720;
            }
            
            //! Encode block_size_ bytes in a block of pixels. (i,j) indicates the pixel at the start of the block.
            void encodeInBlock( std::deque<byte> data, unsigned int i, unsigned int j )
            {
                // Split 3 bytes into eight 3-bit segments
                byte r = 0x00;
                for (int k=0; k<8; k++)
                {
                    // take the kth bit of each byte
                    r =     (((data[0] & (0x1<<k)) >> k) << 0)
                        |   (((data[1] & (0x1<<k)) >> k) << 1)
                        |   (((data[2] & (0x1<<k)) >> k) << 2) ;
                    // Encode these three bits to a pixel
                    encodeInPixel(r, i, j+k);
                }
            }
            
            //! Decode block_size_ bytes from a block of pixels. (i,j) indicates the first pixel in the block.
            void decodeFromBlock( std::deque<byte> & data, unsigned int i, unsigned int j )
            {
                byte x, a, b, c;
                a=b=c=0x00;
                for (int k=0; k<8; k++)
                {
                    // Get the relevant 3 bits
                    x = decodeFromPixel(i,j+k);
                    // OR them into the bytes
                    a |= (((x & (0x1<<0)) >> 0) << k);
                    b |= (((x & (0x1<<1)) >> 1) << k);
                    c |= (((x & (0x1<<2)) >> 2) << k);
                }
                // Append the bytes to the data
                data.push_back( a );
                data.push_back( b );
                data.push_back( c );
            }
            
            //! Writes the size of the image (3 bytes) to the last eight pixels, using Hamming codes.
            void writeSize
            (
                unsigned int len    
            )
            {
                // Encode with Hamming (8,4) SECDEC codes.
                hamming::InitEncode(4);
                std::deque<byte> data1;
                std::deque<byte> data2;
                data1.push_back( (byte) hamming::Encode( (len >> 0) & 0x0000000f ) );
                data1.push_back( (byte) hamming::Encode( (len >> 4) & 0x0000000f ) );
                data1.push_back( (byte) hamming::Encode( (len >> 8) & 0x0000000f ) );
                data2.push_back( (byte) hamming::Encode( (len >> 12) & 0x0000000f ) );
                data2.push_back( (byte) hamming::Encode( (len >> 16) & 0x0000000f ) );
                data2.push_back( (byte) hamming::Encode( (len >> 20) & 0x0000000f ) );
                //
                encodeInBlock( data1, 719, 704 );
                encodeInBlock( data2, 719, 712 );
            }
    };
    
}
    
#endif //EFB_UPSAMPLED3CONDUITIMAGE_H
    