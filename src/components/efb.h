// Include required standard C++ headers.
#include <vector>
#include <sstream>

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

//! Namespace containing all the Encrypted Facebook C++ code.
/**
    This namespace contains several abstract base classes along with their concrete subclasses. In places we use abstract classes in place of interfaces - hence the 'I' prefix. Some abstract classes do not particularly conform to the definition of interface, however for continuity we still use the 'I' prefix to denote that these classes are abstract and must be extended.

    \par Main library class and Firefox interface
    
    The main libray (abstract) class \ref ICore implements IFirefoxInterface - the external interface exposed to Firefox via the C wrapper code. When a concrete subclass is instantiated it requires a Facebook user ID with which a profile directory is created (if not present). Clean up of any cached images is performed both on construction and destruction.
    
    On instantiation the class also creates (as members) instances of the cryptograhic and error correction classes which group related functions. This class also contains methods for UTF8 string decoding and encoding.
    
    The class also forms part of what can be described as something similiar to an abstract factory pattern - in that any concrete subclass of \ref ICore determines (to some extent) which concrete cryptography, error correction and image subclasses are used. This is because there exists some interdependancy between exact implementations of these library sub-components. For example, the error correction implementation must match the bit error rate inccured when the image carrier implementation undergoes compression. 
    
    \par Cryptograhy and error correction algorithms
    
    The \ref ICrypto class defines an interface to the cryptography algorithms used by the rest of the library. These include algorithms for both symmetric and asymmetric encryption/decryption, and also for public/private key pair generation. The \ref IFec class similarly defines an interface to the forward error correction algorithms.
    
    For both library classes, subclass creation and destruction is managed by the main library class which will extend \ref ICore. A concrete \ref ICore subclass will select concrete subclasses of \ref IFec and \ref ICrypto.
    
    \par Conduit image class
    
    The \ref IConduitImage abstract class extends the CImg library class (CImg) by adding functionality to implant and extract data to the image in a reasonably JPEG compression immune fashion. Like the \ref IFec and \ref ICrypto library classes the concrete implemention is specified by the concrete implementation of \ref ICore.    
*/

namespace efb {

    typedef unsigned short  Unicode2Bytes;
    typedef unsigned int    Unicode4Bytes;
    typedef unsigned char   byte;
    typedef struct
    {
        unsigned int hi;
        unsigned int lo;
    }                       FacebookId; 

    //! Firefox interface definition.
    class IFirefoxInterface
    {
        public :
            //! Initialise library with Facebook ID and extension directory.
            virtual unsigned int initialise() = 0;
            //! Close the library and wipe any volatile directories.
            virtual void close() = 0;
            
            //! Encrypt a file into an image for the supplied array of recipients.
            virtual unsigned int encryptFileInImage
            (
                const FacebookId ids[],
                const unsigned int len,
                const char*  src_filename,
                const char*  img_out_filename
            ) const = 0;
            //! Attempt to extract and decrypt a file from an image.
            virtual unsigned int decryptFileFromImage
            (
                const char*  img_in_filename,
                const char*  dst_filename
            ) const = 0;
            
            //! Take a message string and encrypt into a Facebook-ready string. Both will be null terminated.
            virtual unsigned int encryptString
            (
                const char*  input,
                      char*  output
            ) const = 0;
            //! Take string from Facebook and decrypt to a message string. Both will be null terminated.
            virtual unsigned int decryptString(
                const char*  input,
                      char*  output
            ) const = 0;
            
            //! Debug function for calculating BER
            virtual unsigned int calculateBitErrorRate
            (
                const char*  file1,
                const char*  file2
            ) const = 0;
    };
    
    //! Abstract class definining the interface for the cryptographic algorithms.
    class ICrypto
    {
        public :
            //! Returns the predicted header size so we can leave room before encryption.
            virtual unsigned int calculateHeaderSize( unsigned int numOfIds ) const = 0;
            //! Writes header and encrypts message. Assumes there is a header-size offset before data bytes begin.
            virtual void encryptMessage
            (
                std::vector<FacebookId>& ids,
                std::vector<byte>& data // with header-size offset before data bytes begin
            ) = 0;
            //! Parses any data header and attempts to decrypt the data
            virtual void decryptMessage( std::vector<byte>& data ) = 0;
    };
    
