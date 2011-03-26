#ifndef EFB_BOTANRSACRYPTO_H
#define EFB_BOTANRSACRYPTO_H

/**
################################################################################
    This file contains the class definition for a Botan library based crytography module.
################################################################################
*/

// Standard library includes
#include <fstream>

// Botan crypto library includes
#include <botan/botan.h>
#include <botan/rsa.h>
#include <botan/pubkey.h>
#include <botan/look_pk.h>

// eFB Library sub-component includes
#include "ICrypto.h"

namespace efb {
        //! Botan cryptography class using N-byte AES and M-byte RSA.
    /**
        This class uses the Botan cryptography library to perform encryption and decryption in place. AES and RSA are the symmetric and asymmetric (respectively) schemes employed. The template variables <N,M> determine the key lengths. The header consists of two length bytes specifying the number of recipients, the message IV in plaintext, and a sequence of (Facebook ID, message-key) pairs. Each message-key is encrypted under the public key of the Facebook ID it is paired with.
    */
    template <int N, int M>
    class BotanRSACrypto : public ICrypto
    {
        // Generate or set a random IV and random message key
        void generateNewIv()
            {iv_ = Botan::InitializationVector(rng_, 16);} // a random 16-byte iv
        void generateNewMessageKey()
            {key_ = Botan::SymmetricKey(rng_, N);} // a random N-byte key
        //! Write out an encypted message key using the public key of the ID provided.
        void getCipheredMessageKey
        (
            FacebookId & id,
            byte data[]
        )
        {
            Botan::RSA_PublicKey pubkey = idkeymap_[ id ];
            Botan::PK_Encryptor* encryptor = Botan::get_pk_encryptor(pubkey, "EME1(SHA-512)");
            Botan::SecureVector<byte> mkey_encrypted = encryptor->encrypt(key_.begin(),key_.length(),rng_);
            for (unsigned int i=0; i<mkey_encrypted.size();i++)
                data[i] = mkey_encrypted[i];
        }
        //! Write the length tag at the start of the data
        void writeNumIds( byte data[], unsigned short len ) const
        {
            data[0] = (unsigned char) (len >> 8) ;
            data[1] = (unsigned char) len;
        }
        //! Read the length tag at the start of the data
        unsigned short readNumIds( std::vector<byte> data ) const
        {
            unsigned short len;
            unsigned char len_hi, len_lo;
            len_hi = data[0];
            len_lo = data[1];
            len = (((unsigned short) len_hi) << 8) | len_lo;
            return len;
        }
        
        //! Create the crypto header using a new IV and message key.
        void createCryptoHeader
        (
            std::vector<FacebookId> & ids,
            std::vector<byte> & data
        )
        {
            // Randomise key and initialisation vector.
            generateNewIv();
            generateNewMessageKey();
            
            // Set length of output key (same as public key for RSA)
            unsigned int key_len = M;    
            
            // Offset into the header
            unsigned int offset = 0;

            // Write tag with the number of IDs to the start of the header.
            writeNumIds( &data[offset], (unsigned short) ids.size() );
            offset+=2;
            
            // Write IV to the header, in plaintext
            for (unsigned int i=0; i<iv_.length(); i++)
                data[offset+i] = iv_.begin()[i];
            offset+=iv_.length();

            // For each ID, lookup the public key and encrypt the message key
            for (unsigned int i=0; i<ids.size();i++) {

                // Insert the ID
                FacebookId id = ids[i];
                for (unsigned int j=0; j<8; j++)
                    data[offset+j] = (unsigned char) (id.val >> (j*8));
                offset+=8;                
                // Insert the encrypted message key
                getCipheredMessageKey(id, &data[offset]);
                offset+= key_len;
            }
            
        }
        
        //! Attempty to parse a crypto header - this will retrieve and set the message key and IV.
        void parseCryptoHeader
        (
            std::vector<byte> & data
        )
        {
            // Offset into the header
            unsigned int offset = 0;
            
            // Set length of output key (same as public key for RSA)
            unsigned int key_len = M;  
            
            // Retrieve the number of recipients
            unsigned int len = retrieveHeaderSize(data);
            offset+=2;
            
            // Retrieve the IV
            iv_ =  Botan::InitializationVector( &data[offset], iv_.length() );
            offset+=iv_.length();
            
            // Loop through till we find user's ID (if it exists)
            for (unsigned int i=0; i<len; i++)
            {
                // Extract an ID
                unsigned long long int id_int = 0;
                for (unsigned int j=0; j<8; j++)
                    id_int = id_int | (((unsigned long long int)data[offset+j]) << (j*8));
                offset+=8;
                // If we have our ID, extract messsage key
                if (id_int == id_.val) {
                    // We can decrypt, so do so
                    std::string key_str((char*)&data[offset], key_len );
                    key_ = Botan::SymmetricKey( decryptor_->decrypt( &data[offset], key_len ) );
                    return;
                }
                offset+=key_len;
            }
            
            throw DecryptionException("ID not found - cannot decrypt this message.");
        }
        
