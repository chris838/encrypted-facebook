#ifndef EFB_UPSAMPLEDCONDUITIMAGE_H
#define EFB_UPSAMPLEDCONDUITIMAGE_H

// Library sub-component includes
#include "BufferedConduitImage.h"

namespace efb {
    
    //! Buffered conduit image abstract subclass which stores order_ bits for each 8-bit pixel value, scaled up and offset to span the 0-255 range.
    class UpsampledConduitImage : public BufferedConduitImage
    {
        //! Attribute which determines how many bits are stored in each pixel.
        const unsigned int order_;
                
        //! Convert binary to 8-bit gray codes.
        byte binaryToGray( byte num )
        {
            return (num>>1) ^ num;
        }
        
        //! Convert 8-bit gray codes to binary.
        byte grayToBinary( byte num )
        {
            unsigned short temp = num ^ (num>>8);
            temp ^= temp>>4;
            temp ^= temp>>2;
            temp ^= temp>>1;
            return temp;
        }
        
        protected :
            
            //! Encode bits into a single pixel, using Gray codes.
            void encodeInPixel( byte data, unsigned int i, unsigned int j )
            {
                // Choose a scale factor and an offset that minimise errors.
                unsigned int factor = (255 / ((0x01 << order_)-1)) + 1;            
                unsigned int offset =  ((factor * ((0x01 << order_)-1)) - 255) / 2;
                int x = (binaryToGray(data) * factor) - offset;
                // Output in range 0-255 inclusive
                operator()(i,j) =  (x>255? 255 : (x<0? 0 : x));
            }
            
            // Decode order_ bits from a single pixel.
            byte decodeFromPixel( unsigned int i, unsigned int j )
            {
                // Choose a scale factor and an offset that minimise errors.
                unsigned int factor = (255 / ((0x01 << order_)-1)) + 1;            
                unsigned int offset =  ((factor * ((0x01 << order_)-1)) - 255) / 2;
                int x = operator()(i,j) + offset;
                int y = factor;
                // Round to nearest value
                int r = (( x%y <<1) >= y) ? (x/y) + 1 : (x/y);
                // Cap to max value
                byte max = (0x01 << order_)-1;
                r = (r > max) ? max : r;
                // Convert to binary from gray code
                return grayToBinary( r );
            }
        
        public :
            
            //! Constructor.
            UpsampledConduitImage(unsigned int block_size, unsigned int order) :
                BufferedConduitImage(block_size),
                order_(order)
            {}
    };
    
}
    
#endif //EFB_UPSAMPLEDCONDUITIMAGE_H
    