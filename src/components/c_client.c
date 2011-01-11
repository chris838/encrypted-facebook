#include "c_wrapper.h"

IeFBLibrary* lib;

unsigned int initialise() {  
  lib = create_IeFBLibrary();
  return 1;
}

/* Load a cryptographic identity from disk. File paths are null terminated */
const unsigned int c_loadIdentity(const char* private_key_filename, const char* public_key_filename, const char* passphrase)
{
  return loadIdentity(lib, private_key_filename, public_key_filename, passphrase);
}

/* Generate a cryptographic identity and write to disk. File paths are null terminated */
const unsigned int c_generateIdentity(const char* private_key_filename, const char* public_key_filename, const char* passphrase)
{
  return generateIdentity(lib, private_key_filename, public_key_filename, passphrase);
}

/* Load a set of ID public key pairs for use when encrypting, fron the provided directory. */
const unsigned int c_loadIdKeyPair(const char* idkeypairs_dir)
{
  return loadIdKeyPair(lib, idkeypairs_dir );
}

/* Encrypt a null terminated string into another null terminated Facebook-ready string, for the supplied set of intended recipients*/
const char* c_encryptString(const char* ids, const char* str)
{
  return encryptString( lib, ids, str);
}

/* Attempt to decrypt a Facebook null terminated string into another null terminated string. */
const char* c_decryptString(const char* str)
{
  return decryptString( lib, str);
}

/* Takes the full path to a file and encyrpts it into a destination image file, using the given set of intented recipients. */
const unsigned int c_encryptFileInImage(
const char* ids, const char* data_in_filename, const char* img_out_filename
)
{
  return encryptFileInImage( lib,ids,data_in_filename,img_out_filename );
}

/* Takes the full path to an image and attempts to extract and decyrpt any stored data into a destination file. */
const unsigned int c_decryptFileFromImage(const char* img_in_filename, const char* data_out_filename)
{
  return decryptFileFromImage( lib,img_in_filename,data_out_filename);
}

/* Debug function to calculate the bit error rate of two files. */
const unsigned int c_calculateBitErrorRate(const char* file1, const char* file2)
{
  return calculateBitErrorRate( lib,file1,file2 );
}

int close() {
  destroy_object(lib) ;
  return 1;
}