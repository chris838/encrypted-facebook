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
  const char*        & pathtofile
) const
{
/*
  CImg<unsigned char> img;
  //CImg<unsigned char> album;
  //CImg<unsigned char> icon;
  
  try {
    // Try to load the specified image
    img.load( pathtofile );
  }
  catch (CImgInstanceException &e) {
    // If we get an error, return with failure status
    std::fprintf(stderr,"CImg Library Error : %s",e.what());
    return 1;
  }
  
  // Open jpeg file

  // Encrypt !!!!TODO!!!!
  
  // Add forward error correction encoding
  
  // Obtain the time as a string, for use as a filename
  time_t rawtime; time( &rawtime );
  std::stringstream ss;
  ss << rawtime;*/
  
  // Wipe over the DST coefficients of the template file with the
  // encrypted data bytes + FEC, then write out to a valid jpeg
  //WriteToDSTCoeffs(); // ( buffer, temp_dir + ss.str() + ".jpg" )
  ReadFromDSTCoeffs();
  
  // Return with a path to the file

  return 0; 
}


unsigned int base::WriteToDSTCoeffs(
) const
{
  // TEMPORARY!!!!!!!!!!!!!!!!!!! We won't load from disk
  const char* templatefilename  = "/home/chris/Desktop/template.jpg";
  const char* inputfilename     = "/home/chris/Desktop/in.jpg";
  const char* outfilename       = "/home/chris/Desktop/out.jpg";
  
  // Vars to hold jpeg data/metadata
  static jvirt_barray_ptr * coef_arrays; 
  struct jpeg_decompress_struct srcinfo;
  struct jpeg_compress_struct dstinfo;
  struct jpeg_error_mgr jsrcerr, jdsterr;
  JBLOCKARRAY src_buffer;
  
  // Open the files
  FILE * template_file;
  std::ifstream input_file;
  FILE * output_file;
  //
  template_file = fopen(templatefilename,"r");
  if (template_file == NULL) {
   perror("Can't open template file");
   return (1);
  }
  //
  input_file.open( inputfilename );
  if(!input_file.is_open()) {
    perror("Can't open input file");
    return 1;
  }
  //
  output_file = fopen(outfilename,"w");
  if (output_file == NULL) {
   perror("Can't open output file");
   return (1);
  }	
  
  // Prepare for modifying the template jpeg
  srcinfo.err = jpeg_std_error(&jsrcerr);
  dstinfo.err = jpeg_std_error(&jdsterr);
  jpeg_create_decompress(&srcinfo);
  jpeg_create_compress(&dstinfo);
  jpeg_stdio_src(&srcinfo, template_file);
  jpeg_read_header(&srcinfo, TRUE);

  // Load the jpeg DCT coefs and jpeg parameters
  coef_arrays = jpeg_read_coefficients(&srcinfo);
  jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

  // Load the DCT quantisation table values
  UINT16 * intensity_quants = srcinfo.quant_tbl_ptrs[0]->quantval;
  UINT16 * chrome_quants    = srcinfo.quant_tbl_ptrs[1]->quantval;
  
  // Load the input file into a byte array
  input_file.seekg(0, std::ios::end);
  size_t fileSize = input_file.tellg(); // get the length of the file
  input_file.seekg(0, std::ios::beg);
  // create a vector to hold all the bytes in the file
  std::vector<char> input_bytes(fileSize, 0);
  // read the file
  input_file.read(&input_bytes[0], fileSize);
  
  // Loop throught the DCT coef array writing data
  // We need a slightly bigger loop for the intensity values,
  // since they *aren't* subsampled. So for each row...
  for (int row=0; row<90; row++) {
    // ...load single row of blocks into src_buffer
    src_buffer = (*srcinfo.mem->access_virt_barray)
      ((j_common_ptr) &srcinfo, coef_arrays[0], row, 1, TRUE);
    // for each block in a row
    for (int blk=0; blk < 90; blk++) {
        // for each DCT coeff
        for (int dct=0; dct < 64; dct++) {
          
            src_buffer[0][blk][dct] = intensity_quants[dct];
            //std::cout << dct << ": " << src_buffer[0][blk][dct] << "\n";
        }
    }
  }
  // Now for the chrominance values, which are subsampled and therefore
  // have only 45x45 rows/coloums. So for each chrome coef array:
  for (int chrome = 1; chrome < 3; chrome++) {
    // For each row...
    for (int row=0; row<45; row++) {
        // ...load single row of blocks into src_buffer
        src_buffer = (*srcinfo.mem->access_virt_barray)
          ((j_common_ptr) &srcinfo, coef_arrays[chrome], row, 1, TRUE);
        // for each block in a row
        for (int blk=0; blk < 45; blk++) {
            // for each DCT coeff
            for (int dct=0; dct < 64; dct++) {
                //src_buffer[0][blk][dct] = 0;
                //std::cout << src_buffer[0][blk][dct] << "\n";
            }
        }
    }
  }

  // Write out changes  
  dstinfo.optimize_coding = TRUE;
  jpeg_stdio_dest(&dstinfo, output_file);
  jpeg_write_coefficients(&dstinfo, coef_arrays);
  
  // Free up memory and close file handlers
  jpeg_finish_compress(&dstinfo);
  jpeg_destroy_compress(&dstinfo);
  jpeg_finish_decompress(&srcinfo);
  jpeg_destroy_decompress(&srcinfo);
  fclose(template_file);
  input_file.close();
  fclose(output_file);

  return 0;
}

