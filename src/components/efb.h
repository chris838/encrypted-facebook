// Include required standard C++ headers.
#include <stdio.h>
#include <string>
#include <bitset>
#include <vector>
#include <deque>
#include <map>
#include <sstream>
#include <stdexcept>

// Include Shiffra Reed Solomon library
#include "schifra/schifra_galois_field.hpp"
#include "schifra/schifra_galois_field_polynomial.hpp"
#include "schifra/schifra_sequential_root_generator_polynomial_creator.hpp"
#include "schifra/schifra_reed_solomon_encoder.hpp"
#include "schifra/schifra_reed_solomon_decoder.hpp"
#include "schifra/schifra_reed_solomon_block.hpp"
#include "schifra/schifra_error_processes.hpp"

// Include CImg library with JPEG support
#define JPEG_INTERNALS
#define cimg_verbosity 0     // Disable modal window in CImg exceptions.
#define cimg_use_jpeg 1
#include "CImg.h"

// Include the Botan crypto library
#include <botan/botan.h>
#include <botan/rsa.h>
#include <botan/pubkey.h>
#include <botan/look_pk.h>

// Useful mask bytes for UTF8 enc/decoding
#define         MASKBITS                0x3F
#define         MASKBYTE                0x80
#define         MASK2BYTES              0xC0
#define         MASK3BYTES              0xE0
#define         MASK4BYTES              0xF0
#define         MASK5BYTES              0xF8
#define         MASK6BYTES              0xFC

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
    

//! Namespace containing all the Encrypted Facebook C++ code.
/**
    This namespace contains several abstract base classes along with their concrete subclasses. In places we use abstract classes in place of interfaces - hence the 'I' prefix. Some abstract classes do not particularly conform to the definition of interface, however for continuity we still use the 'I' prefix to denote that these classes are abstract and must be extended.

    \par Main library class and Firefox interface
    
    The main libray (abstract) class \ref IBasicLibrary implements IeFBLibrary - the external interface exposed to Firefox via the C wrapper code. When a concrete subclass is instantiated it requires a Facebook user ID with which a profile directory is created (if not present). Clean up of any cached images is performed both on construction and destruction.
    
    On instantiation the class also creates (as members) instances of the cryptograhic and error correction classes which group related functions using its 'factory' member. This class also contains methods for UTF8 string decoding and encoding.
    
    The \ref BasicLibary conrete subclass is instantiated with some \ref ILibFactory subclass. This abstract factory pattern is used to group complimentary libray sub-components, since there exists some interdependancy between the exact implementation of each of these sub-components. For example, the error correction implementation must match the bit error rate inccured when the image carrier implementation undergoes compression. 
    
    \par Cryptograhy and error correction algorithms
    
    The \ref ICrypto class defines an interface to the cryptography algorithms used by the rest of the library. These include algorithms for both symmetric and asymmetric encryption/decryption, and also for public/private key pair generation. The \ref IFec class similarly defines an interface to the forward error correction algorithms.
    
    For both library classes, subclass creation and destruction is managed by the main library class which will extend \ref IeFBLibrary. A concrete \ref ILibFactory subclass will select concrete subclasses of \ref IFec and \ref ICrypto.
    
    \par Conduit image class
    
    The \ref IConduitImage abstract class extends the CImg library class (CImg) by adding functionality to implant and extract data to the image in a reasonably JPEG compression immune fashion. Like the \ref IFec and \ref ICrypto library classes the concrete implemention is specified by the concrete implementation of \ref ILibFactory.    
*/
namespace efb {

    typedef unsigned short  Unicode2Bytes;
    typedef unsigned char   byte;
    
    // Exceptions for dealing with images - grouped into implant and extract.
    struct ImplantException : public std::runtime_error {
        ImplantException(const std::string &err) : std::runtime_error(err) {} };
    struct ExtractException : public std::runtime_error {
        ExtractException(const std::string &err) : std::runtime_error(err) {} };
    struct ConduitImageImplantException : public ImplantException {
        ConduitImageImplantException(const std::string &err) : ImplantException(err) {} };
    struct ConduitImageExtractException : public ExtractException {
        ConduitImageExtractException(const std::string &err) : ExtractException(err) {} };
    struct FecEncodeException : public ImplantException {
        FecEncodeException(const std::string &err) : ImplantException(err) {} };
    struct FecDecodeException : public ExtractException {
        FecDecodeException(const std::string &err) : ExtractException(err) {} };
    struct EncryptionException : public ImplantException {
        EncryptionException(const std::string &err) : ImplantException(err) {} };
    struct DecryptionException : public ExtractException {
        DecryptionException(const std::string &err) : ExtractException(err) {} };
        
    // Key exception, thrown when dealing with identities and cryptographic keys.
    struct IdException : public std::runtime_error {
        IdException(const std::string &err) : std::runtime_error(err) {} };
        
    //! Structure for representing Facebook IDs, which we assume to be passed on construction as 15-16 byte ASCII character sequences. They can be represented by an 8-byte integer type. 
    struct FacebookId
    {
        unsigned long long int val;
        
        FacebookId( const char* str )
        {
            // Convert string to a numeric values so that key representation takes mimimal space.
            std::stringstream ss;
            ss << str;
            if ( (ss >> val).fail() ) throw IdException("Error converting Facebook ID to numeric value.");
        }
        
        FacebookId( std::string str )
        {
            // Convert string to a numeric values so that key representation takes mimimal space.
            std::stringstream ss;
            ss << str;
            if ( (ss >> val).fail() ) throw IdException("Error converting Facebook ID to numeric value.");
        }
        
        FacebookId() : val(0) {}        
    };
    
    //! Overloaded operator so type can be used in a keymap
    bool operator< (const FacebookId& fid1, const FacebookId& fid2)
        {return fid1.val < fid2.val;}

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
    
    //! Abstract class definining the interface for the error correction algorithms.
    class IFec
    {
        public :
            //! Encode data by appending error correction codes.
            virtual void encode( std::vector<byte>& data) const =0;
            //! Decode (correct) data in place.
            virtual void decode( std::vector<byte>& data) const =0;
    };

    //! Abstract class defining a conduit image, with functionality for implanting and extracting data.
    class IConduitImage : public cimg_library::CImg<byte>
    {
        public :
            //! Get the maximum ammount of data that can be stored in this implementation.
            virtual unsigned int getMaxData() = 0;
            //! Check how much data (if any) is stored in the current image.
            virtual unsigned int readSize() = 0;
            //! Implant data.
            virtual void implantData( std::vector<byte>& data ) = 0;
            //! Extract data.
            virtual void extractData( std::vector<byte>& data ) = 0;
    };
    
    
    //! Abstract class defining a codec to encode and decode strings to and from Facebook.
    class IStringCodec
    {
        public :
            virtual std::string binaryToFbReady( std::vector<byte>& data) const = 0;
            virtual std::vector<byte> fbReadyToBinary( std::string& str ) const = 0; 
    };
    
