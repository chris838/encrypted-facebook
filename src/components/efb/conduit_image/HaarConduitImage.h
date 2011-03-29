#ifndef EFB_HAARCONDUITIMAGE_H
#define EFB_HAARCONDUITIMAGE_H

// Library sub-component includes
#include "BufferedConduitImage.h"

namespace efb {
    
    //! Conduit image class which uses the Haar wavelet tranform to store data in low frequency image components.
    class HaarConduitImage : public BufferedConduitImage
    {
        //! Get the block coordinates based on the index of the byte we are writing.
        void getBlockCoords( unsigned int &i, unsigned int &j, unsigned int idx)
        {
            i = ((idx/block_size_) / 90)*8 ;
            j = ((idx/block_size_) % 90)*8 ;
        }
        
        //! Encode block_size_ bytes in a block of pixels. (i,j) indicates the pixel at the start of the block.
        void encodeInBlock( std::deque<byte> data, unsigned int i, unsigned int j )
        {
            // Temp workspace variable
            short int temp[8][8];
            // Apply the Haar transform to the block (two iterations)
            haar2dDwt (i, j, temp);
            
            // Write 3 bytes of data to the temp block
            byte a, b, c;
            a = data[0]; b=data[1]; c=data[2];
            temp[0][0] 	= (a & 0xfc) | 0x02;
            temp[1][0] 	= (b & 0xfc) | 0x02;
            temp[0][1] 	= (c & 0xfc) | 0x02;
            temp[1][1]	= ((a & 0x03) <<6) | ((b & 0x03) <<4) | ((c & 0x03) <<2) | 0x02;
            
            // Apply the inverse transform and write back to the image
            haar2dDwti(temp, i, j);
        }
        

        //! Perform the Haar Discrete Wavelet transform on an 8x8 temp block.
        /**
            Perform the Haar Discrete Wavelet Transform (with lifting scheme so the inverse can be performed losslessly). The result is written to the supplied 8x8 array, since we cannot do this in place (we would need (at least) an extra sign bit for 3/4 of the 64 coefficients).
        */
        void haar2dDwt
        (
            unsigned int x0,
            unsigned int y0,
            short int temp[8][8]
        ) const
        {
            short int temp2[8][8];
            // First iteration works on the entire 8x8 block
            // For each row...
            for (unsigned int j=0; j<8; j++) {
                // Perform 1D Haar transfrom with integer lifting
                for (unsigned int i=0; i<4; i++) {
                    // average
                    temp2[i][j] = divFloor( operator()(x0+(2*i), y0+j) +
                                            operator()(x0+((2*i)+1), y0+j),
                                            2);
                    // difference
                    temp2[4+i][j] = operator()(x0+(2*i), y0+j) -
                                    operator()(x0+((2*i)+1), y0+j);
                }
            }
            // For each column...
            for (unsigned int i=0; i<8; i++) {
                // Perform 1D Haar transfrom with integer lifting
                for (unsigned int j=0; j<4; j++) {
                    // average
                    temp[i][j] = divFloor( temp2[i][2*j] + temp2[i][(2*j)+1], 2);
                    // difference
                    temp[i][4+j] = temp2[i][2*j] - temp2[i][(2*j)+1];
                }
            }
            // Then the next iteration on the top left 4x4 corner block
            // For each row...
            for (unsigned int j=0; j<4; j++) {
                // Perform 1D Haar transfrom with integer lifting
                for (unsigned int i=0; i<2; i++) {
                    // average
                    temp2[i][j] = divFloor(temp[2*i][j] + temp[2*i+1][j], 2);
                    // difference
                    temp2[2+i][j] = temp[2*i][j] - temp[2*i+1][j];
                }
            }
            // For each column...
            for (unsigned int i=0; i<4; i++) {
                // Perform 1D Haar transfrom with integer lifting
                for (unsigned int j=0; j<2; j++) {
                    // average
                    temp[i][j] = divFloor(temp2[i][2*j] + temp2[i][2*j+1], 2);
                    // difference
                    temp[i][2+j] = temp2[i][2*j] - temp2[i][2*j+1];
                }
            }
        }
        
        //! Helper function for lifting scheme
        int divFloor(int a, int b) const {
          if (a>=0) return a/b;
          else return (a-1)/b; 
        }
        
        //! Truncate Haar coefficients (preserving stored data).
        void truncateCoefficients( short int& p1, short int& p2, short int& m)
        {
            // If p1 or p2 lie outside the range 0-255 we must rectify this, however we *MUST* also preserve their mean value (m) as this contains data.
            /*
            if (p1<0) {p1 = 0; p2 = 2*m;}
            if (p1>255) {p1 = 255; p2 = 2*m-255;}
            if (p2<0) {p2 = 0; p1 = 2*m;}
            if (p2>255) {p2 = 255; p1 = 2*m-255;}
            */
            if ((p1<0) || (p1>255) || (p2<0) || (p2>255)) {p1=p2=m;}
            
        }
        
