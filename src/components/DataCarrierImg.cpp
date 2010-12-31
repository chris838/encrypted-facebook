#include "DataCarrierImg.h"

//! Format the image in preparation for implantation.
/**
    This operation will resize the image to 720x720x1 and truncate the colour channels, as only data
    storage in single-channel (greyscale) image is supported. JPEG compression requires an
    (imperfect) colour space transform from RGB to YCrCb which complicates using colour images for
    data storage. Even worse - Facebook's JPEG compression process uses chrominance subsampling.
    However, this does mean that discarding the additional two chrominance channels only results
    in a %50 reduction in maximum potential data storage capacity.        
 */
void
DataCarrierImg::format_for_implantation()
{
    // Format the image to 720x720, greyscale, single slice (resample using Lanczos)
    resize(720,720,1,-1,6);
    channel(0);
    
    _is_formatted = true;
}


//! Implant a specified vector<char> into the image.
/**
    This operation implants a data vector into the image, using 3 bytes per 8x8 pixel block. The final
    two blocks are reserved for storing the length of the data vector, so it may be easily recovered.
    Unused blocks are padded with random data. This function may throw
    DataCarrierImgExtractionException, normally if the data provided is too large to store.
    /param data The vector<char> that we wish to implant in the image. Must be no more than
    24294 bytes (90x90 blocks, 3 bytes per block, subtract 6-byte length tag).       
 */
void
DataCarrierImg::implant_data
(
    std::vector<char>& data
)
{
    // Check the image is properly formatted
    if (!_is_formatted)
        throw DataCarrierImgImplantationException("Error whilst implanting: image not formatted. ",
                                                  "Was 'format_for_implantation' called?");
        
    // Check the data isn't too large, note down size and pad out to a multiple of three
    if (data.size() > _max_data)
        throw DataCarrierImgImplantationException("Error whilst implanting: too much data");
    unsigned short len = data.size(); // store this for length tag later
    std::srand ( time(NULL) );
    while ( data.size() % 3 != 0) data.push_back( (char) std::rand() ); // random padding
    
    // Loop through the image in 8x8 blocks
    unsigned int idx, i, j;
    for (int block_i=0; block_i<90; block_i++) {
      for (int block_j=0; block_j<90; block_j++) {
          
          i = block_i*8; j = block_j*8;
          // Apply the Haar transform to the block (two iterations)
          Haar2D_DWT (i, j);
          Haar2D_DWT (i, j);
          // Write out 3 bytes of data to the approximation coefficients
          idx = 3*((block_i*90) + block_j);
          if ( idx+2 < data.size() ) {
              // 3 bytes to one 8x8 block
              EncodeInBlock(i, j, data[idx], data[idx+1], data[idx+2] );
          } else {
              // finish with random bytes
              EncodeInBlock(i, j, (char) std::rand(), (char) std::rand(), (char) std::rand());
          }
          // Apply the inverse transform
          Haar2D_DWTi(i, j);
          Haar2D_DWTi(i, j);
      }    
    }
  
    // Write the size of the data stored to the last two image blocks
    write_size(len);

}

//! Writes the size of the image (2 bytes) to the last two blocks, using triple modular redundancy.
void
DataCarrierImg::write_size
(
    unsigned short len    
)
{
    unsigned char len_hi,len_lo;
    len_hi = (unsigned char) (len >> 8);
    len_lo = (unsigned char) len;
    // Apply the Haar transform to the two blocks (two iterations)
    Haar2D_DWT (89*8, 88*8);
    Haar2D_DWT (89*8, 88*8);
    Haar2D_DWT (89*8, 89*8);
    Haar2D_DWT (89*8, 89*8);
    // Write in each half of the 2-byte length tag three times over
    EncodeInBlock(89*8, 88*8, len_hi, len_hi, len_hi );
    EncodeInBlock(89*8, 89*8, len_lo, len_lo, len_lo );
    // Apply the inverse transform
    Haar2D_DWTi(89*8, 88*8);
    Haar2D_DWTi(89*8, 88*8);
    Haar2D_DWTi(89*8, 89*8);
    Haar2D_DWTi(89*8, 89*8);
}