    //! Abstract factory class.
    /**
        An abstract factory class used to generate library sub-components. It defines a group of compatible or complementary sub-component implementations.
    */
    class ILibFactory
    {
        public :
            //! Conduit image object creater.
            virtual IConduitImage& create_IConduitImage() const = 0;
            //! Cryptography library object creater.
            virtual ICrypto& create_ICrypto() const = 0;
            //! Forward error correction library object creater.
            virtual IFec& create_IFec() const = 0;
            //! String codec object creater.
            virtual IStringCodec& create_IStringCodec() const = 0;
    };
    
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
    //! Schifra Reed Solomon error correction library where code rate is (N,M)
    template <int N, int M>
    class SchifraFec : public IFec
    {
        // Finite Field Parameters
        const std::size_t field_descriptor_;
        const std::size_t generator_polynommial_index_;
        const std::size_t generator_polynommial_root_count_;
        // Reed Solomon Code Parameters
        const std::size_t code_width_;
        const std::size_t fec_width_;
        const std::size_t data_width_;
        // Finite Field and Generator Polynomials
        schifra::galois::field field_;
        schifra::galois::field_polynomial generator_polynomial_;
        schifra::sequential_root_generator_polynomial_creator polynomial_creator_;
        // Encoder and Decoder (Codec)
        schifra::reed_solomon::encoder<N,N-M> encoder_;
        schifra::reed_solomon::decoder<N,N-M> decoder_;
    
        //! Generate FEC code from a message   
        void encodeBlock(
            std::string & message,
            std::string	& fec
        ) const
        {
            // Instantiate RS Block For Codec
            schifra::reed_solomon::block<N,N-M> block;
            // Transform message into Reed-Solomon encoded codeword
            if (!encoder_.encode(message,block))
                throw FecEncodeException("Error - Critical encoding failure!");
            block.fec_to_string(fec);
        }
        
        //! Try and fix any errors in a block-size message using FEC code
        void decodeBlock(
            std::string & message,
            std::string	& fec
        ) const
        {
            // Instantiate RS Block For Codec
            schifra::reed_solomon::block<N,N-M> block(message, fec);

            // Try and fix any errors in the message
            if (!decoder_.decode(block))
                throw FecDecodeException("Error - Critical decoding failure!");
            block.data_to_string(message);
        }
        
        public :
            //! Default constructor.
            SchifraFec(
                std::size_t field_descriptor,
                std::size_t generator_polynommial_index,
                std::size_t generator_polynommial_root_count,
                const unsigned int primitive_polynomial_size,
                const unsigned int primitive_polynomial[9]
            ) :
                // Initialisation for const attributes
                field_descriptor_(field_descriptor),
                generator_polynommial_index_(generator_polynommial_index),
                generator_polynommial_root_count_(generator_polynommial_root_count),
                code_width_(N),
                fec_width_(N-M),
                data_width_(M),
                // Instantiate Finite Field and Generator Polynomials
                field_
                (
                    field_descriptor_,
                    primitive_polynomial_size,
                    primitive_polynomial
                ),
                generator_polynomial_(field_),
                polynomial_creator_ // don't need this var, but need to call constructor (probably)
                (
                    field_,
                    generator_polynommial_index_,
                    generator_polynommial_root_count_,
                    generator_polynomial_
                ),
                // Instantiate Encoder and Decoder (Codec)
                encoder_(field_,generator_polynomial_),
                decoder_(field_,generator_polynommial_index_)
            {}
            
            //! Encode data by appending error correction codes.
            void encode( std::vector<byte>& data) const
            {
                // Cycle through each block, appending FEC code at the back of the array. Note that any final partial block will be padded automatically by this process, provided enough blocks exist.
                unsigned int num_blocks = (data.size()/data_width_) + (data.size()%data_width_==0?0:1);
                if ( num_blocks*fec_width_ < data_width_ )
                    throw FecEncodeException(
                    "Not enough data blocks to pad (possible) partial last block.");
                
                for (unsigned int i=0; i<num_blocks*data_width_;i+=data_width_) {
                    std::string message((char*) &data[i], data_width_);
                    std::string fec(fec_width_, static_cast<byte>(0x00));
                    encodeBlock(message, fec);   
                    for (unsigned int j=0;j<fec_width_;j++) data.push_back( fec[j] );
                }            
            }
            
            
            //! Decode (i.e. correct) data in place.
            void decode( std::vector<byte>& data) const
            {
                // We decode backwards since the last block may contain encoded FEC codes for the initial blocks.
                unsigned int num_blocks = (data.size()/code_width_) + (data.size()%code_width_==0?0:1);
                
                for (int i=(num_blocks-1)*data_width_; i>=0; i-=data_width_) {
                    std::string message((char*) &data[i], data_width_);
                    std::string fec((char*) &data[data.size()-fec_width_], fec_width_ );
                    try{decodeBlock(message,fec);}
                    catch (FecDecodeException &e) {
                        std::cout << "block didn't decode " << i/data_width_ << std::endl;
                    }
                    //remove used fec
                    for (unsigned int j=0;j<fec_width_;j++) data.pop_back();
                    //write back corrected data
                    for (unsigned int j=0;j<data_width_;j++) data[i+j] = message[j];
                }   
            }
            
    };
    
    //! Reed Solomon error correction using the Schifra library with 255-byte block size providing a (255,223) code rate.
    class ReedSolomon255Fec : public SchifraFec<255,223>
    {
        public :
            //! Default constructor.
            ReedSolomon255Fec() : SchifraFec<255,223>
            (
                8,      // field_descriptor
                120,    // generator_polynommial_index
                32,      // generator_polynommial_root_count
                schifra::galois::primitive_polynomial_size06,
                schifra::galois::primitive_polynomial06
            ) {}
    };
    //! Reed Solomon error correction using the Schifra library with 15-byte block size providing a (15,8) code rate.
    class ReedSolomon15Fec : public SchifraFec<15,8>
    {
        public :
            //! Default constructor.
            ReedSolomon15Fec() : SchifraFec<15,8>
            (
                4,      // field_descriptor
                0,    // generator_polynommial_index
                7,      // generator_polynommial_root_count
                schifra::galois::primitive_polynomial_size01,
                schifra::galois::primitive_polynomial01
            ) {}
    };
    
    
    //! Conduit image (abstract) class which buffers data bytes and reades/writes several at a time to a block in the image.
    class BufferedConduitImage : public IConduitImage
   {           
        //! Buffer to store data while reading and writing.
        std::deque<byte> rbuffer_, wbuffer_;
        
        //! Counters indicating the read and write heads.
        unsigned int rhead_, whead_;
                
