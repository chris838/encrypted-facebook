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
  CImg<short int> * src =
  new CImg<short int>(); // source image object
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
  
  // Pad out to multiple of ten (since we encode 10 bytes per 4-block)
  while ( data.size() % 10 != 0) data.push_back( 0x0000 );


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
  DecryptPhoto( ss );

  // Return with the filename
  return 0; 
}

void base::EncodeInImage(
   CImg<short int>    	& img, 
   std::vector<char> 	& data
) const
{

  // Format the source image to 720x720, greyscale, single slice
  // (resample using Lanczos)
  img.resize(720,720,1,-1,6);
  img.channel(0);

  // Loop through the image in 16x16 blocks
  unsigned int idx; int exit = false;
  for (int block_i=0; block_i<45; block_i++) {
      for (int block_j=0; block_j<45; block_j++) {
	  
	  // Apply the transform (two iterations) to four blocks
	  for (int k1=0; k1<2; k1++) {
	    for (int k2=0; k2<2; k2++) {
	      Haar2D_DWT ( img , (2*block_i+k1)*8, (2*block_j+k2)*8 , 0);
	      Haar2D_DWT ( img , (2*block_i+k1)*8, (2*block_j+k2)*8 , 0);
	    }
	  }
	  
	  // Write out 10 bytes of data to the approximation coefficients
	  // 5 bytes to two blocks
	  idx = 10*((block_i*45) + block_j);
	  if ( idx+9 < data.size() ) {
	    // 5 bytes to the first row of Y blocks
	    EncodeInYBlocks(img, 2*block_i*8, 2*block_j*8,
		    data[idx], data[idx+1], data[idx+2], data[idx+3], data[idx+4]
		    );
	    // 5 bytes to the second row of Y blocks
	    EncodeInYBlocks(img, 2*block_i*8, (2*block_j+1)*8,
		    data[idx+5], data[idx+6], data[idx+7], data[idx+8], data[idx+9]
		    );
	  } else {
	    // Stop writing data
	    exit = true;
	  }
	  
	  // Apply the inverse transform
	  for (int k1=0; k1<2; k1++) {
	    for (int k2=0; k2<2; k2++) {
	      Haar2D_DWTi( img , (2*block_i+k1)*8, (2*block_j+k2)*8 , 0);
	      Haar2D_DWTi( img , (2*block_i+k1)*8, (2*block_j+k2)*8 , 0);
	    }
	  }
	  
	  // If we finished, break out of the loops
	  if (exit) block_i=block_j=45;
    }
  }

}

void base::EncodeInYBlocks(
    cimg_library::CImg<short int> & 	img,
    unsigned int 			x0,
    unsigned int 			y0,
    unsigned char 			a,
    unsigned char 			b,
    unsigned char 			c,
    unsigned char 			d,
    unsigned char 			e
) const
{
  img( x0, y0, 0) 	= (a & 0xf8) | 0x04;
  img( x0+1, y0, 0) 	= (b & 0xf8) | 0x04;
  img( x0, y0+1, 0) 	= (c & 0xf8) | 0x04;
  img( x0+1, y0+1, 0) 	= (d & 0xf8) | 0x04;
  img( x0+8, y0, 0) 	= (e & 0xf8) | 0x04;
  img( x0+9, y0, 0)	= (((a & 0x07) << 5) | ((b & 0x06) << 2)) | 0x04;
  img( x0+8, y0+1, 0)	= (((c & 0x07) << 5) | ((d & 0x06) << 2)) | 0x04;
  img( x0+9, y0+1, 0)	= (((e & 0x07) << 5) | ((b & 0x01) << 4) | ((d & 0x01) << 3)) | 0x04;
}

unsigned int base::DecryptPhoto(
  const char*                   & data_filename
) const
{
  data_filename = "/home/chris/Desktop/data2.bin"; // !!!TEMP!!!
  //
  const char* src_filename =    // source image file (from which we read data bytes)
    "/home/chris/Desktop/dst.jpg";
  std::ofstream data_file;	// data file object	 
  CImg<short int> * src =
    new CImg<short int>(); 	// source image object
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
  
{

//----------------------------------------------------------------------------  
// Calculate bit error rate
std::ifstream data0_file;
std::vector<char>     data0;
data0_file.open( "/home/chris/Desktop/data.bin", ios::binary );
if(!data0_file.is_open()) {
  std::cout << "Error opening data0 file:" << "\n";
  return 1;
}
data0_file.seekg(0, std::ios::end);
size_t data0_size = data0_file.tellg(); // get the length of the file
data0_file.seekg(0, std::ios::beg);
data0.resize( data0_size );
// read the file
data0_file.read(&data0[0], data0_size);
int total, errors;
for (unsigned int i=0; i<data0.size(); i++) {
  unsigned char error = data0[i] ^ data[i];
  bitset<8> bs( error );
  errors += bs.count();
  total += 8;
}
std::cout << ((float) errors) / total << "\n";
//----------------------------------------------------------------------------

}
  
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
    cimg_library::CImg<short int>	 & img,
    std::vector<char>		         & data
) const
{
	    
  // Loop through the image in 16x16 blocks
  for (int block_i=0; block_i<45; block_i++) {
      for (int block_j=0; block_j<45; block_j++) {
	  	    
	  // Apply the transform (two iterations) to four blocks
	  for (int k1=0; k1<2; k1++) {
	    for (int k2=0; k2<2; k2++) {
	      Haar2D_DWT ( img , (2*block_i+k1)*8, (2*block_j+k2)*8 , 0);
	      Haar2D_DWT ( img , (2*block_i+k1)*8, (2*block_j+k2)*8 , 0);
	    }
	  }
	    
	  // Read in 10 bytes of data from the approximation coefficients
	  // 5 bytes from the first row of Y blocks
	  DecodeFromYBlocks( img, 2*block_i*8, 2*block_j*8, data);
	  // 5 bytes from the second row of Y blocks
	  DecodeFromYBlocks( img, 2*block_i*8, (2*block_j+1)*8, data);
    }
  }
}

