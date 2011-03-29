#ifndef EFB_UPSAMPLED4CONDUITIMAGE_H
#define EFB_UPSAMPLED4CONDUITIMAGE_H

// Library sub-component includes
#include "UpsampledConduitImage.h"

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
        
        public :
            
            //! Constructor.
            Upsampled4ConduitImage() :
                UpsampledConduitImage(1,4) // block size is 1 byte, order is 4 bits per pixel
            {}
            
            //! Get the maximum ammount of data (in bytes) that can be stored in this implementation.
            virtual unsigned int getMaxData()
            {
                return (720*720*4)/8;
            }
    };
    
}
    
#endif //EFB_UPSAMPLED4CONDUITIMAGE_H
    