        //! Perform the inverse Haar Discrete Wavelet transform on an 8x8 block.
        /**
            Perform the inverse Haar Discrete Wavelet Transform using a lifting scheme. We start at pixel (x0,y0) in the image and work on the subsequent 8x8 block. We read in from the supplied temp block containing the wavelet coefficients.
        */
        void haar2dDwti(
            short int temp[8][8],
            unsigned int x0,
            unsigned int y0
        )
        {
            short int temp2[8][8], p1, p2;
            // First iteration just on the 4x4 top left corner block
            // For each column...
            for (unsigned int i=0; i<4; i++) {
                // Perform 1D inverse Haar transfrom with integer lifting
                for (unsigned int j=0; j<2; j++) {
                    p1 	= temp[i][j] + divFloor(temp[i][2+j]+1,2) ;
                    p2 	= p1 - temp[i][2+j];
                    // Check we don't overflow the pixel
                    if (i<2) truncateCoefficients(p1,p2,temp[i][j]);
                    temp2[i][2*j] = p1;
                    temp2[i][2*j+1] = p2;
                }
            }
            // For each row (do the same again)...
            for (unsigned int j=0; j<4; j++) {
                // Perform 1D inverse Haar transfrom with integer lifting
                for (unsigned int i=0; i<2; i++) {
                    // Check we don't overflow the pixel
                    p1 	= temp2[i][j] + divFloor(temp2[2+i][j]+1,2) ;
                    p2 	= p1 - temp2[2+i][j];
                    truncateCoefficients(p1,p2,temp2[i][j]);
                    temp[2*i][j] =  p1;
                    temp[2*i+1][j] = p2;                    
                }
            }
            // Then the next iteration on the entire 8x8 block
            // For each column...
            for (unsigned int i=0; i<8; i++) {
                // Perform 1D inverse Haar transfrom with integer lifting
                for (unsigned int j=0; j<4; j++) {
                    // Check we don't overflow the pixel
                    p1 	= temp[i][j] + divFloor(temp[i][4+j]+1,2) ;
                    p2 	= p1 - temp[i][4+j];
                    if (i<4) truncateCoefficients(p1,p2,temp[i][j]);
                    temp2[i][2*j] = p1;
                    temp2[i][2*j+1] = p2; 
                }
            }
            // For each row (do the same again)...
            for (unsigned int j=0; j<8; j++) {
                // Perform 1D inverse Haar transfrom with integer lifting
                for (unsigned int i=0; i<4; i++) {
                    // Check we don't overflow the pixel
                    p1 	= temp2[i][j] + divFloor(temp2[4+i][j]+1,2) ;
                    p2 	= p1 - temp2[4+i][j];
                    truncateCoefficients(p1,p2,temp2[i][j]);
                    operator()(x0+2*i, y0+j) = temp[2*i][j] = p1;
                    operator()(x0+2*i+1, y0+j) = temp[2*i+1][j] = p2;                    
                }
            }
        }
        
        //! Decode block_size_ bytes from a block of pixels. (i,j) indicates the first pixel in the block.
        void decodeFromBlock( std::deque<byte> & data, unsigned int i, unsigned int j )
        {
            // Temp workspace variable
            short int temp[8][8];
            // Apply the transform (two iterations) to the block
            haar2dDwt (i, j, temp);
            
            // Extract the four approximation coefficients
            byte p1,p2,p3,p4;
            p1 = temp[0][0];
            p2 = temp[1][0];
            p3 = temp[0][1];
            p4 = temp[1][1];
            // Retrive the three data bytes
            byte a,b,c;
            a = (p1 & 0xfc) | ((p4 & 0xc0) >> 6);
            b = (p2 & 0xfc) | ((p4 & 0x30) >> 4);
            c = (p3 & 0xfc) | ((p4 & 0x0c) >> 2);
            // Append them to the read buffer
            data.push_back( a );
            data.push_back( b );
            data.push_back( c );
        }
        
        public :
            
            //! Constructor.
            HaarConduitImage() :
                BufferedConduitImage(3)
            {}
            
            //! Get the maximum ammount of data (in bytes) that can be stored in this implementation.
            virtual unsigned int getMaxData()
            {
                return (90*90*3);
            }
    };
    
}
    
#endif //EFB_HAARCONDUITIMAGE_H
    