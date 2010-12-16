#include "base_class.h"

#include "rs/schifra_galois_field.hpp"
#include "rs/schifra_galois_field_polynomial.hpp"
#include "rs/schifra_sequential_root_generator_polynomial_creator.hpp"
#include "rs/schifra_reed_solomon_encoder.hpp"
#include "rs/schifra_reed_solomon_decoder.hpp"
#include "rs/schifra_reed_solomon_block.hpp"
#include "rs/schifra_error_processes.hpp"

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
  const char*	& data_filename,
  const char* 	& dst_filename
)
const
{
  const char* src_filename =    		 // source image file (to carry data signal)
  "/home/chris/Desktop/src.bmp";
  std::ifstream data_file;			 // data file object	 
  CImg<short int> * src = new CImg<short int>(); // source image object
  std::deque<char>     data; 			 // byte array for our data bytes we wish to transferred


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
  // read the file
  std::vector<char> data_vec( data_size );
  data_file.read(&data_vec[0], data_size);
  for (unsigned int i=0; i<data_size;i++) data.push_back( data_vec[i] );
  
  
  // Check size
  if (data.size() > 21185) {
    std::cout << "Error - too much data:" << "\n";
    return 2;
  }

  // !!TODO!! Encrypt the data bytes
  
  
  // Pad out to multiple of 223 since this is the FEC block size
  srand( time(NULL) );
  while ( data.size() % 223 != 0) data.push_back( (char) rand() );
  
  
  // Add Reed-Solomon FEC encoding
  std::string m, f; std::deque<char> data_fec;
  for (deque<char>::iterator i = data.begin(); i!=data.end(); i+=223) {
    m = std::string( i, i+223 );
    ReedSolomonEncoder( m, f );
    for (unsigned int i=0;i<223;i++) data_fec.push_back(m[i]);
    for (unsigned int i=0;i<32;i++) data_fec.push_back( f[i]);;
  }
  data = data_fec;
  
  
  // Prepend 6-byte length tag, encoded with triple modular redundancy
  // Note that we store the original data size, before FEC and padding
  unsigned short len = (unsigned short) data_size;
  unsigned char len_hi,len_lo;
  len_hi = (unsigned char) (len >> 8);
  len_lo = (unsigned char) len;
  for (int i=0; i<3; i++) data.push_front( len_lo);
  for (int i=0; i<3; i++) data.push_front( len_hi);
  

  // Double check size
  if (data.size() > 90*90*3) {
    std::cout << "Error - too much data:" << "\n";
    return 2;
  }

  
    // Pad out to full image size
  while ( data.size() < 90*90*3 ) data.push_back( (char) rand() );

  
  // Encode into image using Haar DWT
  EncodeInImage( *src, data );
  

  // Generate a filename and save our final image (in a lossless format)
  try {src->save( dst_filename );}
  catch (CImgInstanceException &e) {
    std::cout << "Error saving output image: " << e.what() << "\n";
    return 1;
  }

  // Return with succes
  return 0; 
}

void base::EncodeInImage(
   CImg<short int>    	& img, 
   std::deque<char> 	& data
) const
{

  // Format the source image to 720x720, greyscale, single slice
  // (resample using Lanczos)
  img.resize(720,720,1,-1,6);
  img.channel(0);

  // Loop through the image in 8x8 blocks
  unsigned int idx, i, j; int exit = false;
  for (int block_i=0; block_i<90; block_i++) {
    for (int block_j=0; block_j<90; block_j++) {
	
	i = block_i*8; j = block_j*8;
	
	// Apply the transform to the block (two iterations)
	Haar2D_DWT (img , i, j);
	Haar2D_DWT (img , i, j);
	
	// Write out 3 bytes of data to the approximation coefficients
	idx = 3*((block_i*90) + block_j);
	if ( idx+2 < data.size() ) {
	  // 3 bytes to one 8x8 block
	  EncodeInBlock(img, i, j, data[idx], data[idx+1], data[idx+2] );
	} else {
	  // Stop writing data
	  exit = true;
	}
	
	// Apply the inverse transform
	Haar2D_DWTi(img , i, j);
	Haar2D_DWTi(img , i, j);
	
	// If we finished, break out of the loops
	if (exit) block_i=block_j=90;
    }
  }
}