        //! Format the image in preparation for implantation.
        /**
            This operation will resize the image to 720x720x1 and truncate the colour channels, as only data storage in single-channel (greyscale) image is supported. JPEG compression requires a (lossy) colour space transform from RGB to YCrCb which complicates using colour images for data storage. Even worse - Facebook's JPEG compression process uses chrominance subsampling. However, this does mean that discarding the additional two chrominance channels only results in a %50 reduction in maximum potential data storage capacity.        
         */
        void formatForImplantation()
        {
            // Format the image to 720x720, greyscale, single slice (resample using Lanczos)
            resize(720,720,1,-1,6);
            channel(0);
        }
        
        //! Write a single byte to the image.
        /**
            Bytes are queued into a buffer which is flushed automatically whenever its size reaches that of the block size. The write head indicates the number of bytes already stored in the image - it *DOES NOT* include bytes stored in the buffer.
        */
        void write( byte b )
        {
            wbuffer_.push_back( b );
            // If we have enough bytes, flush the buffer.
            if ( wbuffer_.size() >= block_size_ ) flush(); 
        }
        
        //! Flush the buffer. The buffer contents (if not empty) will be written out to the image. Random bytes are used as padding if the buffer is partially full.
        void flush()
        {
            if ( wbuffer_.size() > 0 )
            {
                // If partially full, pad with randoms.
                while (wbuffer_.size() < block_size_) wbuffer_.push_back( std::rand() );
                // Calculate the pixel coords based on the write head position
                unsigned int i,j;
                getBlockCoords(i,j, whead_);
                // Write out the buffer contents
                encodeInBlock( wbuffer_, i, j );
                // Increment write head
                whead_+=block_size_;
                // Clear the buffer
                wbuffer_.clear();
            }
        }
        
        //! Get the block coordinates based on the index of the byte we are writing.
        virtual void getBlockCoords( unsigned int &i, unsigned int &j, unsigned int idx) = 0;
        
        //! Encode block_size_ bytes in a block of pixels. (i,j) indicates the pixel at the start of the block.
        virtual void encodeInBlock( std::deque<byte> data, unsigned int i, unsigned int j ) = 0;
        
        //! Read a single byte from the image.
        /**
            Data must be read three bytes at a time. They are therefore queued into a read buffer. Calls to read return elements from the read buffer until it is empty, at which point it refills.
        **/
        byte read()
        {
            // If buffer is empty, fill it.
            if ( rbuffer_.size() == 0)
            {
                // Get the coordinates for the next three data bytes based on the read head position.
                unsigned int i,j;
                getBlockCoords(i,j, rhead_);
                // Read in the next three bytes
                decodeFromBlock( rbuffer_, i, j);
                // Advance the read head
                rhead_+=block_size_;
            }
            // Return and remove the first item in the buffer
            byte r = rbuffer_.front();
            rbuffer_.pop_front();
            return r;
        }
                
        //! Decode block_size_ bytes from a block of pixels. (i,j) indicates the pixel at the start of the block.
        virtual void decodeFromBlock( std::deque<byte> & data, unsigned int i, unsigned int j ) = 0;
        
        //! Write out the size of the data stored to the image.
        virtual void writeSize(unsigned int len) = 0;
        
        protected :
            
            //! Variable to determine how many bytes are stored per block
            const unsigned int block_size_;
            
            //! Helper function to decode triple modular redundancy coded bytes
            byte tripleModR( byte a, byte b, byte c) const
            {
                return ((a&b)|(a&c)|(b&c));
            }
        
        public :
            
            //! Constructor.
            BufferedConduitImage(unsigned int block_size) :
                rhead_(0),
                whead_(0),
                block_size_(block_size)
            {
                // Seed the random number generator
                std::srand ( time(NULL) );
            }
            
            //! Implant data.
            virtual void implantData( std::vector<byte>& data )
            {                
                // Format the image for implantation
                formatForImplantation();
                                
                // Check the data isn't too large, note down size
                if (data.size() > getMaxData())
                    throw ConduitImageImplantException("Too much data");
                
                // Seed the random number generator for good measure
                std::srand ( time(NULL) );
                
                // Loop through the vector and write out the data bytes to the image
                unsigned int idx = 0;
                while( idx < data.size() )
                {
                    // Write a byte to the image
                    write( data[idx] );
                    idx++;
                }/*
                // Now pad out with random bytes till we reach capacity
                while( idx < getMaxData() )
                {
                    // Write a (random) byte to the image
                    write( (byte) std::rand() );
                    idx++;
                }*/
                // Flush the buffer to ensure all data has actually been written out.
                flush();
                
                // Write the size of the data stored.
                writeSize( data.size() );
            }
            
            //! Extract data.
            virtual void extractData( std::vector<byte>& data )
            {
                // Read the length tag from the image, check it is not too large .
                unsigned int len = readSize();
                if ( len > getMaxData() )
                    throw ConduitImageImplantException("Length tag too large.");
                    
                // Reserve enough space to store the extracted data
                data.reserve( len );
                                
                // Keep reading bytes until we have the desired amount
                while( data.size() < len )
                {
                    // Read a single byte into data
                    data.push_back( read() );
                }
            }
    };
    
    
    //! Conduit image class which uses the Haar wavelet tranform to store data in low frequency image components.
    class HaarConduitImage : public BufferedConduitImage
    {
        //! Get the block coordinates based on the index of the byte we are writing.
        void getBlockCoords( unsigned int &i, unsigned int &j, unsigned int idx)
        {
            i = ((idx/block_size_) / 90)*8 ;
            j = ((idx/block_size_) % 90)*8 ;
        }
        
        //! Encode block_size_ bytes in a block of pixels. (i,j) indicates the pixel at the start of the block.
        void encodeInBlock( std::deque<byte> data, unsigned int i, unsigned int j )
        {
            // Temp workspace variable
            short int temp[8][8];
            // Apply the Haar transform to the block (two iterations)
            haar2dDwt (i, j, temp);
            
            // Write 3 bytes of data to the temp block
            byte a, b, c;
            a = data[0]; b=data[1]; c=data[2];
            temp[0][0] 	= (a & 0xfc) | 0x02;
            temp[1][0] 	= (b & 0xfc) | 0x02;
            temp[0][1] 	= (c & 0xfc) | 0x02;
            temp[1][1]	= ((a & 0x03) <<6) | ((b & 0x03) <<4) | ((c & 0x03) <<2) | 0x02;
            
            // Apply the inverse transform and write back to the image
            haar2dDwti(temp, i, j);
        }
        

