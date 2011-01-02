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
    const PubKey                pubkeys[],
    const unsigned int 	        pk_len,
    const char*		    &   data_filename,
    const char* 	    &   dst_filename
)
const
{ 
    const char* src_filename =  "/home/chris/Desktop/src.bmp"; // source image file (to carry data signal)
    std::ifstream 	data_file;		 // data file object	 
    DataCarrierImg * 	src = new DataCarrierImg(); // source image object
    std::vector<char>     data; 		 	 // byte array for our data bytes we wish to transferred
    data.reserve(90*90*3);			 // we know the max number of items possible to store
    unsigned int 		head_size, data_size;		 // size of the raw data we are sending
    
    
    // Generate a message key and random IV
    Botan::LibraryInitializer init;
    Botan::AutoSeeded_RNG rng;
    Botan::SymmetricKey key(rng, 16); // a random 128-bit key
    Botan::InitializationVector iv(rng, 16); // a random 128-bit IV
  
  
    // Create a message header containing the random IV and a set of (id, encrypted message key) pairs
    std::string ivstr = iv.as_string();
    for (unsigned int i=0; i<32; i++) data.push_back( ivstr[i] );
    // TODO - for now, just add in plaintext
    std::string keystr = key.as_string();
    for (unsigned int i=0; i<32; i++) data.push_back( keystr[i] );
    // Note the size of the header
    head_size = data.size();


    // Load the binary file and check the size
    data_file.open( data_filename, std::ios::binary );
    if(!data_file.is_open()) {
      std::cout << "Error opening data file:" << "\n";
      return 1;
    }
    data_file.seekg(0, std::ios::end);
    data_size = data_file.tellg(); // get the length of the file
    if (head_size + data_size > 21222) {
      std::cout << "Error - too much data (before processing)" << "\n";
      return 2;
    } else if (head_size + data_size < 223) {
      std::cout << "Error - too little data (before processing)" << "\n";
      std::cout << "Need at least one FEC block for ciphertext stealing" << "\n";
      return 2;
    }
  
    
    // Read the file into the data byte vector
    data_file.seekg(0, std::ios::beg);
    data.resize(head_size + data_size);
    data_file.read(&data[head_size], data_size);


    // Encrypt the data bytes
    Botan::Pipe pipe(get_cipher("AES-128/CFB", key, iv, Botan::ENCRYPTION)); // CFB mode, so no padding required
    pipe.start_msg();
    pipe.write((Botan::byte*) &data[data.size() - data_size], data.size() ); // write in plaintext
    pipe.end_msg();
    pipe.read((Botan::byte*) &data[data.size() - data_size], data.size()); // read back out ciphertext
  
  
    // Add Reed-Solomon FEC encoding with "ciphertext stealing" instead of padding
    std::string m, f; std::vector<char> data_fec;
    data_fec.reserve(90*90*3);
    // first encode all full blocks
    std::vector<char>::iterator i;
    for (i = data.begin(); i<data.end()-223; i+=223) {
      m = std::string( i, i+223 );
      ReedSolomonEncoder( m, f );
      for (unsigned int i=0;i<223;i++) data_fec.push_back(m[i]);
      for (unsigned int i=0;i<32;i++) data_fec.push_back( f[i]);;
    }
    // then, if we have any data left
    if (i != data.end()) {
      // pad the last block with data from the penultimate (encoded) block
      std::cout << "\n";
      while ( data.size() % 223 != 0) {
        data.push_back( data_fec.back() );
        std::cout << (int) data_fec.back() << ',';
        data_fec.pop_back();
      }
      std::cout << "\n";
      // encode the final block
      m = std::string( data.end()-223, data.end() );
      ReedSolomonEncoder( m, f );
      for (unsigned int i=0;i<223;i++) data_fec.push_back(m[i]);
      for (unsigned int i=0;i<32;i++) data_fec.push_back( f[i]);
    }
    data = data_fec;
    

    // Load the source image file into a CImg object
    try {src->load( src_filename );}
    catch (CImgInstanceException &e) {
      std::cout << "Error loading source image:" << e.what() << "\n";
      return 1;
    }
  

    // Store the data vector in the image
    src->format_for_implantation();
    try {src->implant_data( data );}
    catch (DataCarrierImgImplantationException &e) {
        std::cout << "Error implanting data: " << e.what() << "\n";
        return 1;
    }
  

    // Generate a filename and save our final image (in a lossless format)
    try {src->save( dst_filename );}
    catch (CImgInstanceException &e) {
      std::cout << "Error saving output image: " << e.what() << "\n";
      return 1;
    }
    
    // Return with succes
    return 0; 
}

