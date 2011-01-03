#ifndef C_WRAPPER_H
#define C_WRAPPER_H

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct IeFBLibrary IeFBLibrary; // opaque

IeFBLibrary* create_IeFBLibrary();

const char* encryptString(IeFBLibrary* This, const char ids[], const unsigned int len,const char* str);

const char* decryptString(IeFBLibrary* This, const char* str);

const unsigned int encryptFileInImage(IeFBLibrary* This, const char ids[], const unsigned int len, const char* data_in_filename, const char* img_out_filename);

const unsigned int decryptFileFromImage(IeFBLibrary* This, const char* img_in_filename, const char* data_out_filename);

const unsigned int calculateBitErrorRate( const IeFBLibrary* This, const char* file1, const char* file2 );

void destroy_object( IeFBLibrary* This ) ;

#ifdef __cplusplus
   }
#endif

#endif /* C_WRAPPER_H */