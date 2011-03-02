#ifndef EFB_CONDUITIMAGE_H
#define EFB_CONDUITIMAGE_H

// Standard library includes
#include <vector>
#include <deque>

// CImg library includes with JPEG support
#define JPEG_INTERNALS
#define cimg_verbosity 0     // Disable modal window in CImg exceptions.
#define cimg_use_jpeg 1
#include "CImg.h"

// Library sub-component includes
#include "Exceptions.h"

/**
    This file contains the interface definition for the conduit image along with any classes which implement that interface.
*/

namespace efb {
    
    // Conduit image exceptions
    struct ConduitImageImplantException : public ImplantException {
        ConduitImageImplantException(const std::string &err) : ImplantException(err) {} };
    struct ConduitImageExtractException : public ExtractException {
        ConduitImageExtractException(const std::string &err) : ExtractException(err) {} };
        
    //! Abstract class defining a conduit image, with functionality for implanting and extracting data.
    class IConduitImage : public cimg_library::CImg<byte>
    {
        public :
            //! Get the maximum ammount of data that can be stored in this implementation.
            virtual unsigned int getMaxData() = 0;
            //! Check how much data (if any) is stored in the current image.
            virtual unsigned int readSize() = 0;
            //! Implant data.
            virtual void implantData( std::vector<byte>& data ) = 0;
            //! Extract data.
            virtual void extractData( std::vector<byte>& data ) = 0;
    };
    
    
    //! Conduit image (abstract) class which buffers data bytes and reades/writes several at a time to a block in the image.
    class BufferedConduitImage : public IConduitImage
   {           
        //! Buffer to store data while reading and writing.
        std::deque<byte> rbuffer_, wbuffer_;
        
        //! Counters indicating the read and write heads.
        unsigned int rhead_, whead_;
                
        //! Format the image in preparation for implantation.
        /**
            This operation will resize the image to 720x720x1 and truncate the colour channels, as only data storage in single-channel (greyscale) image is supported. JPEG compression requires a (lossy) colour space transform from RGB to YCrCb which complicates using colour images for data storage. Even worse - Facebook's JPEG compression process uses chrominance subsampling. However, this does mean that discarding the additional two chrominance channels only results in a %50 reduction in maximum potential data storage capacity.        
         */
        void formatForImplantation()
        {
            // Format the image to 720x720, greyscale, single slice (resample using Lanczos)
            resize(720,720,1,-1,6);
            channel(0);
        }
        
        //! Write a single byte to the image.
        /**
            Bytes are queued into a buffer which is flushed automatically whenever its size reaches that of the block size. The write head indicates the number of bytes already stored in the image - it *DOES NOT* include bytes stored in the buffer.
        */
        void write( byte b )
        {
            wbuffer_.push_back( b );
            // If we have enough bytes, flush the buffer.
            if ( wbuffer_.size() >= block_size_ ) flush(); 
        }
        
        //! Flush the buffer. The buffer contents (if not empty) will be written out to the image. Random bytes are used as padding if the buffer is partially full.
        void flush()
        {
            if ( wbuffer_.size() > 0 )
            {
                // If partially full, pad with randoms.
                while (wbuffer_.size() < block_size_) wbuffer_.push_back( std::rand() );
                // Calculate the pixel coords based on the write head position
                unsigned int i,j;
                getBlockCoords(i,j, whead_);
                // Write out the buffer contents
                encodeInBlock( wbuffer_, i, j );
                // Increment write head
                whead_+=block_size_;
                // Clear the buffer
                wbuffer_.clear();
            }
        }
        
        //! Get the block coordinates based on the index of the byte we are writing.
        virtual void getBlockCoords( unsigned int &i, unsigned int &j, unsigned int idx) = 0;
        
        //! Encode block_size_ bytes in a block of pixels. (i,j) indicates the pixel at the start of the block.
        virtual void encodeInBlock( std::deque<byte> data, unsigned int i, unsigned int j ) = 0;
        