void base::EncodeInBlock(
    cimg_library::CImg<short int> & 	img,
    unsigned int 			x0,
    unsigned int 			y0,
    unsigned char 			a,
    unsigned char 			b,
    unsigned char 			c
) const
{  
  img( x0, y0 ) 	= (a & 0xfc) | 0x02;
  img( x0+1, y0 ) 	= (b & 0xfc) | 0x02;
  img( x0, y0+1 ) 	= (c & 0xfc) | 0x02;
  img( x0+1, y0+1 ) 	= ((a & 0x03) <<6) | ((b & 0x03) <<4) | ((c & 0x03) <<2) | 0x02;
}

unsigned char triple_mod_r(
  unsigned char a,
  unsigned char b,
  unsigned char c
)
{
  return ((a&b)|(a&c)|(b&c));
}

unsigned int base::DecryptPhoto(
  const char*                   & src_filename,
  const char*                   & data_filename
) const
{

  std::ofstream     data_file;				// data file object	 
  CImg<short int> * src =  new CImg<short int>(); 	// source image object
  std::deque<char>  data; 				// for data bytes we wish to transfer


  // Load the source image file into a CImg object
  try {src->load( src_filename );}
  catch (CImgInstanceException &e) {
    std::cout <<  "Error loading source image:" << e.what() << "\n";
    return 1;
  }
  
  // Decode the first two blocks to get data length
  std::deque<char> len_blocks;
  Haar2D_DWT ( *src , 0, 0 );
  Haar2D_DWT ( *src , 0, 0 );
  Haar2D_DWT ( *src , 0, 8 );
  Haar2D_DWT ( *src , 0, 8 );
  DecodeFromBlock( *src, 0, 0, len_blocks);
  DecodeFromBlock( *src, 0, 8, len_blocks);
  unsigned char len_hi, len_lo;
  len_hi = triple_mod_r( len_blocks[0], len_blocks[1], len_blocks[2] );
  len_lo = triple_mod_r( len_blocks[3], len_blocks[4], len_blocks[5] );
  unsigned short len = (len_hi << 8) | len_lo;
  // also want the length with fec
  unsigned short len_wfec =  (len%223==0 ? len : (len-(len%223))+223); // padding
  len_wfec = (len_wfec / 223) * 255;


  // Decode from image using Haar DWT
  DecodeFromImage( *src, data, (unsigned int) len_wfec+6 );
  
  
  // Remove the first 6 bytes which encode the data length
  for (int i=0; i<6; i++) data.pop_front();
  
  
  // Remove any padding bytes at the end
  while (data.size() > len_wfec) data.pop_back();
  
  
  // Correct errors
  std::string m, f, m2; std::deque<char> data_corrected;
  for (deque<char>::iterator i = data.begin(); i!=data.end(); i+=255) {
    m = std::string( i, i+223 );
    f = std::string( i+223, i+255 );
    ReedSolomonDecoder( m, f, m2 );
    for (unsigned int i=0;i<223;i++) data_corrected.push_back(m2[i]);
  }
  data = data_corrected;
  
  // Remove FEC padding at the end
  while (data.size() > len) data.pop_back();
  
  // !!TODO!! Decrypt
  
    
  // Save data to a file
  data_file.open( data_filename, ios::binary);
  if(!data_file.is_open()) {
    std::cout << "Error creating data file:" << "\n";
    return 1;
  }
  std::vector<char> data_vec( data.size() );
  for (unsigned int i=0; i<data.size();i++) data_vec[i] = data[i];

  data_file.write(&data_vec[0], data_vec.size() );

  // Return with the filename
  return 0; 
}

void base::DecodeFromImage(
    cimg_library::CImg<short int>	 & img,
    std::deque<char>		         & data,
    unsigned int			   len
) const
{
	    
  // Loop through the image in 8x8 blocks
  int i, j; unsigned int idx;
  for (int block_i=0; block_i<90; block_i++) {
      for (int block_j=0; block_j<90; block_j++) {
	
	  i = block_i*8; j = block_j*8;
	  
	  // Apply the transform (two iterations) to the block
	  Haar2D_DWT ( img , i, j );
	  Haar2D_DWT ( img , i, j );
	    
	  // Read in 3 bytes of data from the approximation coefficients
	  DecodeFromBlock( img, i, j, data);
	  
	  // exit if we have enough data
	  idx = 3*((block_i*90) + block_j);
	  if ( idx+2 > len) block_i = block_j = 90;
    }
  }
}