        //! Botan library attribute members
        Botan::LibraryInitializer init_;
        Botan::AutoSeeded_RNG rng_;
        Botan::InitializationVector iv_;
        // user keys
        Botan::SymmetricKey key_;
        Botan::RSA_PrivateKey private_key_;
        Botan::RSA_PublicKey public_key_;
        // recipient public key dictionary
        std::map<FacebookId,Botan::RSA_PublicKey> idkeymap_;
        // user's Facebook ID
        FacebookId id_;
        // PK encryptor object
        Botan::PK_Decryptor* decryptor_;
        
        public :
        
            BotanRSACrypto()
            {
                // so we can work out their sizes properly...
                generateNewIv();
                generateNewMessageKey();
            }
            
            unsigned int calculateHeaderSize( unsigned int numOfIds ) const
                // Length tag + IV length + number of IDs x (ID length + key length)
                {return sizeof(short) + iv_.length() + numOfIds*(sizeof(long long int) + M);}
            
            unsigned int retrieveHeaderSize(std::vector<byte>& data) const
            {
                unsigned int numOfIds = readNumIds(data);
                return calculateHeaderSize(numOfIds);
            }
                
            void encryptMessage
            (
                std::vector<FacebookId>& ids,
                std::vector<byte>& data // with header-size offset before data bytes begin
            )
            {
                // note - this will change (randomise) the IV and message key
                createCryptoHeader( ids, data );
                
                // perform the encryption, skipping the first <header size> bytes
                unsigned int hs = calculateHeaderSize( ids.size() ), ds = data.size(), ms = ds - hs;
                std::stringstream ss; ss << "AES-" << N*8 << "/CFB";
                Botan::Pipe encrypter(
                    get_cipher(ss.str(), key_, iv_, Botan::ENCRYPTION));
                encrypter.start_msg();
                encrypter.write((Botan::byte*) &data[hs], ms ); 
                encrypter.end_msg();
                encrypter.read((Botan::byte*) &data[hs], ms );
            }
            
            void decryptMessage( std::vector<byte>& data )
            {
                // note - this will try make a valid header from the start of the data and use it to set the IV and message key. If the image is not valid or we are not on the intended recipients list this may well throw an exception.
                parseCryptoHeader(data);
            
                // perform the decryption, skipping the first <header size> bytes
                unsigned int hs = retrieveHeaderSize(data), ds = data.size(), ms = ds - hs;
                std::stringstream ss; ss << "AES-" << N*8 << "/CFB";
                Botan::Pipe decrypter(
                    get_cipher(ss.str(), key_, iv_, Botan::DECRYPTION));
                decrypter.start_msg();
                decrypter.write((Botan::byte*) &data[hs], ms );  
                decrypter.end_msg();
                decrypter.read((Botan::byte*) &data[hs], ms );
            }
            
            //! Generate a private/public key pair and save to disk.
            void generateKeys
            (
                std::ofstream& private_key_file,
                std::ofstream& public_key_file,
                std::string& passphrase
            )
            {
                // Create keys.
                Botan::RSA_PrivateKey rsa_key(rng_, 8*M);
                
                // PEM encode as strings.
                std::string rsa_private_pem = Botan::PKCS8::PEM_encode(rsa_key, rng_, passphrase);
                std::string rsa_public_pem = Botan::X509::PEM_encode(rsa_key);
                
                // Write to disk
                private_key_file << rsa_private_pem;
                public_key_file << rsa_public_pem;
            }
            
            //! Load a private/public key pair from disk.
            void loadKeys
            (
                std::string& private_key_filename,
                std::string& public_key_filename,
                std::string& passphrase
            )
            {
                // Load into pointers
                Botan::PKCS8_PrivateKey* private_key_ptr(
                    Botan::PKCS8::load_key(private_key_filename, rng_, passphrase) );
                Botan::X509_PublicKey* public_key_ptr(
                    Botan::X509::load_key(public_key_filename) );
                
                // Downcasting - but we know they are RSA keys so its ok
                private_key_ = *dynamic_cast<Botan::RSA_PrivateKey*>(private_key_ptr);
                public_key_ = *dynamic_cast<Botan::RSA_PublicKey*>(public_key_ptr);
                
                // Add public key to key map so it can be used for encryption
                idkeymap_[id_] = public_key_;

                // Create the decryption object using our private key
                decryptor_ = Botan::get_pk_decryptor(private_key_, "EME1(SHA-512)");                
            }
            
            //! Load potential recipients' public keys into memory.
            void loadIdKeyPair
            (
                FacebookId& id,
                std::string& key_filename
            )
            {
                // Read the public key from the file.
                Botan::X509_PublicKey* key_ptr(
                    Botan::X509::load_key(key_filename) );
                Botan::RSA_PublicKey key = *dynamic_cast<Botan::RSA_PublicKey*>(key_ptr);
                
                // Save the pair in the id/key map.
                idkeymap_[id] = key;
            }
            
            //! Set the Facebook ID for decryption
            void setUserId(const FacebookId& id) {
                id_.val = id.val;
            }
    };
}

#endif //EFB_BOTANRSACRYPTO_H