        //! Perform the Haar Discrete Wavelet transform on an 8x8 temp block.
        /**
            Perform the Haar Discrete Wavelet Transform (with lifting scheme so the inverse can be performed losslessly). The result is written to the supplied 8x8 array, since we cannot do this in place (we would need (at least) an extra sign bit for 3/4 of the 64 coefficients).
        */
        void haar2dDwt
        (
            unsigned int x0,
            unsigned int y0,
            short int temp[8][8]
        ) const
        {
            short int temp2[8][8];
            // First iteration works on the entire 8x8 block
            // For each row...
            for (unsigned int j=0; j<8; j++) {
                // Perform 1D Haar transfrom with integer lifting
                for (unsigned int i=0; i<4; i++) {
                    // average
                    temp2[i][j] = divFloor( operator()(x0+(2*i), y0+j) +
                                            operator()(x0+((2*i)+1), y0+j),
                                            2);
                    // difference
                    temp2[4+i][j] = operator()(x0+(2*i), y0+j) -
                                    operator()(x0+((2*i)+1), y0+j);
                }
            }
            // For each column...
            for (unsigned int i=0; i<8; i++) {
                // Perform 1D Haar transfrom with integer lifting
                for (unsigned int j=0; j<4; j++) {
                    // average
                    temp[i][j] = divFloor( temp2[i][2*j] + temp2[i][(2*j)+1], 2);
                    // difference
                    temp[i][4+j] = temp2[i][2*j] - temp2[i][(2*j)+1];
                }
            }
            // Then the next iteration on the top left 4x4 corner block
            // For each row...
            for (unsigned int j=0; j<4; j++) {
                // Perform 1D Haar transfrom with integer lifting
                for (unsigned int i=0; i<2; i++) {
                    // average
                    temp2[i][j] = divFloor(temp[2*i][j] + temp[2*i+1][j], 2);
                    // difference
                    temp2[2+i][j] = temp[2*i][j] - temp[2*i+1][j];
                }
            }
            // For each column...
            for (unsigned int i=0; i<4; i++) {
                // Perform 1D Haar transfrom with integer lifting
                for (unsigned int j=0; j<2; j++) {
                    // average
                    temp[i][j] = divFloor(temp2[i][2*j] + temp2[i][2*j+1], 2);
                    // difference
                    temp[i][2+j] = temp2[i][2*j] - temp2[i][2*j+1];
                }
            }
        }
        
        //! Helper function for lifting scheme
        int divFloor(int a, int b) const {
          if (a>=0) return a/b;
          else return (a-1)/b; 
        }
        
        //! Truncate Haar coefficients (preserving stored data).
        void truncateCoefficients( short int& p1, short int& p2, short int& m)
        {
            // If p1 or p2 lie outside the range 0-255 we must rectify this, however we *MUST* also preserve their mean value (m) as this contains data.
            /*
            if (p1<0) {p1 = 0; p2 = 2*m;}
            if (p1>255) {p1 = 255; p2 = 2*m-255;}
            if (p2<0) {p2 = 0; p1 = 2*m;}
            if (p2>255) {p2 = 255; p1 = 2*m-255;}
            */
            if ((p1<0) || (p1>255) || (p2<0) || (p2>255)) {p1=p2=m;}
            
        }
        
        //! Perform the inverse Haar Discrete Wavelet transform on an 8x8 block.
        /**
            Perform the inverse Haar Discrete Wavelet Transform using a lifting scheme. We start at pixel (x0,y0) in the image and work on the subsequent 8x8 block. We read in from the supplied temp block containing the wavelet coefficients.
        */
        void haar2dDwti(
            short int temp[8][8],
            unsigned int x0,
            unsigned int y0
        )
        {
            short int temp2[8][8], p1, p2;
            // First iteration just on the 4x4 top left corner block
            // For each column...
            for (unsigned int i=0; i<4; i++) {
                // Perform 1D inverse Haar transfrom with integer lifting
                for (unsigned int j=0; j<2; j++) {
                    p1 	= temp[i][j] + divFloor(temp[i][2+j]+1,2) ;
                    p2 	= p1 - temp[i][2+j];
                    // Check we don't overflow the pixel
                    if (i<2) truncateCoefficients(p1,p2,temp[i][j]);
                    temp2[i][2*j] = p1;
                    temp2[i][2*j+1] = p2;
                }
            }
            // For each row (do the same again)...
            for (unsigned int j=0; j<4; j++) {
                // Perform 1D inverse Haar transfrom with integer lifting
                for (unsigned int i=0; i<2; i++) {
                    // Check we don't overflow the pixel
                    p1 	= temp2[i][j] + divFloor(temp2[2+i][j]+1,2) ;
                    p2 	= p1 - temp2[2+i][j];
                    truncateCoefficients(p1,p2,temp2[i][j]);
                    temp[2*i][j] =  p1;
                    temp[2*i+1][j] = p2;                    
                }
            }
            // Then the next iteration on the entire 8x8 block
            // For each column...
            for (unsigned int i=0; i<8; i++) {
                // Perform 1D inverse Haar transfrom with integer lifting
                for (unsigned int j=0; j<4; j++) {
                    // Check we don't overflow the pixel
                    p1 	= temp[i][j] + divFloor(temp[i][4+j]+1,2) ;
                    p2 	= p1 - temp[i][4+j];
                    if (i<4) truncateCoefficients(p1,p2,temp[i][j]);
                    temp2[i][2*j] = p1;
                    temp2[i][2*j+1] = p2; 
                }
            }
            // For each row (do the same again)...
            for (unsigned int j=0; j<8; j++) {
                // Perform 1D inverse Haar transfrom with integer lifting
                for (unsigned int i=0; i<4; i++) {
                    // Check we don't overflow the pixel
                    p1 	= temp2[i][j] + divFloor(temp2[4+i][j]+1,2) ;
                    p2 	= p1 - temp2[4+i][j];
                    truncateCoefficients(p1,p2,temp2[i][j]);
                    operator()(x0+2*i, y0+j) = temp[2*i][j] = p1;
                    operator()(x0+2*i+1, y0+j) = temp[2*i+1][j] = p2;                    
                }
            }
        }
        
        //! Decode block_size_ bytes from a block of pixels. (i,j) indicates the first pixel in the block.
        void decodeFromBlock( std::deque<byte> & data, unsigned int i, unsigned int j )
        {
            // Temp workspace variable
            short int temp[8][8];
            // Apply the transform (two iterations) to the block
            haar2dDwt (i, j, temp);
            
            // Extract the four approximation coefficients
            byte p1,p2,p3,p4;
            p1 = temp[0][0];
            p2 = temp[1][0];
            p3 = temp[0][1];
            p4 = temp[1][1];
            // Retrive the three data bytes
            byte a,b,c;
            a = (p1 & 0xfc) | ((p4 & 0xc0) >> 6);
            b = (p2 & 0xfc) | ((p4 & 0x30) >> 4);
            c = (p3 & 0xfc) | ((p4 & 0x0c) >> 2);
            // Append them to the read buffer
            data.push_back( a );
            data.push_back( b );
            data.push_back( c );
        }
        