void base::DecodeFromBlock(
    cimg_library::CImg<short int> & img,
    unsigned int 			x0,
    unsigned int 			y0,
    std::deque<char>		& data
) const
{
  unsigned char p1,p2,p3,p4;
  p1 = img( x0, y0 );
  p2 = img( x0+1, y0 );
  p3 = img( x0, y0+1 );
  p4 = img( x0+1, y0+1 );
  
  unsigned char a,b,c;
  a = (p1 & 0xfc) | ((p4 & 0xc0) >> 6);
  b = (p2 & 0xfc) | ((p4 & 0x30) >> 4);
  c = (p3 & 0xfc) | ((p4 & 0x0c) >> 2);
  
  data.push_back( a );
  data.push_back( b );
  data.push_back( c );
}

int div_floor(int a, int b) {
  if (a>=0) return a/b;
  else return (a-1)/b; 
}

void base::Haar2D_DWT(
	CImg<short int> & img, 
	unsigned int x0,
	unsigned int y0
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
      temp[i] 		=  img(x, y0+(2*i)) - img(x, y0+(2*i+1));
      // average
      img(x, y0+i)	=  div_floor( img(x, y0+(2*i)) + img(x, y0+(2*i+1)), 2);
    }
    for (int i=0; i<4; i++) img(x, y0+4+i) = temp[i];
  }
  
  // For each row, perform the 1D (horizontal) HDWT with integer lifting
  for (unsigned int y=y0; y<(y0+8); y++) {
    for (int i=0; i<4; i++) {
      // difference
      temp[i] 		=  img(x0+(2*i), y) - img(x0+(2*i+1), y);
      // average
      img(x0+i, y)	=  div_floor( img(x0+(2*i), y) + img(x0+(2*i+1), y), 2);
    }
    // copy in difference values
    for (int i=0; i<4; i++) img(x0+4+i, y) = temp[i];
  }
  
}

void base::Haar2D_DWTi(
	CImg<short int> & img,
	unsigned int x0,
	unsigned int y0
) const
{
  // Perform the inverse Haar Discrete Wavelet Transform using a lifting scheme
  // We start at pixel (x0,y0) in the image and work on the subsequent 8x8 block.
  short int temp[8], p1, p2;
  // For each row, perform the 1D (horizontal) inverse HDWT with integer lifting
  for (unsigned int y=y0; y<(y0+8); y++) {
    for (unsigned int i=0; i<4; i++) {
      // Check we don't overflow the pixel
      p1 	= img(x0+i, y) + div_floor(img(x0+4+i, y)+1,2) ; // lifting scheme here
      p2 	= p1 - img(x0+4+i, y);
      if ((p1>255) || (p1<0) || (p2>255) || (p2<0)) {
	p1 = p2 = img(x0+i, y);
      }
      temp[2*i]	  = p1;
      temp[2*i+1] = p2;
    }
    for (int i=0; i<8; i++) img(x0+i, y) = temp[i];
  }

  // For each column, perform the 1D (vertical) inverse HDWT with integer lifting
  for (unsigned int x=x0; x<(x0+8); x++) {
    for (unsigned int i=0; i<4; i++) {
      p1   	= img(x, y0+i) + div_floor(img(x, y0+4+i)+1,2) ; // lifting scheme here
      p2	= p1 - img(x, y0+4+i);
      if ((p1>255) || (p1<0) || (p2>255) || (p2<0)) {
	p1 = p2 = img(x, y0+i);
      }
      temp[2*i]	  = p1;
      temp[2*i+1] = p2;
    }
    for (int i=0; i<8; i++) img(x, y0+i) = temp[i];
  }
  
}


