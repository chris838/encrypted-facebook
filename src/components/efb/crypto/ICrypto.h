#ifndef EFB_ICRYPTO_H
#define EFB_ICRYPTO_H

// Standard libary includes
#include <vector>

// eFB Library sub-component includes
#include "../Common.h"

namespace efb {
    
    // Encryption exceptions
    struct EncryptionException : public ImplantException {
        EncryptionException(const std::string &err) : ImplantException(err) {} };
    struct DecryptionException : public ExtractException {
        DecryptionException(const std::string &err) : ExtractException(err) {} };
        
    //! Abstract class definining the interface for the cryptographic algorithms.
    class ICrypto
    {
        public :
            //! Returns the predicted header size so we can leave room before encryption.
            virtual unsigned int calculateHeaderSize( unsigned int numOfIds ) const = 0;
            //! Retrieves header of any stored data size so we can skip this after decryption.
            virtual unsigned int retrieveHeaderSize(std::vector<byte>& data) const = 0;
            //! Writes header and encrypts message. Assumes there is a header-size offset before data bytes begin.
            virtual void encryptMessage
            (
                std::vector<FacebookId>& ids,
                std::vector<byte>& data // with header-size offset before data bytes begin
            ) = 0;
            //! Parses any data header and attempts to decrypt the data, leaving the header in place.
            virtual void decryptMessage( std::vector<byte>& data ) = 0;
            //! Generate and write a new private/public key pair to disk.
            virtual void generateKeys
            (
                std::ofstream& private_key_file,
                std::ofstream& public_key_file,    
                std::string& passphrase
            ) = 0;
            //! Load a private/public key pair into memory.
            virtual void loadKeys
            (
                std::string& private_key_filename,
                std::string& public_key_filename,   
                std::string& passphrase
            ) = 0;
            //! Load a potential recipient's public key into memory.
            virtual void loadIdKeyPair
            (
                FacebookId& id,
                std::string& key_filename
            ) = 0;
            //! Set the Facebook ID for decryption
            virtual void setUserId(const FacebookId& id) = 0;
    };
}

#endif //EFB_ICRYPTO_H