    //! Abstract class definining the interface for the error correction algorithms.
    class IFec
    {
        public :
            //! Encode data into a new vector.
            virtual std::vector<byte>& encode( std::vector<byte>& data);
            //! Decode (correct) data into a new vector.
            virtual std::vector<byte>& decode( std::vector<byte>& data);
    };

    //! Abstract class defining a conduit image, with functionality for implanting and extracting data.
    class IConduitImage : public cimg_library::CImg<char>
    {
        public :
            //! Get the maximum ammount of data that can be stored in this implementation.
            virtual unsigned int getMaxSize() = 0;
            //! Check how much data (if any) is stored in the current image.
            virtual unsigned short read_size() = 0;
            //! Implant data.
            virtual void implant_data( std::vector<char>& data ) = 0;
            //! Extract data.
            virtual void extract_data( std::vector<char>& data ) = 0;
    };
    
    //! Core library class.
    /**
        This is the main library class. Other library components (cryptography and error correction) are grouped into further classes and instantiated as attribute members.
    */
    class ICore : public IFirefoxInterface
    {
        //! Instance of the cryptographic library code.
        ICrypto* crypto;
        //! Instance of the forward error correction library code.
        IFec* fec;
        //! Initialiser for the cryptograhic library code.
        void init_crypto()
            {crypto = &create_ICrypto();}
        //! Initialiser for the forward error correction library code.
        void init_fec()
            {fec = &create_IFec();}
        //! Abstract factory pattern object creater.
        virtual IConduitImage& create_IConduitImage() = 0;
        //! Abstract factory pattern object creater.
        virtual ICrypto& create_ICrypto() = 0;
        //! Abstract factory pattern object creater.
        virtual IFec& create_IFec() = 0;
    };
    
    //! Botan cryptography class using N-byte AES and M-byte RSA.
    /**
        This class uses the Botan cryptography library to perform encryption and decryption in place. AES and RSA are the symmetric and asymmetric (respectively) schemes employed. The template variables <N,M> determine the key lengths. The header consists of two length bytes specifying the number of recipients, the message IV in plaintext, and a sequence of (Facebook ID, message-key) pairs. Each message-key is encrypted under the public key of the Facebook ID it is paired with.
    */
    template <int N, int M>
    class BotanCrypto : public ICrypto
    {
        // Generate or set a random IV and random message key
        void generateNewIv()
            {iv =  Botan::InitializationVector(rng, N);} // a random N-byte iv
        void generateNewMessageKey()
            {key =  Botan::SymmetricKey(rng, N);} // a random N-byte key
        void setIv( std::string & ivstr )
            {iv =  Botan::InitializationVector( ivstr );}
        void setMessageKey( std::string & keystr)
            {key =  Botan::SymmetricKey( keystr );}
            
        // Append the current (plaintext) IV and (ciphered) message key to the supplied vector
        void getIv( std::vector<byte> & data )
        {
            std::string ivstr = iv.as_string();
            for (unsigned int i=0; i<ivstr.length(); i++)
            data.push_back( ivstr[i] );
        }
        void getCipheredMessageKey
        (
            FacebookId & id,
            std::vector<byte> & data
        ) {}
        
        //! Create the crypto header using a new IV and message key.
        void createCryptoHeader
        (
            std::vector<FacebookId> & ids,
            std::vector<byte> & data
        )
        {
            generateNewIv();
            generateNewMessageKey();
            // create the header and write to the start of the vector
            getIv( data );
            // TODO - for now just append the key in plaintext
            std::string keystr = key.as_string();
            for (unsigned int i=0; i<keystr.length(); i++)
                data.push_back( keystr[i] );
        }
        
        //! Attempty to parse a crypto header - this will retrieve and set the message key and IV.
        void parseCryptoHeader
        (
            std::vector<byte> & data
        )
        {
            std::string ivstr(iv.length(), 'x');
            for (unsigned int i=0; i<iv.length(); i++)
                ivstr[i] = data[i];
            setIv( ivstr );
            // TODO - right now the key is just appended in plaintext
            std::string keystr(key.length(), 'x');
            for (unsigned int i=0; i<key.length(); i++)
                keystr[i] = data[iv.length()+i];
            setMessageKey( keystr );
        }
        
        //! Get the header size 
        unsigned int retrieveHeaderSize(std::vector<byte>& data) const
            {return iv.as_string().length() + key.as_string().length();}
            
