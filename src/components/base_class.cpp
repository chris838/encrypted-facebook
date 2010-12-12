#include "base_class.h"

using namespace cimg_library;
BZ_USING_NAMESPACE(blitz)

base::base(const std::string cd, const std::string td) : cache_dir(cd), temp_dir(td)
{
  std::cout << "Loading library..." << "\n";
  std::cout << "Cache directory: " << cache_dir << "\n";
  std::cout << "Temp directory: " << temp_dir << "\n";
}

void base::UTF8Encode2BytesUnicode(
    std::vector< Unicode2Bytes >  & input,
    std::vector< byte >           & output
) const
{
   for(unsigned int i=0; i < input.size(); i++)
   {
      // First we add an offset of 0x00b0
      // so no dealing with control or null characters
      unsigned int in = ((unsigned int) input[i]) + 0x00b0;
      
      // 0xxxxxxx
      if(in < 0x80)
      {
         output.push_back((byte)in);
      }
      // 110xxxxx 10xxxxxx
      else if(in < 0x800)
      {
         output.push_back((byte)(MASK2BYTES | (in >> 6)));
         output.push_back((byte)(MASKBYTE | (in & MASKBITS)));
      }
      else if ( (in >= 0xd800) & (in <= 0xdfff) )
      {  
        // this character range is not valid (they are surrogate pair codes)
        // so we shift 1 bit to the left and use those code points instead
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
}

void base::UTF8Decode2BytesUnicode(
    std::vector< byte >           & input,
    std::vector< Unicode2Bytes >  & output
) const
{
   for(unsigned int i=0; i < input.size();)
   {
      unsigned int ch;

      // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
      if((input[i] & MASK4BYTES) == MASK4BYTES)
      {
        if (input[i] > 0xf2) // this is a surrogate character
        {                   
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
         i += 4;
      }
      // 1110xxxx 10xxxxxx 10xxxxxx
      else if((input[i] & MASK3BYTES) == MASK3BYTES)
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
      // 0xxxxxxx
      else if(input[i] < MASKBYTE)
      {
         ch = input[i];
         i += 1;
      }
      
      // subtract 0xb0 offest
      output.push_back( (Unicode2Bytes) (ch - 0xb0) );
   }
}

unsigned int base::EncryptPhoto(
  const char*        & data_filename
)
const
{
  data_filename = "/home/chris/Desktop/data.bin"; // !!!TEMP!!!
  //
  const char* src_filename =    // source image file (to carry data signal)
  "/home/chris/Desktop/src.bmp";
  std::ifstream data_file;	// data file object
  const char* dst_filename = 	// output filename
  "/home/chris/Desktop/dst.bmp";	 
  CImg<unsigned char> * src =
  new CImg<unsigned char>(); 	// source image object
  std::vector<char>     data; 	// byte array for our data bytes we wish to transferred


  // Load the source image file into a CImg object
  try {src->load( src_filename );}
  catch (CImgInstanceException &e) {
    std::cout << "Error loading source image:" << e.what() << "\n";
    return 1;
  }


  // Load the binary file specified into a byte array
  data_file.open( data_filename, ios::binary );
  if(!data_file.is_open()) {
    std::cout << "Error opening data file:" << "\n";
    return 1;
  }
  data_file.seekg(0, std::ios::end);
  size_t data_size = data_file.tellg(); // get the length of the file
  data_file.seekg(0, std::ios::beg);
  data.resize( data_size );
  // read the file
  data_file.read(&data[0], data_size);

  // !!TODO!! Add length tag at start
  // !!TODO!! Encrypt the data bytes
  // !!TODO!! Add FEC encoding
  
  // Pad out to multiple of three (since we encode 3 bytes per block)
  while ( data.size() % 3 != 0) data.push_back( 0x0000 );


  // Encode into image using Haar DWT
  EncodeInImage( *src, data );
  

  // Generate a filename and save our final image (in a lossless format)
  //time_t rawtime; time( &rawtime );
  //std::stringstream ss;
  //ss << rawtime;
  // filename would be { ss.str + ".png" } or something
  try {src->save( dst_filename );}
  catch (CImgInstanceException &e) {
    std::cout << "Error saving output image: " << e.what() << "\n";
    return 1;
  }

  const char* s = "adsfsdf";
  const char*& ss = s;
  //DecryptPhoto( ss );

  // Return with the filename
  return 0; 
}

void base::EncodeInImage(
   CImg<unsigned char>    & img, 
   std::vector<char> 	  & data
) const
{

  // Format the source image to 720x720, 3 channel YCbCr colour, single slice
  // (resample using Lanczos)
  img.resize(720,720,1,3,6);
  //img.RGBtoYCbCr();
  
  // Create a temporary destination image
  // (need an extra sign bit after performing forward transform)
  CImg<short int> temp_img(720,720,1,3);
  
  // Loop through the image in 8x8 blocks
  unsigned int idx; int exit = false;
  for (int block_i=0; block_i<90; block_i++) {
      for (int block_j=0; block_j<90; block_j++) {
	// Loop through each channel
	for (int c=0; c<3; c++) {
	  
	  
	  // Apply the transform (two iterations)
	  Haar2D_DWT (  img , temp_img, block_i*8, block_j*8 , c);
	  Haar2D_DWT (  img , temp_img, block_i*8, block_j*8 , c);
	  
	  
	  // Write out 3 bytes of data to the approximation coefficients
	  idx = (block_i*90) + block_j;
	  if ( 3*idx+2 < data.size() ) {
	    /*EncodeInBlock(temp_img,		// target image
		     block_i*8, block_j*8, c, 	// coords of the block and channel
		     data[3*idx],		// bytes to hide
		     data[3*idx+1],		//
		     data[3*idx+2]   		// 
		     );*/
	  } else {
	    // Stop writing data
	    exit = true;
	  }  
	  
	  // Apply the inverse transform
	  Haar2D_DWTi(  temp_img, img , block_i*8, block_j*8 , c);
	  Haar2D_DWTi(  temp_img, img , block_i*8, block_j*8 , c);
	  
	  // If we finished, break out of the loop
	  if (exit) block_i=block_j=c=90;
	}
    }
  }
  
  // Convert back to RGB
  //img.YCbCrtoRGB();

}

void base::EncodeInBlock(
    cimg_library::CImg<short int> & 	img,
    unsigned int 			x0,
    unsigned int 			y0,
    unsigned int 			c,
    unsigned char 			block_a,
    unsigned char 			block_b,
    unsigned char 			block_c
) const
{
  img( x0, y0, c ) = block_a;
  img( x0+1, y0, c ) = block_b;
  img( x0, y0+1, c ) = block_c;
  img( x0+1, y0+1, c ) = 0xff;
}

unsigned int base::DecryptPhoto(
  const char*                   & data_filename
) const
{
  data_filename = "/home/chris/Desktop/data2.bin"; // !!!TEMP!!!
  //
  const char* src_filename =    // source image file (from which we read data bytes)
    "/home/chris/Desktop/dst.bmp";
  std::ofstream data_file;	// data file object	 
  CImg<unsigned char> * src =
    new CImg<unsigned char>(); 	// source image object
  std::vector<char>     data; 	// byte array for our data bytes we wish to transferred


  // Load the source image file into a CImg object
  try {src->load( src_filename );}
  catch (CImgInstanceException &e) {
    std::cout <<  "Error loading source image:" << e.what() << "\n";
    return 1;
  }

  // Decode from image using Haar DWT
  DecodeFromImage( *src, data );
  
  // !!TODO!! Correct errors
  // !!TODO!! Decrypt
  
  // Save data to a file
  data_file.open( data_filename, ios::binary);
  if(!data_file.is_open()) {
    std::cout << "Error creating data file:" << "\n";
    return 1;
  }
  data_file.write(&data[0], data.size() );

  // Return with the filename
  return 0; 
}

void base::DecodeFromImage(
    cimg_library::CImg<unsigned char>	 & img,
    std::vector<char>		         & data
) const
{

  // Format the image to Y'CbCr colour
  //img.RGBtoYCbCr();
  
  // Create a temporary destination image
  // (need an extra sign bit after performing forward transform)
  CImg<short int> temp_img(720,720,1,3);
  
  // Loop through the image in 8x8 blocks
  for (int block_i=0; block_i<90; block_i++) {
      for (int block_j=0; block_j<90; block_j++) {
	// Loop through each channel
	for (int c=0; c<3; c++) {
	  
	  // Apply the transform (two iterations)
	  Haar2D_DWT (  img , temp_img, block_i*8, block_j*8 , c);
	  Haar2D_DWT (  img , temp_img, block_i*8, block_j*8 , c);
	  
	  // Read in 3 bytes of data from the approximation coefficients
	  DecodeFromBlock(temp_img,		// target image
		     block_i*8, block_j*8, c, 	// coords of the block and channel
		     data			// vector for output bytes
		     );
	}
    }
  }
  
  // Convert back to RGB
  //img.YCbCrtoRGB();
}

void base::DecodeFromBlock(
    cimg_library::CImg<short int> & img,
    unsigned int 			x0,
    unsigned int 			y0,
    unsigned int 			c,
    std::vector<char>		& data
) const
{
  data.push_back( img( x0,   y0, c ) );
  data.push_back( img( x0+1, y0, c ) );
  data.push_back( img( x0, y0+1, c ) );
}

int div_floor(int a, int b) {
  if (a>=0) return a/b;
  else return (a-1)/b; 
}

void base::Haar2D_DWT(
	CImg<unsigned char> & src, 
	CImg<short int>  & dst,
	unsigned int x0,
	unsigned int y0,
	unsigned int c
) const
{
  // Perform the Haar Discrete Wavelet Transform (with lifting scheme so
  // the inverse can be performed losslessly). We start at pixel (x0,y0)
  // in the image and work on the subsequent 8x8 block.
  
  // For each column, perform the 1D (vertical) HDWT with integer lifting
  for (unsigned int x=x0; x<(x0+8); x++) {
    for (int i=0; i<4; i++) {
      // difference
      dst(x, y0+4+i, c)  =  src(x, y0+(2*i), c) - src(x, y0+(2*i+1), c);
      // average
      dst(x, y0+i, c)	=  div_floor( src(x, y0+(2*i), c) + src(x, y0+(2*i+1), c), 2);
    }
  }
  
  // For each row, perform the 1D (horizontal) HDWT with integer lifting
  short int temp[4];
  for (unsigned int y=y0; y<(y0+8); y++) {
    for (int i=0; i<4; i++) {
      // difference
      temp[i] 		=  dst(x0+(2*i), y, c) - dst(x0+(2*i+1), y, c);
      // average
      dst(x0+i, y, c)	= div_floor( dst(x0+(2*i), y, c) + dst(x0+(2*i+1), y, c), 2);
    }
    // copy in difference values
    for (int i=0; i<4; i++) dst(x0+4+i, y, c) = temp[i];
  }
  
}

void base::Haar2D_DWTi(
	CImg<short int> & src, 
	CImg<unsigned char>  & dst,
	unsigned int x0,
	unsigned int y0,
	unsigned int c
) const
{
   
  // Perform the inverse Haar Discrete Wavelet Transform using a lifting scheme
  // We start at pixel (x0,y0) in the image and work on the subsequent 8x8 block.

  short int temp[8];
  // For each row, perform the 1D (horizontal) inverse HDWT with integer lifting
  for (unsigned int y=y0; y<(y0+8); y++) {
    for (unsigned int i=0; i<4; i++) {
      temp[2*i]   = src(x0+i, y, c) + div_floor(src(x0+4+i, y, c)+1,2) ; // lifting scheme here
      temp[2*i+1] = temp[2*i] - src(x0+4+i, y, c);
    }
    for (int i=0; i<8; i++) src(x0+i, y, c) = temp[i];
  }

  // For each column, perform the 1D (vertical) inverse HDWT with integer lifting
  for (unsigned int x=x0; x<(x0+8); x++) {
    for (unsigned int i=0; i<4; i++) {
      dst(x, y0+2*i, c)	  = src(x, y0+i, c) + div_floor(src(x, y0+4+i, c)+1,2) ; // lifting scheme here
      dst(x, y0+2*i+1, c) = dst(x, y0+2*i, c) - src(x, y0+4+i, c);
    }
  }
  
}

base::~base()
{
  std::cout << "base::destructor\n" ;
}