        //! Read a single byte from the image.
        /**
            Data must be read three bytes at a time. They are therefore queued into a read buffer. Calls to read return elements from the read buffer until it is empty, at which point it refills.
        **/
        byte read()
        {
            // If buffer is empty, fill it.
            if ( rbuffer_.size() == 0)
            {
                // Get the coordinates for the next three data bytes based on the read head position.
                unsigned int i,j;
                getBlockCoords(i,j, rhead_);
                // Read in the next three bytes
                decodeFromBlock( rbuffer_, i, j);
                // Advance the read head
                rhead_+=block_size_;
            }
            // Return and remove the first item in the buffer
            byte r = rbuffer_.front();
            rbuffer_.pop_front();
            return r;
        }
                
        //! Decode block_size_ bytes from a block of pixels. (i,j) indicates the pixel at the start of the block.
        virtual void decodeFromBlock( std::deque<byte> & data, unsigned int i, unsigned int j ) = 0;
        
        //! Write out the size of the data stored to the image.
        virtual void writeSize(unsigned int len) = 0;
        
        protected :
            
            //! Variable to determine how many bytes are stored per block
            const unsigned int block_size_;
            
            //! Helper function to decode triple modular redundancy coded bytes
            byte tripleModR( byte a, byte b, byte c) const
            {
                return ((a&b)|(a&c)|(b&c));
            }
        
        public :
            
            //! Constructor.
            BufferedConduitImage(unsigned int block_size) :
                rhead_(0),
                whead_(0),
                block_size_(block_size)
            {
                // Seed the random number generator
                std::srand ( time(NULL) );
            }
            
            //! Implant data.
            virtual void implantData( std::vector<byte>& data )
            {                
                // Format the image for implantation
                formatForImplantation();
                                
                // Check the data isn't too large, note down size
                if (data.size() > getMaxData())
                    throw ConduitImageImplantException("Too much data");
                
                // Seed the random number generator for good measure
                std::srand ( time(NULL) );
                
                // Loop through the vector and write out the data bytes to the image
                unsigned int idx = 0;
                while( idx < data.size() )
                {
                    // Write a byte to the image
                    write( data[idx] );
                    idx++;
                }
                // Now pad out with random bytes till we reach capacity
                while( idx < getMaxData() )
                {
                    // Write a (random) byte to the image
                    write( (byte) std::rand() );
                    idx++;
                }
                // Flush the buffer to ensure all data has actually been written out.
                flush();
                
                // Write the size of the data stored.
                writeSize( data.size() );
            }
            
            //! Extract data.
            virtual void extractData( std::vector<byte>& data )
            {
                // Read the length tag from the image, check it is not too large .
                unsigned int len = readSize();
                if ( len > getMaxData() )
                    throw ConduitImageImplantException("Length tag too large.");
                    
                // Reserve enough space to store the extracted data
                data.reserve( len );
                                
                // Keep reading bytes until we have the desired amount
                while( data.size() < len )
                {
                    // Read a single byte into data
                    data.push_back( read() );
                }
            }
    };
    
    
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
        
        //! Writes the size of the image (2 bytes) to the last blocks using triple modular redundancy.
        void writeSize
        (
            unsigned int len    
        )
        {
            std::deque<byte> data1, data2;
            byte hi = (byte) (len >> 8);
            byte lo = (byte) (len & 0x00ff);
            // Create three copies of the hi and lo bytes
            for (unsigned int i=0;i<3;i++) data1.push_back( hi );
            for (unsigned int i=0;i<3;i++) data2.push_back( lo );
            // Write out to the last blocks
            encodeInBlock( data1, 712, 704 );
            encodeInBlock( data2, 712, 712 );
        }
        
        public :
            
            //! Constructor.
            HaarConduitImage() :
                BufferedConduitImage(3)
            {}
            
            //! Get the maximum ammount of data (in bytes) that can be stored in this implementation.
            virtual unsigned int getMaxData()
            {
                return (90*90*3) - 6;
            }
            