unsigned int base::CalculateBER(
      const char*                   & file1_path,
      const char*                   & file2_path
) const
{
  
  // ifstream objects
  std::ifstream file1, file2;
  
  // open file 1
  file1.open( file1_path, ios::binary );
  if(!file1.is_open()) {
    std::cout << "Error opening data file 1:" << "\n";
    return 1;
  }
  
  // and file 2
  file2.open( file2_path, ios::binary );
  if(!file2.is_open()) {
    std::cout << "Error opening data file 2:" << "\n";
    return 1;
  }
  
  // vector byte arrays 
  std::vector<char> data1, data2;
  
  // load file 1 into array
  file1.seekg(0, std::ios::end);
  size_t file1_size = file1.tellg(); // get the length of the file
  file1.seekg(0, std::ios::beg);
  data1.resize( file1_size );
  // read into array
  file1.read(&data1[0], file1_size);
  
  // load file 2 into array
  file2.seekg(0, std::ios::end);
  size_t file2_size = file2.tellg(); // get the length of the file
  file2.seekg(0, std::ios::beg);
  data2.resize( file2_size );
  // read into array
  file2.read(&data2[0], file2_size);
  
  // Calculate the BER and output to std::cout
  unsigned int total=0, errors=0;
  for (unsigned int i=0; i<data1.size(); i++) {
    unsigned char error = data1[i] ^ data2[i];
    bitset<8> bs( error );
    errors += bs.count();
    total += 8;
  }
  std::cout << ((float) errors) / total << "\n";

  return 0;
}
unsigned int base::ReedSolomonEncoder(
      std::string  	& message,
      std::string	& fec
) const
{

  /* Finite Field Parameters */
  const std::size_t field_descriptor                 =   8;
  const std::size_t generator_polynommial_index      =   120;
  const std::size_t generator_polynommial_root_count =   32;
  
  /* Reed Solomon Code Parameters */
  const std::size_t code_length = 255;
  const std::size_t fec_length  =  32;
  const std::size_t data_length = code_length - fec_length;

   /* Instantiate Finite Field and Generator Polynomials */
   schifra::galois::field field(field_descriptor,
                                schifra::galois::primitive_polynomial_size06,
                                schifra::galois::primitive_polynomial06);

   schifra::galois::field_polynomial generator_polynomial(field);

   schifra::sequential_root_generator_polynomial_creator(field,
                                                         generator_polynommial_index,
                                                         generator_polynommial_root_count,
                                                         generator_polynomial);

   /* Instantiate Encoder and Decoder (Codec) */
   schifra::reed_solomon::encoder<code_length,fec_length> encoder(field,generator_polynomial);

   message = message + std::string(data_length - message.length(),static_cast<unsigned char>(0x00));

   /* Instantiate RS Block For Codec */
   schifra::reed_solomon::block<code_length,fec_length> block;

   /* Transform message into Reed-Solomon encoded codeword */
   if (!encoder.encode(message,block))
   {
      std::cout << "Error - Critical encoding failure!" << std::endl;
      return 1;
   }
   
   fec = std::string(fec_length,static_cast<unsigned char>(0x00));
   block.fec_to_string(fec);

   return 0;
}

unsigned int base::ReedSolomonDecoder(
      std::string  	& message_plus_errors,
      std::string	& fec,
      std::string	& message
) const
{
  /* Finite Field Parameters */
  const std::size_t field_descriptor                 =   8;
  const std::size_t generator_polynommial_index      =   120;
  const std::size_t generator_polynommial_root_count =   32;
  
  /* Reed Solomon Code Parameters */
  const std::size_t code_length = 255;
  const std::size_t fec_length  =  32;
  const std::size_t data_length = code_length - fec_length;
  
  /* Instantiate Finite Field and Generator Polynomials */
  schifra::galois::field field(field_descriptor,
			       schifra::galois::primitive_polynomial_size06,
			       schifra::galois::primitive_polynomial06);
  schifra::galois::field_polynomial generator_polynomial(field);
  schifra::sequential_root_generator_polynomial_creator(field,
							generator_polynommial_index,
							generator_polynommial_root_count,
							generator_polynomial);
  
  /* Instantiate Decoder (Codec) */
  schifra::reed_solomon::decoder<code_length,fec_length> decoder(field,generator_polynommial_index);
  
  /* Instantiate RS Block For Codec */
  schifra::reed_solomon::block<code_length,fec_length> block(message_plus_errors, fec);
  
  if (!decoder.decode(block))
  {
     std::cout << "Error - Critical decoding failure!" << std::endl;
     return 1;
  }
  
  message = std::string(data_length,static_cast<unsigned char>(0x00));
  block.data_to_string(message);
  
  return 0;
}

base::~base()
{
  std::cout << "base::destructor\n" ;
}
