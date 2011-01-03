#include "c_wrapper.h"
#include "efb.h"

IeFBLibrary* create_IeFBLibrary()
{
  /* For now we use this concrete implementation. Potentially the exact library implementation which is instantiated could be decided at runtime, e.g. by passing parameters (specified in browser) to this function. */
  return (IeFBLibrary*) new Core();
}

/* Take a null terminated UTF8 string. Remove the null terminal and treat as an array of arbitrary binary data. Encrypt it for the supplied set of intended recipients, prepending the appropriate message header. Encode into a Facebook-ready UTF8 format, treating each two byte pair as a unicode codepoint (with some modifications to avoid certain illegal characters, including null). Terminate with a null character and return. */
const char* encryptString(
  IeFBLibrary* This, const char ids[], const unsigned int len, const char* str)
{
  return "something";
}

/* Take a null terminated UTF8 string which has been downloaded from Facebook. Remove the null terminal and decode from the Facebook-ready UTF8 format into arbitrary binary data. Attempt to parse a message header and decrypt the data. If succesful the output should be a valid UTF8 string (with no null characters). Terminate with a null character and return. */
const char* decryptString( IeFBLibrary* This, const char* str)
{
  return "something";
}

/* Given the path to a target file and a destination image, encrypt and store the file within the image. */
const unsigned int encryptFileInImage
(
  IeFBLibrary* This,
  const char ids[],
  const unsigned int len,
  const char* data_in_filename,
  const char* img_out_filename
)
{
  return This->encryptFileInImage( ids, len, data_in_filename, img_out_filename );
}

/* Given the path to a source image and a destination file, attempt to extract and decrypt data from the image and write out to the file. */
const unsigned int decryptFileFromImage
(
    IeFBLibrary* This,
    const char* img_in_filename,
    const char* data_out_filename
)
{
  return This->decryptFileFromImage( img_in_filename, data_out_filename );
}

/* Helper function calculates bit error rate. */
const unsigned int calculateBitErrorRate( const IeFBLibrary* This, const char file1[], const char file2[] )
{
  return This->calculateBitErrorRate( file1,file2 );
}

void destroy_object( IeFBLibrary* This )
{
  delete This;
}