        //! Writes the size of the image (2 bytes) to the last blocks using triple modular redundancy.
        void writeSize
        (
            unsigned int len    
        )
        {
            std::deque<byte> data1, data2;
            byte hi = (byte) (len >> 8);
            byte lo = (byte) (len & 0x00ff);
            // Create three copies of the hi and lo bytes
            for (unsigned int i=0;i<3;i++) data1.push_back( hi );
            for (unsigned int i=0;i<3;i++) data2.push_back( lo );
            // Write out to the last blocks
            encodeInBlock( data1, 712, 704 );
            encodeInBlock( data2, 712, 712 );
        }
        
        public :
            
            //! Constructor.
            HaarConduitImage() :
                BufferedConduitImage(3)
            {}
            
            //! Get the maximum ammount of data (in bytes) that can be stored in this implementation.
            virtual unsigned int getMaxData()
            {
                return (90*90*3) - 6;
            }
            
            //! Check how much data (if any) is stored in the current image.
            virtual unsigned int readSize()
            {
                std::deque<byte> data1,data2;
                decodeFromBlock(data1, 712, 704 );
                decodeFromBlock(data2, 712, 712 );
                unsigned int len =
                            (tripleModR(data1[0],data1[1],data2[2]) << 8 )
                        |   (tripleModR(data2[0],data2[1],data2[2]) << 0 );
                return len;
            }
    };
    
    // Buffered conduit image abstract subclass which stores order_ bits for each 8-bit pixel value, scaled up and offset to span the 0-255 range.
    class UpsampledConduitImage : public BufferedConduitImage
    {
        //! Attribute which determines how many bits are stored in each pixel.
        const unsigned int order_;
                
        //! Convert binary to 8-bit gray codes.
        byte binaryToGray( byte num )
        {
            return (num>>1) ^ num;
        }
        
        //! Convert 8-bit gray codes to binary.
        byte grayToBinary( byte num )
        {
            unsigned short temp = num ^ (num>>8);
            temp ^= temp>>4;
            temp ^= temp>>2;
            temp ^= temp>>1;
            return temp;
        }
        
        protected :
            
            //! Encode bits into a single pixel, using Gray codes.
            void encodeInPixel( byte data, unsigned int i, unsigned int j )
            {
                // Choose a scale factor and an offset that minimise errors.
                unsigned int factor = (255 / ((0x01 << order_)-1)) + 1;            
                unsigned int offset =  ((factor * ((0x01 << order_)-1)) - 255) / 2;
                int x = (binaryToGray(data) * factor) - offset;
                // Output in range 0-255 inclusive
                operator()(i,j) =  (x>255? 255 : (x<0? 0 : x));
            }
            
            // Decode order_ bits from a single pixel.
            byte decodeFromPixel( unsigned int i, unsigned int j )
            {
                // Choose a scale factor and an offset that minimise errors.
                unsigned int factor = (255 / ((0x01 << order_)-1)) + 1;            
                unsigned int offset =  ((factor * ((0x01 << order_)-1)) - 255) / 2;
                int x = operator()(i,j) + offset;
                int y = factor;
                // Round to nearest value
                int r = (( x%y <<1) >= y) ? (x/y) + 1 : (x/y);
                // Cap to max value
                byte max = (0x01 << order_)-1;
                r = (r > max) ? max : r;
                // Convert to binary from gray code
                return grayToBinary( r );
            }
        
        public :
            
            //! Constructor.
            UpsampledConduitImage(unsigned int block_size, unsigned int order) :
                BufferedConduitImage(block_size),
                order_(order)
            {}
    };
    
    
    //! Upsampling conduit image class which uses 3 bits per 8-bit pixel.
    /**
        This class stores 3 bits per pixel with an approximate error rate of 0.015%. TODO - check this.
    */
    class Upsampled3ConduitImage : public UpsampledConduitImage
    {
        //! Get the block coordinates based on the index of the byte we are writing.
        void getBlockCoords( unsigned int &i, unsigned int &j, unsigned int idx)
        {
            j = ((idx/block_size_)*8);
            i = j / 720;
            j = j % 720;
        }
        
        //! Encode block_size_ bytes in a block of pixels. (i,j) indicates the pixel at the start of the block.
        void encodeInBlock( std::deque<byte> data, unsigned int i, unsigned int j )
        {
            // Split 3 bytes into eight 3-bit segments
            byte r = 0x00;
            for (int k=0; k<8; k++)
            {
                // take the kth bit of each byte
                r =     (((data[0] & (0x1<<k)) >> k) << 0)
                    |   (((data[1] & (0x1<<k)) >> k) << 1)
                    |   (((data[2] & (0x1<<k)) >> k) << 2) ;
                // Encode these three bits to a pixel
                encodeInPixel(r, i, j+k);
            }
        }
        
        //! Decode block_size_ bytes from a block of pixels. (i,j) indicates the first pixel in the block.
        void decodeFromBlock( std::deque<byte> & data, unsigned int i, unsigned int j )
        {
            byte x, a, b, c;
            a=b=c=0x00;
            for (int k=0; k<8; k++)
            {
                // Get the relevant 3 bits
                x = decodeFromPixel(i,j+k);
                // OR them into the bytes
                a |= (((x & (0x1<<0)) >> 0) << k);
                b |= (((x & (0x1<<1)) >> 1) << k);
                c |= (((x & (0x1<<2)) >> 2) << k);
            }
            // Append the bytes to the data
            data.push_back( a );
            data.push_back( b );
            data.push_back( c );
        }
        
        //! Writes the size of the image (3 bytes) to the last twelve pixels, using triple modular redundancy.
        void writeSize
        (
            unsigned int len    
        )
        {
            std::deque<byte> data;
            data.push_back( (byte) (len >> 16) );
            data.push_back( (byte) (len >> 8) );
            data.push_back( (byte) (len & 0x00ff) );
            //
            encodeInBlock( data, 719, 696 );
            encodeInBlock( data, 719, 704 );
            encodeInBlock( data, 719, 712 );
        }
        
        public :
            
            //! Constructor.
            Upsampled3ConduitImage() :
                UpsampledConduitImage(3,3) // block size is 3 bytes, order is 3 bits per pixel
            {}
            
            //! Get the maximum ammount of data (in bytes) that can be stored in this implementation.
            virtual unsigned int getMaxData()
            {
                return (720*720*3)/8 - 9;
            }
            
            //! Check how much data (if any) is stored in the current image.
            virtual unsigned int readSize()
            {
                std::deque<byte> data;
                decodeFromBlock(data, 719, 696 );
                decodeFromBlock(data, 719, 704 );
                decodeFromBlock(data, 719, 712 );
                unsigned int len =
                            (tripleModR(data[0],data[3],data[6]) << 16)
                        |   (tripleModR(data[1],data[4],data[7]) << 8 )
                        |   (tripleModR(data[2],data[5],data[8]) << 0 );
                return len;
            }
    };

