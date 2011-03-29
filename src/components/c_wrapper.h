#ifndef C_WRAPPER_H
#define C_WRAPPER_H

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct IeFBLibrary IeFBLibrary; /* opaque */

IeFBLibrary* create_IeFBLibrary(const char* id, const char* dir);

const unsigned int loadIdentity(IeFBLibrary* This, const char* private_key_filename, const char* public_key_filename, const char* passphrase);

const unsigned int generateIdentity(IeFBLibrary* This, const char* private_key_filename, const char* public_key_filename, const char* passphrase);

const unsigned int loadIdKeyPair( IeFBLibrary* This, const char* id, const char* key_filename);

const char* encryptString(IeFBLibrary* This, const char* ids, const char* str_in);

const char* decryptString(IeFBLibrary* This, const char* str_in);

const unsigned int encryptFileInImage(IeFBLibrary* This, const char* ids, const char* data_in_filename, const char* img_out_filename);

const unsigned int decryptFileFromImage(IeFBLibrary* This, const char* img_in_filename, const char* data_out_filename);

const unsigned int calculateBitErrorRate( IeFBLibrary* This, const char* file1, const char* file2 );

void destroy_object( IeFBLibrary* This ) ;

#ifdef __cplusplus
   }
#endif

#endif /* C_WRAPPER_H */