//! Encode 3 bytes of data to the Haar coefficients of an 8x8 block
/**
    This operation assumes we have performed the Haar transform (two iterations) on the block in
    question. We store the provided 3 bytes of data in the most significant bits of 4 Haar coefficents
    which represent the low-frequency component of the image.
 */
void
DataCarrierImg::EncodeInBlock
(
    unsigned int 			x0,
    unsigned int 			y0,
    unsigned char 			a,
    unsigned char 			b,
    unsigned char 			c
)
{  
  operator()( x0, y0 ) 	= (a & 0xfc) | 0x02;
  operator()( x0+1, y0 ) 	= (b & 0xfc) | 0x02;
  operator()( x0, y0+1 ) 	= (c & 0xfc) | 0x02;
  operator()( x0+1, y0+1 ) 	= ((a & 0x03) <<6) | ((b & 0x03) <<4) | ((c & 0x03) <<2) | 0x02;
}

//! Attempt to allocate and return a new vector<char> containing the extracted data.
/**
    This operation allocates a new vector<char> big of a size denoted by the
    length tag at the end (last two 8x8 pixel blocks) of the image. Extraction of the data is
    then attempted. This function may well throw DataCarrierImgExtractionException since the image
    may be a normal Facebook image and not contain anything.
 */
void
DataCarrierImg::extract_data
(
    std::vector<char>&  data
)
{
    // Read the length tag from the image, check it is not too large .
    unsigned short len = read_size();
    if (len > _max_data )
        throw DataCarrierImgImplantationException("Extraction failed: length tag too large.");
        
    // Reserve enough space to store the extracted data + possible padding
    data.reserve( len+3 );
    
    // Loop through the image in 8x8 blocks
    int i, j; unsigned int idx;
    for (int block_i=0; block_i<90; block_i++) {
        for (int block_j=0; block_j<90; block_j++) {
          
            i = block_i*8; j = block_j*8;
            
            // Apply the transform (two iterations) to the block
            Haar2D_DWT ( i, j );
            Haar2D_DWT ( i, j );
            // Read in 3 bytes of data from the approximation coefficients
            DecodeFromBlock( i, j, data);
            // exit if we have enough data
            idx = 3*((block_i*90) + block_j);
            if ( idx+2 > len) block_i = block_j = 90;
      }
    }
    
    // Remove any padding bytes at the end
    while (data.size() > len) data.pop_back();
}


unsigned short
DataCarrierImg::read_size()
{
    std::vector<char> len_blocks;
    Haar2D_DWT ( 89*8, 88*8 );
    Haar2D_DWT ( 89*8, 88*8 );
    Haar2D_DWT ( 89*8, 89*8 );
    Haar2D_DWT ( 89*8, 89*8 );
    DecodeFromBlock( 89*8, 88*8, len_blocks);
    DecodeFromBlock( 89*8, 89*8, len_blocks);
    Haar2D_DWTi( 89*8, 88*8 );
    Haar2D_DWTi( 89*8, 88*8 );
    Haar2D_DWTi( 89*8, 89*8 );
    Haar2D_DWTi( 89*8, 89*8 );
    unsigned char len_hi, len_lo;
    len_hi = triple_mod_r( len_blocks[0], len_blocks[1], len_blocks[2] );
    len_lo = triple_mod_r( len_blocks[3], len_blocks[4], len_blocks[5] );
    
    return ((len_hi << 8) | len_lo);
}

unsigned char
DataCarrierImg::triple_mod_r
(
  unsigned char a,
  unsigned char b,
  unsigned char c
) const
{
  return ((a&b)|(a&c)|(b&c));
}

void
DataCarrierImg::DecodeFromBlock(
    unsigned int 		x0,
    unsigned int 		y0,
    std::vector<char>		& data
) const
{
    unsigned char p1,p2,p3,p4;
    p1 = operator()( x0, y0 );
    p2 = operator()( x0+1, y0 );
    p3 = operator()( x0, y0+1 );
    p4 = operator()( x0+1, y0+1 );
    
    unsigned char a,b,c;
    a = (p1 & 0xfc) | ((p4 & 0xc0) >> 6);
    b = (p2 & 0xfc) | ((p4 & 0x30) >> 4);
    c = (p3 & 0xfc) | ((p4 & 0x0c) >> 2);
    
    data.push_back( a );
    data.push_back( b );
    data.push_back( c );
}