void base::DecodeFromYBlocks(
    cimg_library::CImg<short int> & img,
    unsigned int 			x0,
    unsigned int 			y0,
    std::vector<char>		& data
) const
{
  unsigned char p1,p2,p3,p4,p5,p6,p7,p8;
  p1 = img( x0, y0, 0 );
  p2 = img( x0+1, y0, 0 );
  p3 = img( x0, y0+1, 0 );
  p4 = img( x0+1, y0+1, 0 );
  p5 = img( x0+8, y0, 0 );
  p6 = img( x0+9, y0, 0 );
  p7 = img( x0+8, y0+1, 0 );
  p8 = img( x0+9, y0+1, 0 );
  
  unsigned char a,b,c,d,e;
  a = (p1 & 0xf8) | ((p6 & 0xe0) >> 5);
  b = (p2 & 0xf8) | ((p6 & 0x18) >> 2) | ((p8 & 0x10) >> 4 );
  c = (p3 & 0xf8) | ((p7 & 0xe0) >> 5);
  d = (p4 & 0xf8) | ((p7 & 0x18) >> 2) | ((p8 & 0x08) >> 3 );
  e = (p5 & 0xf8) | ((p8 & 0xe0) >> 5);
  
  data.push_back( a );
  data.push_back( b );
  data.push_back( c );
  data.push_back( d );
  data.push_back( e );
}

int div_floor(int a, int b) {
  if (a>=0) return a/b;
  else return (a-1)/b; 
}

void base::Haar2D_DWT(
	CImg<short int> & img, 
	unsigned int x0,
	unsigned int y0,
	unsigned int c
) const
{
  // Perform the Haar Discrete Wavelet Transform (with lifting scheme so
  // the inverse can be performed losslessly). We start at pixel (x0,y0)
  // in the image and work on the subsequent 8x8 block.
  
  // Temp array for calculations
  short int temp[4];
  
  // For each column, perform the 1D (vertical) HDWT with integer lifting
  for (unsigned int x=x0; x<(x0+8); x++) {
    for (int i=0; i<4; i++) {
      // difference
      temp[i] 		=  img(x, y0+(2*i), c) - img(x, y0+(2*i+1), c);
      // average
      img(x, y0+i, c)	=  div_floor( img(x, y0+(2*i), c) + img(x, y0+(2*i+1), c), 2);
    }
    for (int i=0; i<4; i++) img(x, y0+4+i, c) = temp[i];
  }
  
  // For each row, perform the 1D (horizontal) HDWT with integer lifting
  for (unsigned int y=y0; y<(y0+8); y++) {
    for (int i=0; i<4; i++) {
      // difference
      temp[i] 		=  img(x0+(2*i), y, c) - img(x0+(2*i+1), y, c);
      // average
      img(x0+i, y, c)	=  div_floor( img(x0+(2*i), y, c) + img(x0+(2*i+1), y, c), 2);
    }
    // copy in difference values
    for (int i=0; i<4; i++) img(x0+4+i, y, c) = temp[i];
  }
  
}

void base::Haar2D_DWTi(
	CImg<short int> & img,
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
      temp[2*i]   = img(x0+i, y, c);// + div_floor(img(x0+4+i, y, c)+1,2) ; // lifting scheme here
      temp[2*i+1] = temp[2*i];//- img(x0+4+i, y, c);
    }
    for (int i=0; i<8; i++) img(x0+i, y, c) = temp[i];
  }

  // For each column, perform the 1D (vertical) inverse HDWT with integer lifting
  for (unsigned int x=x0; x<(x0+8); x++) {
    for (unsigned int i=0; i<4; i++) {
      temp[2*i]   = img(x, y0+i, c);// + div_floor(img(x, y0+4+i, c)+1,2) ; // lifting scheme here
      temp[2*i+1] = temp[2*i];// - img(x, y0+4+i, c);
    }
    for (int i=0; i<8; i++) img(x, y0+i, c) = temp[i];
  }
  
}

base::~base()
{
  std::cout << "base::destructor\n" ;
}