unsigned int base::DecryptPhoto(
    const char*     & src_filename,
    const char*     & data_filename
) const
{

    std::ofstream     data_file;			// data file object	 
    DataCarrierImg * src =  new DataCarrierImg(); 	// source image object
    std::vector<char>  data; 				// for data bytes we wish to transfer
    
    
    // Load the source image file into a CImg object
    try {src->load( src_filename );}
    catch (CImgInstanceException &e) {
      std::cout <<  "Error loading source image:" << e.what() << "\n";
      return 1;
    }
    
    
    // Check that the dimensions are exactly 720x720
    if (src->width() != 720 || src->height() != 720) {
      std::cout << "Error - wrong image dimensions" << "\n";
      return 2;
    }
      
    
      // Decode from image using Haar DWT
      try {src->extract_data( data );}
      catch (DataCarrierImgExtractionException &e) {
          std::cout << "Error extracting data: " << e.what() << "\n";
          return 1;
      }
    
    // Correct errors
    std::string m, f, m2, m3; std::vector<char> data_corrected;
    std::vector<char>::iterator i;
    // decode all but the final full block
    for (i = data.begin(); i < data.end()-510; i+=255) {
      m = std::string( i, i+223 );
      f = std::string( i+223, i+255 );
      if ( ReedSolomonDecoder( m, f, m2 ) > 0) {
        std::cout << "FEC decoding failed, exiting" << "\n";
        return 3;
      }
      for (unsigned int i=0;i<223;i++) data_corrected.push_back(m2[i]);
    }
    // if we have any partial blocks
    if (i != data.end()-255) {
      // decode the final block-size bits of data
      m = std::string( data.end()-255, data.end()-32 );
      f = std::string( data.end()-32 , data.end() );
      if ( ReedSolomonDecoder( m, f, m3 ) > 0) {
        std::cout << "FEC decoding failed, exiting" << "\n";
        return 3;
      }
      // write back to data
      std::cout << "\n";
      for (int j = data.size()%255; j < 255; j++) {
        std::cout << (int) m3[j-32] << ',';
        data[ data.size()-(j+1) ] = m3[j-32];
      }
      std::cout << "\n";
    }
    // now decode the final full block
    for (i = data.begin() + ((data.size() - data.size()%255) - 255); i < data.end()-255; i+=255) {
      m = std::string( i, i+223 );
      f = std::string( i+223, i+255 );
      if ( ReedSolomonDecoder( m, f, m2 ) > 0) {
        std::cout << "FEC decoding failed, exiting" << "\n";
        return 3;
      }
      for (unsigned int i=0;i<223;i++) data_corrected.push_back(m2[i]);
    }
    // if any partial data, it now needs to be appended
    for (unsigned int i=0;i< data.size()%255 - 32 ;i++) data_corrected.push_back(m3[i]);
    data = data_corrected;
    
    
    // Retrieve the  message key and decrypt the data bytes
    // !!!TODO!!! - See if we can decrpyt the message
    // for now the key is always in plaintext at the front of the message
    Botan::LibraryInitializer init;
    std::string ivstr(32, 'x');
    for (unsigned int i=0; i<32; i++) ivstr[i] = data[i];
    Botan::InitializationVector iv( ivstr );
    std::string keystr(32, 'x');
    for (unsigned int i=0; i<32; i++) keystr[i] = data[32+i];
    Botan::SymmetricKey key( keystr );
    // We use AES-128 with CFB, so no padding required
    Botan::Pipe pipe(get_cipher("AES-128/CFB", key, iv, Botan::DECRYPTION));
    // Write ciphertext into cipher
    pipe.start_msg();
    pipe.write((Botan::byte*) &data[64], data.size()-64 );  
    pipe.end_msg();
    // Write out the plaintext back to the vector
    pipe.read((Botan::byte*) &data[64], data.size()-64 );
    
      
    // Save data to a file
    data_file.open( data_filename, std::ios::binary);
    if(!data_file.is_open()) {
      std::cout << "Error creating data file:" << "\n";
      return 1;
    }
    data_file.write(&data[64], data.size()-64 );
    
    // Return with success
    return 0; 
}

