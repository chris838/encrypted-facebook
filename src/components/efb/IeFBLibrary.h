#ifndef IEFBLIBRARY_H
#define IEFBLIBRARY_H

//! Library interface definition.
/**
    This definition lies outside the namespace so it can be seen by the C wrapper (since C doesnt't support namespaces). There may exist a better way. 
*/
class IeFBLibrary
{
    public :
        //! Load a cryptographic identity from the filenames provided.
        virtual unsigned int loadIdentity
        (
            const char* private_key_filename,
            const char* public_key_filename,
            const char* passphrase
        ) = 0;
        
        //! Generate a new cryptographic identity and write out to the filenames provided.
        virtual unsigned int generateIdentity
        (
            const char* private_key_filename,
            const char* public_key_filename,
            const char* passphrase
        ) const = 0;
        
        //! Load a set of Facebook ID / public key pairs which will be used for encrypting messages/photos.
        virtual unsigned int loadIdKeyPair
        (
            const char* id,
            const char* key_filename
        ) = 0;
        
        //! Close the library and wipe any sensitive directories/information.
        virtual void close() = 0;
        
        //! Encrypt a file into an image for the supplied array of recipients.
        virtual unsigned int encryptFileInImage
        (
            const char* ids,
            const char* data_filename,
            const char* img_out_filename
        ) = 0;
        //! Attempt to extract and decrypt a file from an image.
        virtual unsigned int decryptFileFromImage
        (
            const char*  img_in_filename,
            const char*  data_filename
        ) = 0;
        
        //! Take a message string and encrypt into a Facebook-ready string. Both will be null terminated.
        virtual const char* encryptString
        (
            const char* ids,
            const char*  input
        ) const = 0;
        //! Take string from Facebook and decrypt to a message string. Both will be null terminated.
        virtual const char* decryptString(
            const char*  input
        ) const = 0;
        
        //! Debug function for calculating BER
        virtual unsigned int calculateBitErrorRate
        (
            const char*  file1_path,
            const char*  file2_path
        ) const = 0;
};

#endif //IEFBLIBRARY_H