            //! Check how much data (if any) is stored in the current image.
            virtual unsigned int readSize()
            {
                std::deque<byte> data1,data2;
                decodeFromBlock(data1, 712, 704 );
                decodeFromBlock(data2, 712, 712 );
                unsigned int len =
                            (tripleModR(data1[0],data1[1],data2[2]) << 8 )
                        |   (tripleModR(data2[0],data2[1],data2[2]) << 0 );
                return len;
            }
    };
    
    // Buffered conduit image abstract subclass which stores order_ bits for each 8-bit pixel value, scaled up and offset to span the 0-255 range.
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
    
    
    //! Upsampling conduit image class which uses 3 bits per 8-bit pixel.
    /**
        This class stores 3 bits per pixel with an approximate error rate of 0.015%. TODO - check this.
    */
    class Upsampled3ConduitImage : public UpsampledConduitImage
    {
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
        
        //! Writes the size of the image (3 bytes) to the last twelve pixels, using triple modular redundancy.
        void writeSize
        (
            unsigned int len    
        )
        {
            std::deque<byte> data;
            data.push_back( (byte) (len >> 16) );
            data.push_back( (byte) (len >> 8) );
            data.push_back( (byte) (len & 0x00ff) );
            //
            encodeInBlock( data, 719, 696 );
            encodeInBlock( data, 719, 704 );
            encodeInBlock( data, 719, 712 );
        }
        
        public :
            
            //! Constructor.
            Upsampled3ConduitImage() :
                UpsampledConduitImage(3,3) // block size is 3 bytes, order is 3 bits per pixel
            {}
            
            //! Get the maximum ammount of data (in bytes) that can be stored in this implementation.
            virtual unsigned int getMaxData()
            {
                return (720*720*3)/8 - 9;
            }
            
            //! Check how much data (if any) is stored in the current image.
            virtual unsigned int readSize()
            {
                std::deque<byte> data;
                decodeFromBlock(data, 719, 696 );
                decodeFromBlock(data, 719, 704 );
                decodeFromBlock(data, 719, 712 );
                unsigned int len =
                            (tripleModR(data[0],data[3],data[6]) << 16)
                        |   (tripleModR(data[1],data[4],data[7]) << 8 )
                        |   (tripleModR(data[2],data[5],data[8]) << 0 );
                return len;
            }
    };

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
        
        //! Writes the size of the image (3 bytes) to the last twelve pixels, using triple modular redundancy.
        void writeSize
        (
            unsigned int len    
        )
        {
            std::deque<byte> data1,data2,data3;
            data1.push_back( (byte) (len >> 16) );
            data2.push_back( (byte) (len >> 8) );
            data3.push_back( (byte) (len & 0x00ff) );
            //
            encodeInBlock( data1, 719, 702 );
            encodeInBlock( data2, 719, 704 );
            encodeInBlock( data3, 719, 706 );
            encodeInBlock( data1, 719, 708 );
            encodeInBlock( data2, 719, 710 );
            encodeInBlock( data3, 719, 712 );
            encodeInBlock( data1, 719, 714 );
            encodeInBlock( data2, 719, 716 );
            encodeInBlock( data3, 719, 718 );
        }
        
        public :
            
            //! Constructor.
            Upsampled4ConduitImage() :
                UpsampledConduitImage(1,4) // block size is 1 byte, order is 4 bits per pixel
            {}
            
            //! Get the maximum ammount of data (in bytes) that can be stored in this implementation.
            virtual unsigned int getMaxData()
            {
                return (720*720*4)/8 - 9;
            }
            
            //! Check how much data (if any) is stored in the current image.
            virtual unsigned int readSize()
            {
                std::deque<byte> data;
                decodeFromBlock( data, 719, 702 );
                decodeFromBlock( data, 719, 704 );
                decodeFromBlock( data, 719, 706 );
                decodeFromBlock( data, 719, 708 );
                decodeFromBlock( data, 719, 710 );
                decodeFromBlock( data, 719, 712 );
                decodeFromBlock( data, 719, 714 );
                decodeFromBlock( data, 719, 716 );
                decodeFromBlock( data, 719, 718 );
                unsigned int len =
                            (tripleModR(data[0],data[3],data[6]) << 16)
                        |   (tripleModR(data[1],data[4],data[7]) << 8 )
                        |   (tripleModR(data[2],data[5],data[8]) << 0 );
                return len;
            }
    };
}
    
#endif //EFB_CONDUITIMAGE_H
    