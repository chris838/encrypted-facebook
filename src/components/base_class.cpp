#include "base_class.h"

using namespace cimg_library; 

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
  "/home/chris/Desktop/src.png";
  std::ifstream data_file;	// data file object
  const char* dst_filename = 	// output filename
  "/home/chris/Desktop/dst.png";	 
  CImg<unsigned char> src; 	// source image object
  std::vector<char> data; 	// byte array for our data bytes we wish to transferred 
  CImg<unsigned char> dst;	// output image object


  // Load the source image file into a CImg object
  try {src.load( src_filename );}
  catch (CImgInstanceException &e) {
    std::cout << "Error loading source image:" << e.what() << "\n";
    return 1;
  }


  // Load the binary file specified into a byte array
  data_file.open( data_filename );
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


  // !!TODO!! Encrypt the data bytes
  // !!TODO!! Add FEC encoding


  // Encode into image using Haar DWT
  dst =  EncodeInImage( src, data );


  // Generate a filename and save our final image (in a lossless format)
  //time_t rawtime; time( &rawtime );
  //std::stringstream ss;
  //ss << rawtime;
  // filename would be { ss.str + ".png" } or something
  try {dst.save( dst_filename );}
  catch (CImgInstanceException &e) {
    std::cout << "Error saving destination image: " << e.what() << "\n";
    return 1;
  }


  // Return with the filename
  return 0; 
}

CImg<unsigned char> base::EncodeInImage(
   CImg<unsigned char>  & src, 
   std::vector<char> 	& data
) const
{

// Create a destination image
CImg<unsigned char> * dst = new CImg<unsigned char>(720,720,1,3); 

// Format the source image to 720x720, 3 channel YCbCr colour, single slice
// (resample using Lanczos)
src.resize(720,720,1,3,6);
src.RGBToYCbCr();

// Loop through the image in 8x8 blocks
for (int block_i=0; block_i<90; block_i++) {
    for (int block_j=0; block_j<90; block_j++) {
        

    }
}

   return dst;
}

base::~base()
{
  std::cout << "base::destructor\n" ;
}