int int_div( int a, int b ) {
  // calculate a over b with symmetric rounding
  return a / b;
}

unsigned int base::ReadFromDSTCoeffs(
) const
{
  // TEMPORARY!!!!!!!!!!!!!!!!!!! We won't load from disk
  const char* inputfilename     = "/home/chris/Desktop/out.jpg";
  const char* outfilename       = "/home/chris/Desktop/extracted.jpg";
  
  // Vars to hold jpeg data/metadata
  static jvirt_barray_ptr * coef_arrays; 
  struct jpeg_decompress_struct srcinfo;
  struct jpeg_error_mgr jsrcerr;
  JBLOCKARRAY src_buffer;
  
  FILE * input_file;
  std::ofstream output_file;
  //
  input_file = fopen(inputfilename,"r");
  if (input_file == NULL) {
   perror("Can't open template file");
   return (1);
  }
  //
  output_file.open( outfilename );
  if(!output_file.is_open()) {
    perror("Can't open output file");
    return 1;
  }
  
  // Prepare for extracting from the input jpeg
  srcinfo.err = jpeg_std_error(&jsrcerr);
  jpeg_create_decompress(&srcinfo);
  jpeg_stdio_src(&srcinfo, input_file);
  jpeg_read_header(&srcinfo, TRUE);

  // Load the jpeg DCT coefs and jpeg parameters
  coef_arrays = jpeg_read_coefficients(&srcinfo);

  // Load the DCT quantisation table values
  UINT16 * intensity_quants = srcinfo.quant_tbl_ptrs[0]->quantval;
  UINT16 * chrome_quants    = srcinfo.quant_tbl_ptrs[1]->quantval;
  
  // Create a vector to hold all the bytes in the file
  std::vector<char> output_bytes(8100, 0);
  
  // Loop throught the DCT coef array reading data
  // We need a slightly bigger loop for the intensity values,
  // since they *aren't* subsampled. So for each row...
  for (int row=0; row<90; row++) {
    // ...load single row of blocks into src_buffer
    src_buffer = (*srcinfo.mem->access_virt_barray)
      ((j_common_ptr) &srcinfo, coef_arrays[0], row, 1, TRUE);
    // for each block in a row
    for (int blk=0; blk < 90; blk++) {
        // for each DCT coeff
        for (int dct=0; dct < 64; dct++) {
          
            //b4 = src_buffer[0][blk][17];
            std::cout << src_buffer[0][blk][dct] << ",";
        }
        std::cout << "\n";
    }
  }
  // Now for the chrominance values, which are subsampled and therefore
  // have only 45x45 rows/coloums. So for each chrome coef array:
  for (int chrome = 1; chrome < 3; chrome++) {
    // For each row...
    for (int row=0; row<45; row++) {
        // ...load single row of blocks into src_buffer
        src_buffer = (*srcinfo.mem->access_virt_barray)
          ((j_common_ptr) &srcinfo, coef_arrays[chrome], row, 1, TRUE);
        // for each block in a row
        for (int blk=0; blk < 45; blk++) {
            // for each DCT coeff
            for (int dct=0; dct < 64; dct++) {
                //src_buffer[0][blk][dct] = 0;
                //std::cout << src_buffer[0][blk][dct] << "\n";
            }
        }
    }
  }
  
  // Write the extracted bytes to our output file
  // allocate memory for file content
  char * buffer = new char [output_bytes.size()];
  // read content of vector
  for (unsigned int i=0; i<output_bytes.size(); i++) buffer[i] = (char) output_bytes[i];
  // write to outfile
  output_file.write(buffer,output_bytes.size());
  
  // release dynamically-allocated memory
  delete[] buffer;
  
  // Free up memory and close file handlers
  jpeg_finish_decompress(&srcinfo);
  jpeg_destroy_decompress(&srcinfo);
  fclose( input_file);
  output_file.close();

  return 0;
}

base::~base()
{
  std::cout << "base::destructor\n" ;
}