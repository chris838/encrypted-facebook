#ifndef EFB_BUFFEREDCONDUITIMAGE_H
#define EFB_BUFFEREDCONDUITIMAGE_H

// Standard library includes
#include <deque>

// Library sub-component includes
#include "IConduitImage.h"

namespace efb {
    
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
        
        protected :
            
            //! Variable to determine how many bytes are stored per block
            const unsigned int block_size_;
        
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
                rhead_ = whead_ = 0;
                                
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
            }
            
            //! Extract data.
            virtual void extractData( std::vector<byte>& data )
            {
                // Read the length tag from the image, check it is not too large .
                unsigned int len = getMaxData();
                rhead_ = whead_ = 0;
                    
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
    
}
    
#endif //EFB_BUFFEREDCONDUITIMAGE_H
    