    //! Upsampling conduit image class which uses 4 bits per 8-bit pixel.
    /**
        This class stores 4 bits per pixel with an approximate error rate of 4.5%.
    */
    class Upsampled4ConduitImage : public UpsampledConduitImage
    {
        //! Get the block coordinates based on the index of the byte we are writing.
        void getBlockCoords( unsigned int &i, unsigned int &j, unsigned int idx)
        {
            j = ((idx/block_size_)*2);
            i = j / 720;
            j = j % 720;
        }
        
        //! Encode block_size_ bytes in a block of pixels. (i,j) indicates the pixel at the start of the block.
        void encodeInBlock( std::deque<byte> data, unsigned int i, unsigned int j )
        {
            // Split the byte into two segments
            byte b=data[0], hi = 0x00, lo = 0x00;
            hi = b >> 4;
            lo = b & 0x0f;
            encodeInPixel(hi, i, j);
            encodeInPixel(lo, i, j+1);
        }
        
        //! Decode block_size_ bytes from a block of pixels. (i,j) indicates the first pixel in the block.
        void decodeFromBlock( std::deque<byte> & data, unsigned int i, unsigned int j )
        {
            byte hi=0x00, lo=0x00;
            hi = decodeFromPixel(i,j);
            lo = decodeFromPixel(i,j+1);
            data.push_back( (hi << 4) | lo );
        }
        
        //! Writes the size of the image (3 bytes) to the last twelve pixels, using triple modular redundancy.
        void writeSize
        (
            unsigned int len    
        )
        {
            std::deque<byte> data1,data2,data3;
            data1.push_back( (byte) (len >> 16) );
            data2.push_back( (byte) (len >> 8) );
            data3.push_back( (byte) (len & 0x00ff) );
            //
            encodeInBlock( data1, 719, 702 );
            encodeInBlock( data2, 719, 704 );
            encodeInBlock( data3, 719, 706 );
            encodeInBlock( data1, 719, 708 );
            encodeInBlock( data2, 719, 710 );
            encodeInBlock( data3, 719, 712 );
            encodeInBlock( data1, 719, 714 );
            encodeInBlock( data2, 719, 716 );
            encodeInBlock( data3, 719, 718 );
        }
        
        public :
            
            //! Constructor.
            Upsampled4ConduitImage() :
                UpsampledConduitImage(1,4) // block size is 1 byte, order is 4 bits per pixel
            {}
            
            //! Get the maximum ammount of data (in bytes) that can be stored in this implementation.
            virtual unsigned int getMaxData()
            {
                return (720*720*4)/8 - 9;
            }
            
            //! Check how much data (if any) is stored in the current image.
            virtual unsigned int readSize()
            {
                std::deque<byte> data;
                decodeFromBlock( data, 719, 702 );
                decodeFromBlock( data, 719, 704 );
                decodeFromBlock( data, 719, 706 );
                decodeFromBlock( data, 719, 708 );
                decodeFromBlock( data, 719, 710 );
                decodeFromBlock( data, 719, 712 );
                decodeFromBlock( data, 719, 714 );
                decodeFromBlock( data, 719, 716 );
                decodeFromBlock( data, 719, 718 );
                unsigned int len =
                            (tripleModR(data[0],data[3],data[6]) << 16)
                        |   (tripleModR(data[1],data[4],data[7]) << 8 )
                        |   (tripleModR(data[2],data[5],data[8]) << 0 );
                return len;
            }
    };
        