//! Perform the Haar Discrete Wavelet transform on an 8x8 block.
/**
    Perform the Haar Discrete Wavelet Transform (with lifting scheme so
    the inverse can be performed losslessly). We start at pixel (x0,y0)
    in the image and work on the subsequent 8x8 block.
*/
void
DataCarrierImg::Haar2D_DWT(
	unsigned int x0,
	unsigned int y0
)
{  
  // Temp array for calculations
  short int temp[4];
  
  // For each column, perform the 1D (vertical) HDWT with integer lifting
  for (unsigned int x=x0; x<(x0+8); x++) {
    for (int i=0; i<4; i++) {
      // difference
      temp[i] 		=  operator()(x, y0+(2*i)) - operator()(x, y0+(2*i+1));
      // average
      operator()(x, y0+i)	=  div_floor( operator()(x, y0+(2*i)) + operator()(x, y0+(2*i+1)), 2);
    }
    for (int i=0; i<4; i++) operator()(x, y0+4+i) = temp[i];
  }
  
  // For each row, perform the 1D (horizontal) HDWT with integer lifting
  for (unsigned int y=y0; y<(y0+8); y++) {
    for (int i=0; i<4; i++) {
      // difference
      temp[i] 		=  operator()(x0+(2*i), y) - operator()(x0+(2*i+1), y);
      // average
      operator()(x0+i, y)	=  div_floor( operator()(x0+(2*i), y) + operator()(x0+(2*i+1), y), 2);
    }
    // copy in difference values
    for (int i=0; i<4; i++) operator()(x0+4+i, y) = temp[i];
  }
  
}


//! Helper function for lifting scheme
int DataCarrierImg::div_floor(int a, int b) const {
  if (a>=0) return a/b;
  else return (a-1)/b; 
}


//! Perform the inverse Haar Discrete Wavelet transform on an 8x8 block.
/**
    Perform the inverse Haar Discrete Wavelet Transform using a lifting scheme
    We start at pixel (x0,y0) in the image and work on the subsequent 8x8 block.
*/
void
DataCarrierImg::Haar2D_DWTi(
	unsigned int x0,
	unsigned int y0
)
{
  // Temporary array and ints for calculations
  short int temp[8], p1, p2;
  
  // For each row, perform the 1D (horizontal) inverse HDWT with integer lifting
  for (unsigned int y=y0; y<(y0+8); y++) {
    for (unsigned int i=0; i<4; i++) {
      // Check we don't overflow the pixel
      p1 	= operator()(x0+i, y) + div_floor(operator()(x0+4+i, y)+1,2) ; // lifting scheme here
      p2 	= p1 - operator()(x0+4+i, y);
      if ((p1>255) || (p1<0) || (p2>255) || (p2<0)) {
	p1 = p2 = operator()(x0+i, y);
      }
      temp[2*i]	  = p1;
      temp[2*i+1] = p2;
    }
    for (int i=0; i<8; i++) operator()(x0+i, y) = temp[i];
  }

  // For each column, perform the 1D (vertical) inverse HDWT with integer lifting
  for (unsigned int x=x0; x<(x0+8); x++) {
    for (unsigned int i=0; i<4; i++) {
      p1   	= operator()(x, y0+i) + div_floor(operator()(x, y0+4+i)+1,2) ; // lifting scheme here
      p2	= p1 - operator()(x, y0+4+i);
      if ((p1>255) || (p1<0) || (p2>255) || (p2<0)) {
	p1 = p2 = operator()(x, y0+i);
      }
      temp[2*i]	  = p1;
      temp[2*i+1] = p2;
    }
    for (int i=0; i<8; i++) operator()(x, y0+i) = temp[i];
  }
  
}