        //! Botan library members
        Botan::LibraryInitializer init;
        Botan::AutoSeeded_RNG rng;
        Botan::SymmetricKey key;
        Botan::InitializationVector iv;
        
        public :
            
            BotanCrypto()
            {
                // so we can work out their sizes properly...
                generateNewIv();
                generateNewMessageKey();
            }
            
            unsigned int calculateHeaderSize( unsigned int numOfIds ) const
                {return iv.as_string().length() + key.as_string().length();}
                
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
                    get_cipher(ss.str(), key, iv, Botan::ENCRYPTION));
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
                    get_cipher(ss.str(), key, iv, Botan::DECRYPTION));
                decrypter.start_msg();
                decrypter.write((Botan::byte*) &data[hs], ms );  
                decrypter.end_msg();
                decrypter.read((Botan::byte*) &data[hs], ms );
            }
    };
    //! Shifra Reed Solomon error correction library where code rate is (N,M)
    template <int N, int M>
    class ShifraFec : public IFec
    {
        // Finite Field Parameters
        const std::size_t field_descriptor_;
        const std::size_t generator_polynommial_index_;
        const std::size_t generator_polynommial_root_count_;
        // Reed Solomon Code Parameters
        const std::size_t code_length_;
        const std::size_t fec_length_;
        const std::size_t data_length_;
        // Finite Field and Generator Polynomials
        schifra::galois::field field_;
        schifra::galois::field_polynomial generator_polynomial_;
        schifra::sequential_root_generator_polynomial_creator polynomial_creator_;
        // Encoder and Decoder (Codec)
        schifra::reed_solomon::encoder<N,M-N> encoder_;
        schifra::reed_solomon::decoder<N,M-N> decoder_;
    
        //! Generate FEC code from a message   
        void encodeBlock(
            std::string  	& message,
            std::string	& fec
        ) const
        {
            // Pad out to full size
            message = message + std::string(
                data_length_ - message.length(),static_cast<unsigned char>(0x00));
            // Instantiate RS Block For Codec
            schifra::reed_solomon::block<N,N-M> block;
            // Transform message into Reed-Solomon encoded codeword
            if (!encoder_.encode(message,block))
                // !!!TODO!!! - throw exception
                std::cout << "Error - Critical encoding failure!" << std::endl;
            fec = std::string(fec_length_,static_cast<unsigned char>(0x00));
            block.fec_to_string(fec);
        }
        
        //! Try and fix any errors in a block-size message using FEC code
        void decodeBlock(
            std::string & message_plus_errors,
            std::string	& fec,
            std::string	& message
        ) const
        {
            // Instantiate RS Block For Codec
            schifra::reed_solomon::block<N,N-M> block(message_plus_errors, fec);
            // Try and fix any errors in the message
            if (!decoder_.decode(block))
              // !!!TODO!!! - throw exception
               std::cout << "Error - Critical decoding failure!" << std::endl;
            message = std::string(data_length_,static_cast<unsigned char>(0x00));
            block.data_to_string(message);
        }
        
        public :
            //! Default constructor.
            ShifraFec(
                std::size_t field_descriptor,
                std::size_t generator_polynommial_index,
                std::size_t generator_polynommial_root_count
            ) :
                // Initialisation for const attributes
                field_descriptor_(field_descriptor),
                generator_polynommial_index_(generator_polynommial_index),
                generator_polynommial_root_count_(generator_polynommial_root_count),
                code_length_(N),
                fec_length_(N-M),
                data_length_(M),
                // Instantiate Finite Field and Generator Polynomials
                field_
                (
                    field_descriptor_,
                    schifra::galois::primitive_polynomial_size06,
                    schifra::galois::primitive_polynomial06
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
    };
    
    //! Reed Solomon error correction using the Shifra library with 255-byte block size providing a (255,223) code rate.
    class ReedSolomon255Fec : public ShifraFec<255,223>
    {
        public :
            //! Default constructor.
            ReedSolomon255Fec() : ShifraFec<255,223>
            (
                8,      // field_descriptor
                120,    // generator_polynommial_index
                32      // generator_polynommial_root_count
            ) {}
    };
    
    class HaarConduitImage : public IConduitImage
    {
    
    };
    
    
    class Core : public ICore
    {};
}