    //! UTF8 codec which shifts characters by 0xB0 (176) to avoid problem characters.
    class ShiftB0StringCodec : public IStringCodec
    {
        public :
            std::string binaryToFbReady( std::vector<byte>& data) const
            {
                
                std::string output;
                // If not an even number of bytes, we pad with 0x08 byte AND prepend special padding character which is unicode: 0x10F000
                if (data.size()%2 != 0) {
                    data.push_back((byte)0x08);
                    output.push_back((byte)(MASK4BYTES | (0x10F000 >> 18)));
                    output.push_back((byte)(MASKBYTE | (0x10F000 >> 12 & MASKBITS)));
                    output.push_back((byte)(MASKBYTE | (0x10F000 >> 6 & MASKBITS)));
                    output.push_back((byte)(MASKBYTE | (0x10F000 & MASKBITS)));
                }
                std::vector<Unicode2Bytes> input( data.begin(), data.end() );
                for(unsigned int i=0; i < input.size(); i++)
                   {
                      // First we add an offset of 0x00b0
                      // so no dealing with control or null characters
                      unsigned int in = ((unsigned int) input[i]) + 0x00b0;
                      
                      // 0xxxxxxx
                      if(in < 0x80) output.push_back((byte)in);
                      
                      // 110xxxxx 10xxxxxx
                      else if(in < 0x800)
                      {
                         output.push_back((byte)(MASK2BYTES | (in >> 6)));
                         output.push_back((byte)(MASKBYTE | (in & MASKBITS)));
                      }
                      else if ( (in >= 0xd800) & (in <= 0xdfff) )
                      {  
                        // this character range is not valid (they are surrogate pair codes) so we shift 1 bit to the left and use those code points instead
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
                return output;
            }
            
            std::vector<byte> fbReadyToBinary( std::string& input ) const
            {
            bool padded = false;
            std::vector<Unicode2Bytes> output;
            for(unsigned int i=0; i < input.size();)
            {
               unsigned int ch;
         
               // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
               if((input[i] & MASK4BYTES) == MASK4BYTES)
               {
                 if (input[i] > 0xf2 && input[i] < 0x100) // this is a surrogate character
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
               
               // check for padding character
               if (ch == 0x10F000) {
                    padded = true;
               }
               else output.push_back( (Unicode2Bytes) (ch - 0xb0) ); // subtract 0xb0 offset 
            }
            
            std::vector<byte> data(output.begin(), output.end());
            // If we found a padding character, remove last byte
            if (padded) data.pop_back();
            return data;
        }
    };
    
    //! Basic library implementation.
    /**
        This library implementation takes a Facebook ID on instantiation which defines the directory within which file I/O takes place.
    */
    class BasicLibary : public IeFBLibrary
   {
        const ILibFactory& factory_;
        ICrypto& crypto_; // not const, has state (key and iv)
        const IFec& fec_;
        const IStringCodec& string_codec_;
        const FacebookId id_;
        const std::string working_directory_;
        
        public :
            //! Instantiate the library with Facebook ID and extension directory.
            /**
                \param factory The library factory which will generate library sub-components.
                \param id The Facebook ID of the current user.
                \param working_directory The full path to libraries working directory. This will almost certainly contain the Facebook ID. All subsequent pathnames used in correspondence with the library will be in relation to this directory.
            */
            BasicLibary
            (
                const ILibFactory& factory,
                const char* id,
                const char* working_directory
            ) :
                // Initialisation list
                factory_( factory ),
                crypto_( factory_.create_ICrypto() ),
                fec_( factory_.create_IFec() ),
                string_codec_( factory_.create_IStringCodec() ),
                id_( id ),
                working_directory_( working_directory )
            {
                std::cout << "Library initialised." << std::endl;
                std::cout << "Facebook ID is " << id_.val << "." << std::endl;
                std::cout << "Working directory is " << working_directory_ << "." << std::endl;
                // set the ID within the crypto library
                crypto_.setUserId( id_ );
            }
            
            //! Generate a new cryptographic identity and write out to the filenames provided.
            unsigned int generateIdentity
            (
                const char* private_key_filename,
                const char* public_key_filename,
                const char* passphrase
            ) const
            {
                // File handles
                std::ofstream private_key_file, public_key_file;
                   
                // Load the (new) private key file
                private_key_file.open(
                    (working_directory_+private_key_filename).c_str(), std::ios::binary);
                if(!private_key_file.is_open()) {
                    std::cout << "Error opening private key file: ";
                    std::cout << working_directory_ << private_key_filename << std::endl;
                    return 1;
                }
                
                // Load the (new) public key file
                public_key_file.open(
                    (working_directory_+public_key_filename).c_str(), std::ios::binary );
                if(!public_key_file.is_open()) {
                    std::cout << "Error opening public key file: ";
                    std::cout << working_directory_ << public_key_filename << std::endl;
                    return 1;
                }
                
                std::string passphrase_str(passphrase);
                crypto_.generateKeys(
                    private_key_file, public_key_file, passphrase_str );
                
                return 0;
            }
            
            
            //! Load a cryptographic identity from the filenames provided.
            unsigned int loadIdentity
            (
                const char* private_key_filename,
                const char* public_key_filename,
                const char* passphrase
            )
            {
                std::string private_key_filename_full =
                    working_directory_ + *(new std::string(private_key_filename));
                std::string public_key_filename_full =
                    working_directory_ + *(new std::string(public_key_filename));
                
                crypto_.loadKeys(
                    private_key_filename_full,
                    public_key_filename_full,
                    *(new std::string(passphrase))
                );
                
                return 0;
            }
            
            //! Load a potential recipient's ID and private key which may be used for encryption.
            unsigned int loadIdKeyPair
            (
                const char* id,
                const char* key_filename
            )
            {
                // Get ID object
                FacebookId id_obj( id );
                
                // Get key file string
                std::string key_filename_str(key_filename);
                std::string key_filename_full = working_directory_ + key_filename_str;

                crypto_.loadIdKeyPair(id_obj,key_filename_full);
                return 0;
            }
    
            //! Close the library and wipe any volatile directories.
            void close() {}
            
            unsigned int encryptFileInImage
            (
                const char* ids,
                const char*  data_filename,
                const char*  img_out_filename
            )
            {
                // !!!TODO!!! - For now we use a specific template image located on the desktop
                const char* template_filename =  "/home/chris/Desktop/src.bmp";
                //	 
                std::ifstream 	data_file; // input data file
                std::vector<byte> data; // byte array for our data bytes we wish to transfer
                IConduitImage& img = factory_.create_IConduitImage(); // conduit image object
                data.reserve( img.getMaxData() ); // we know the max number of items possible to store
                unsigned int head_size, data_size; // size of the raw data we are sending
                
                // Load IDs into a vector (currently they are semi-colon delimited)
                std::string id_string;
                std::vector<FacebookId> ids_vector;
                unsigned int i = 0;
                while ( ids[i] != '\0' )
                {
                    id_string = "";
                    while (ids[i] != ';')
                    {
                        id_string.push_back( ids[i++] );
                    }
                    FacebookId id_object( id_string );
                    ids_vector.push_back(id_object);
                    i++;
                }
                
                // Add the user's ID
                ids_vector.push_back( id_ );
                
                // Load the file, leaving room for the encryption header
                head_size = 0;// crypto_.calculateHeaderSize( ids_vector.size() );
                data_file.open( data_filename, std::ios::binary );
                if(!data_file.is_open()) {
                    std::cout << "Error opening data file." << std::endl;
                    return 1;
                }
                data_file.seekg(0, std::ios::end);
                data_size = data_file.tellg(); // get the length of the file 
                // read the file into the data byte vector
                data_file.seekg(0, std::ios::beg);
                data = std::vector<byte>( head_size, (byte) '|' );
                data.resize(head_size + data_size);
                data_file.read((char*) &data[head_size], data_size);
                /*
                // Generate header and encrypt the data
                try {crypto_.encryptMessage(ids_vector, data);}
                catch (EncryptionException &e) {
                  std::cout << "Error encrypting: " << e.what() << std::endl;
                  return 4;
                }
                
                // Add error correction code
                try {fec_.encode( data );}
                catch (FecEncodeException &e) {
                  std::cout << "Error adding error correction code: " << e.what() << std::endl;
                  return 2;
                }
                */
                // Load the template image file into a ConduitImage object
                try {img.load( template_filename );}
                catch (cimg_library::CImgInstanceException &e) {
                  std::cout << "Error loading template image: " << e.what() << std::endl;
                  return 3;
                }
              
                // Store the data vector in the image
                try {img.implantData( data );}
                catch (ConduitImageImplantException &e) {
                    std::cout << "Error implanting data: " << e.what() << std::endl;
                    return 4;
                }
              
                // Save our final image (in a lossless format)
                try {img.save( img_out_filename );}
                catch (cimg_library::CImgInstanceException &e) {
                  std::cout << "Error saving output image: " << e.what() << std::endl;
                  return 3;
                }
                
                // Return with succes
                return 0;
            }
        
            unsigned int decryptFileFromImage
            (
                const char*  img_in_filename,
                const char*  data_filename
            )
            {
                std::ofstream       data_file;  // data file object	 
                IConduitImage&      img = factory_.create_IConduitImage();        // source image object
                std::vector<byte>   data;       // for data bytes we wish to transfer
                unsigned int head_size;         // size of header so we can skip
                
                
                // Load the source image file into a CImg object
                try {img.load( img_in_filename );}
                catch (cimg_library::CImgInstanceException &e) {
                  std::cout <<  "Error loading source image: " << e.what() << std::endl;
                  return 1;
                }
                
                // Check that the dimensions are exactly 720x720
                if (img.width() != 720 || img.height() != 720) {
                  std::cout << "Error extracting data: wrong image dimensions." << std::endl;
                  return 2;
                }
                
                // Decode from image 
                try {img.extractData( data );}
                catch (ConduitImageExtractException &e) {
                    std::cout << "Error extracting data: " << e.what() << std::endl;
                    return 2;
                }
                /*
                // Correct errors
                try {fec_.decode( data );}
                catch (FecDecodeException &e) {
                  std::cout << "Error decoding FEC codes: " << e.what() << std::endl;
                  return 3;
                }
                
                // Retrieve the message key from the header and decrypt the data
                try {crypto_.decryptMessage(data);}
                catch (DecryptionException &e) {
                  std::cout << "Error decrypting: " << e.what() << std::endl;
                  return 4;
                }
                */
                // Save data to a file, skipping the header
                head_size = 0; //crypto_.retrieveHeaderSize(data);
                data_file.open( data_filename, std::ios::binary);
                if(!data_file.is_open()) {
                  std::cout << "Error creating data file:" << std::endl;
                  return 1;
                }
                data_file.write((char*) &data[head_size], data.size()-head_size );
                
                // Return with success
                return 0; 
            }
            
            //! Take a message string and encrypt into a Facebook-ready string. Both will be null terminated.
            const char* encryptString
            (
                const char* ids,
                const char*  input
            ) const
            {
                            
                // Load IDs into a vector (currently they are semi-colon delimited)
                std::string id_string;
                std::vector<FacebookId> ids_vector;
                unsigned int i = 0;
                while ( ids[i] != '\0' )
                {
                    id_string = "";
                    while (ids[i] != ';')
                    {
                        id_string.push_back( ids[i++] );
                    }
                    FacebookId id_object( id_string );
                    ids_vector.push_back(id_object);
                    i++;
                }
                                
                // Add the user's ID
                ids_vector.push_back( id_ );
                
                // Copy data into a vector<byte> **INCLUDING** the null terminal. Leave room for the header at the start.
                unsigned int head_size = crypto_.calculateHeaderSize( ids_vector.size() );
                std::vector<byte> data( head_size, (byte)0);
                for (unsigned int i=0; i<strlen(input)+1; i++) data.push_back( input[i] );
                                        
                // Generate header and encrypt the data
                try {crypto_.encryptMessage(ids_vector, data);}
                catch (EncryptionException &e) {
                  std::cout << "Error encrypting: " << e.what() << std::endl;
                  return "";
                }
                
                // Create Facebook ready string to upload
                std::string str = string_codec_.binaryToFbReady( data );
                char* cstr = new char [str.size()+1];
                strcpy(cstr, str.c_str());
                
                return cstr;
            }
            
            //! Take string from Facebook and decrypt to a message string. Both will be null terminated.
            const char* decryptString
            (
                const char*  input
            ) const
            {
                // Copy input into a std::string (strips null terminal)
                std::string str( input );
            
                // Decode the string into a byte array
                std::vector<byte> data = string_codec_.fbReadyToBinary( str );
                
                // Retrieve the message key from the header and decrypt the data
                try {crypto_.decryptMessage(data);}
                catch (DecryptionException &e) {
                  std::cout << "Error decrypting: " << e.what() << std::endl;
                  return "You do not have sufficient privileges to read this message.";
                }
                
                // Bytes should now be original (UTF8, null terminated) message. We must skip the header.
                unsigned int head_size = crypto_.retrieveHeaderSize(data);
                char* cstr = new char [data.size() - head_size];
                strcpy( cstr, (char*) &data[head_size] );
                return cstr;
            }
            
            //! Calculate bit error rate (for debugging purposes).
            unsigned int calculateBitErrorRate
            (
                const char*  file1_path,
                const char*  file2_path
            ) const
            {
                // ifstream objects
                std::ifstream file1, file2;
                // open file 1
                file1.open( file1_path, std::ios::binary );
                if(!file1.is_open()) {
                  std::cout << "Error opening data file 1:" << std::endl;
                  return 1;
                }
                // and file 2
                file2.open( file2_path, std::ios::binary );
                if(!file2.is_open()) {
                  std::cout << "Error opening data file 2:" << std::endl;
                  return 1;
                }
                // vector byte arrays 
                std::vector<byte> data1, data2;
                // load file 1 into array
                file1.seekg(0, std::ios::end);
                size_t file1_size = file1.tellg(); // get the length of the file
                file1.seekg(0, std::ios::beg);
                data1.resize( file1_size );
                // read into array
                file1.read((char*) &data1[0], file1_size);
                // load file 2 into array
                file2.seekg(0, std::ios::end);
                size_t file2_size = file2.tellg(); // get the length of the file
                file2.seekg(0, std::ios::beg);
                data2.resize( file2_size );
                // read into array
                file2.read((char*) &data2[0], file2_size);
                
                // Calculate the BER and output to std::cout
                unsigned int total=0, errors=0; std::vector<byte> e;
                for (unsigned int i=0; i<data1.size(); i++) {
                  byte error = data1[i] ^ data2[i];
                  e.push_back( error );
                  std::bitset<8> bs( error );
                  errors += bs.count();
                  total += 8;
                }
                std::cout << errors << " errors in " << total <<" bits. ";
                std::cout << ((float) errors) / total << std::endl;
                
                // Write out errors to a file
                std::ofstream error_file;
                error_file.open( "/home/chris/Desktop/errors.bin", std::ios::binary);
                if(!error_file.is_open()) {
                  std::cout << "Error creating error file:" << std::endl;
                  return 1;
                }
                error_file.write((char*) &e[0], e.size() );
                
                return 0;
            }
    };

    
    //! Haar wavelet AES-256/RSA-2048 based abstract factory pattern.
    /**
        This implementation uses the Haar wavelet method to store data in images. Errors are corrected with the Schifra Reed Solomon library using a (255,223) code rate. Cryptographic protocols are provided by the Botan library using AES-256 and 2048-bit public key RSA.
    */
    class Haar256Factory : public ILibFactory
    {
            IConduitImage& create_IConduitImage() const {
                return *(new HaarConduitImage());
            }
            ICrypto& create_ICrypto() const {
                return *(new BotanRSACrypto<32,256>());
            }
            IFec& create_IFec() const {
                return *(new ReedSolomon255Fec());
            }
            IStringCodec& create_IStringCodec() const {
                return *(new ShiftB0StringCodec());
            }
    };
    
    //! AES-256/RSA-2048 based abstract factory pattern, using upsampling to store data in images.
    /**
        This implementation stores 4 bits in each 8-bit byte. Errors are corrected with the Schifra Reed Solomon library using a (255,223) code rate. Cryptographic protocols are provided by the Botan library using AES-256 and 2048-bit public key RSA.
    */
    class Upsampled256Factory : public ILibFactory
    {
            IConduitImage& create_IConduitImage() const {
                return *(new Upsampled4ConduitImage());
            }
            ICrypto& create_ICrypto() const {
                return *(new BotanRSACrypto<32,256>());
            }
            IFec& create_IFec() const {
                return *(new ReedSolomon255Fec());
            }
            IStringCodec& create_IStringCodec() const {
                return *(new ShiftB0StringCodec());
            }
    };
}