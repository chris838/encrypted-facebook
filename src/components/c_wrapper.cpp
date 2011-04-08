#include "c_wrapper.h"

// Required eFB Libary component includes
#include "efb/BasicLibary.h"
#include "efb/Upsampled165KiBFactory.h"

IeFBLibrary* create_IeFBLibrary(const char* id, const char* dir)
{
/* For now we use this concrete implementation. Potentially the exact library implementation which is instantiated could be decided at runtime, e.g. by passing parameters (specified in browser) to this function. */
  return (IeFBLibrary*) new efb::BasicLibary(
      *(new efb::Upsampled165KiBFactory()),
      id, dir
    );
}

/* Load a cryptographic identity from the filenames provided. */
const unsigned int loadIdentity(IeFBLibrary* This, const char* private_key_filename, const char* public_key_filename, const char* passphrase)
{
  return This->loadIdentity(private_key_filename, public_key_filename, passphrase);
}

/* Generate a new cryptographic identity and write out to the filenames provided. */
const unsigned int generateIdentity(
   IeFBLibrary* This, const char* private_key_filename, const char* public_key_filename, const char* passphrase)
{
  return This->generateIdentity(private_key_filename, public_key_filename, passphrase);
}

/* Load a set of Facebook ID / public key pairs from the provided directory, which will be used for encrypting messages/photos. */
const unsigned int loadIdKeyPair( IeFBLibrary* This, const char* id, const char* key_filename)
{
  return This->loadIdKeyPair(id,key_filename);
}

/* Take a null terminated UTF8 string. Remove the null terminal and treat as an array of arbitrary binary data. Encrypt it for the supplied set of intended recipients, prepending the appropriate message header. Encode into a Facebook-ready UTF8 format, treating each two byte pair as a unicode codepoint (with some modifications to avoid certain illegal characters, including null). Terminate with a null character and return. */
const char* encryptString(
  IeFBLibrary* This, const char* ids, const char* str_in)
{
  return This->encryptString( ids, str_in );
}

/* Take a null terminated UTF8 string which has been downloaded from Facebook. Remove the null terminal and decode from the Facebook-ready UTF8 format into arbitrary binary data. Attempt to parse a message header and decrypt the data. If succesful the output should be a valid UTF8 string (with no null characters). Terminate with a null character and return. */
const char* decryptString( IeFBLibrary* This, const char* str_in)
{
  return This->decryptString( str_in );
}

/* Given the path to a target file and a destination image, encrypt and store the file within the image. */
const unsigned int encryptFileInImage
(
  IeFBLibrary* This,
  const char* ids,
  const char* data_in_filename,
  const char* img_out_filename
)
{
  return This->encryptFileInImage( ids, data_in_filename, img_out_filename );
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
const unsigned int calculateBitErrorRate( IeFBLibrary* This, const char file1[], const char file2[] )
{
  return This->calculateBitErrorRate( file1,file2 );
}

void destroy_object( IeFBLibrary* This )
{
  delete This;
}