unsigned int base::CalculateBER(
      const char*                   & file1_path,
      const char*                   & file2_path
) const
{
  
  // ifstream objects
  std::ifstream file1, file2;
  
  // open file 1
  file1.open( file1_path, std::ios::binary );
  if(!file1.is_open()) {
    std::cout << "Error opening data file 1:" << "\n";
    return 1;
  }
  
  // and file 2
  file2.open( file2_path, std::ios::binary );
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
    std::bitset<8> bs( error );
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
{return 0;}
/**
{

  // Finite Field Parameters 
  const std::size_t field_descriptor                 =   8;
  const std::size_t generator_polynommial_index      =   120;
  const std::size_t generator_polynommial_root_count =   32;
  
  // Reed Solomon Code Parameters 
  const std::size_t code_length = 255;
  const std::size_t fec_length  =  32;
  const std::size_t data_length = code_length - fec_length;

   // Instantiate Finite Field and Generator Polynomials 
   schifra::galois::field field(field_descriptor,
                                schifra::galois::primitive_polynomial_size06,
                                schifra::galois::primitive_polynomial06);

   schifra::galois::field_polynomial generator_polynomial(field);

   schifra::sequential_root_generator_polynomial_creator(field,
                                                         generator_polynommial_index,
                                                         generator_polynommial_root_count,
                                                         generator_polynomial);

   // Instantiate Encoder and Decoder (Codec) 
   schifra::reed_solomon::encoder<code_length,fec_length> encoder(field,generator_polynomial);

   message = message + std::string(data_length - message.length(),static_cast<unsigned char>(0x00));

   // Instantiate RS Block For Codec 
   schifra::reed_solomon::block<code_length,fec_length> block;

   // Transform message into Reed-Solomon encoded codeword 
   if (!encoder.encode(message,block))
   {
      std::cout << "Error - Critical encoding failure!" << std::endl;
      return 1;
   }
   
   fec = std::string(fec_length,static_cast<unsigned char>(0x00));
   block.fec_to_string(fec);

   return 0;
}
  */
unsigned int base::ReedSolomonDecoder(
      std::string  	& message_plus_errors,
      std::string	& fec,
      std::string	& message
) const
{return 0;}
/**
{
  // Finite Field Parameters 
  const std::size_t field_descriptor                 =   8;
  const std::size_t generator_polynommial_index      =   120;
  const std::size_t generator_polynommial_root_count =   32;
  
  // Reed Solomon Code Parameters 
  const std::size_t code_length = 255;
  const std::size_t fec_length  =  32;
  const std::size_t data_length = code_length - fec_length;
  
  // Instantiate Finite Field and Generator Polynomials 
  schifra::galois::field field(field_descriptor,
			       schifra::galois::primitive_polynomial_size06,
			       schifra::galois::primitive_polynomial06);
  schifra::galois::field_polynomial generator_polynomial(field);
  schifra::sequential_root_generator_polynomial_creator(field,
							generator_polynommial_index,
							generator_polynommial_root_count,
							generator_polynomial);
  
  // Instantiate Decoder (Codec) 
  schifra::reed_solomon::decoder<code_length,fec_length> decoder(field,generator_polynommial_index);
  
  // Instantiate RS Block For Codec 
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
*/

base::~base()
{
  std::cout << "base::destructor\n" ;
}
