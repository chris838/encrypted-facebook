#ifndef C_WRAPPER_H
#define C_WRAPPER_H

#ifdef __cplusplus
   extern "C" {
#endif

  typedef struct base base ; /* opaque */

  base* create_base(const char* cache_dir, const char* temp_dir);

  const char* binToUTF8( const base* This, const char* str , unsigned const int len);
  const char* UTF8ToBin( const base* This, const char* str , unsigned const int len);
  const unsigned int EncryptPhoto( const base* This, const char* str );

  void destroy_object( base* This ) ;

#ifdef __cplusplus
   }
#endif

#endif /* C_WRAPPER_H */