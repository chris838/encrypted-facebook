// Include required standard C++ headers.
#include <vector>

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

    /para Main library class and Firefox interface
    
    The main libray (abstract) class \ref ICore implements IFirefoxInterface - the external interface exposed to Firefox via the C wrapper code. When a concrete subclass is instantiated it requires a Facebook user ID with which a profile directory is created (if not present). Clean up of any cached images is performed both on construction and destruction.
    
    Since only one user should be logged in to Facebook at one time (per running Firefox profile at least) subclasses of ICore <emph>should be designed as singleton (single instance) classes</emph>. However is non trivial to enforce this behavour with the abstract class. On instantiation the class also creates (as members) instances of the cryptograhic and error correction classes which group related functions. This class also contains methods for UTF8 string decoding and encoding.
    
    The class also forms part of what can be described as something similiar to an abstract factory pattern - in that any concrete subclass of \ref ICore determines (to some extent) which concrete cryptography, error correction and image subclasses are used. This is because there exists some interdependancy between exact implementations of these library sub-components. For example, the error correction implementation must match the bit error rate inccured when the image carrier implementation undergoes compression. 
    
    \para Cryptograhy and error correction 
    
    The \ref ICrypto class defines an interface to the cryptography algorithms used by the rest of the library. These include algorithms for both symmetric and asymmetric encryption/decryption, and also for public/private key pair generation. The \ref IFec class similarly defines an interface to the forward error correction algorithms.
    
    For both library classes, subclass creation and destruction is managed by the main library class which will extend \ref ICore. A concrete \ref ICore subclass must select concrete subclasses of \ref IFec and \ref ICrypto.
    
    \para Conduit image
    
    The \ConduitImage abstract class extends the CImg class by defining functionality to implant and extract data to the image in a reasonably JPEG compression immune fashion. Like the \ref IFec and \ref ICrypto library classes the concrete implemention is specified by the concrete implementation of \ref ICore.    
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

    ///! Firefox interface definition.
    class IFirefoxInterface
    {
        public :
            // Initialise (involves constructing singleton) and close library
            virtual unsigned int initialise() = 0;
            virtual void close() = 0;
            
            // Photo encryption/decryption
            virtual unsigned int encryptPhoto
            (
                const FacebookId ids[],
                const unsigned int len,
                const char*  data_filename,
                const char*  dst_filename
            ) const = 0;
            virtual unsigned int decryptPhoto
            (
                const char*  src_filename,
                const char*  data_filename
            ) const = 0;
            
            // String encryption/decryption
            virtual unsigned int encryptString
            (
                const char*  input,
                      char*  output
            ) const = 0;
            virtual unsigned int decryptString(
                const char*  input,
                      char*  output
            ) const = 0;
            
            // Debug function for calculating BER
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
            // For data which we wish to encrypt
            virtual unsigned int calculateHeaderSize( unsigned int numOfIds ) const = 0;
            virtual void encryptMessage
            (
                std::vector<FacebookId>& ids,
                std::vector<byte>& data // with header-size offset before data bytes begin
            ) = 0;
            // For data we wish to decrypt:
            virtual unsigned int retrieveHeaderSize( std::vector<byte>& data ) const = 0;
            virtual void decryptMessage( std::vector<byte>& data ) = 0;
    };
    
    //! Abstract class definining the interface for the error correction algorithms..
    class IFec
    {
        public :
            virtual std::vector<byte>& encode( std::vector<byte>& data);
            virtual std::vector<byte>& decode( std::vector<byte>& data);
    };

    //! Conduit image class, with functionality for implanting and extracting data.
    class IConduitImage : public cimg_library::CImg<char>
    {
        public :
            //! Get the maximum ammount of data that can be stored in this implementation
            virtual unsigned int getMaxSize() = 0;
            //! Check how much data (if any) is stored in the current image
            virtual unsigned short read_size() = 0;
            virtual void implant_data( std::vector<char>& data ) = 0;
            virtual void extract_data( std::vector<char>& data ) = 0;
    };
    
    //! Core library class.
    /**
        This is the main library class. Other library components (cryptography and error correction) are grouped into further classes and instantiated as attribute members.
    */
    class ICore : public IFirefoxInterface
    {
        // FEC and crytography code grouped into classes and accessed as member variables.
        ICrypto* crypto;
        IFec* fec;
        // initialisers for those library classes...
        void init_crypto()
            {crypto = &create_ICrypto();}
        void init_fec()
            {fec = &create_IFec();}
        
        // Abstract factory pattern-like object generators
        virtual IConduitImage& create_IConduitImage() = 0;
        virtual ICrypto& create_ICrypto() = 0;
        virtual IFec& create_IFec() = 0;
    };
    
    //! Botan cryptography class using 128-bit AES and 2048-bit RSA.
    /**
        This class uses the Botan cryptography library to perform encryption and decryption in place. AES-128 and 2048-bit RSA are the symmetric and asymmetric (respectively) schemes employed. The header consists of two length bytes specifying the number of recipients, the message IV in plaintext, and a sequence of (Facebook ID, message-key) pairs. Each message-key is encrypted under the public key of the Facebook ID it is paired with.
    */
    class BotanCrypto : public ICrypto
    {
        // Generate or set a random IV and random message key
        void generateNewIv()
            {iv =  Botan::InitializationVector(rng, 16);} // a random 128-bit iv
        void generateNewMessageKey()
            {key =  Botan::SymmetricKey(rng, 16);} // a random 128-bit key
            
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
        
        virtual void getCipheredMessageKey
        (
            FacebookId & id,
            std::vector<byte> & data
        ) = 0;
        
        //! Create the crypto header using a new IV and message key
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
            std::string ivstr(32, 'x');
            for (unsigned int i=0; i<32; i++)
                ivstr[i] = data[i];
            setIv( ivstr );
            
            std::string keystr(32, 'x');
            for (unsigned int i=0; i<32; i++)
                keystr[i] = data[32+i];
            setMessageKey( keystr );
        }
        
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
                Botan::Pipe encrypter(get_cipher("AES-128/CFB", key, iv, Botan::ENCRYPTION));
                encrypter.start_msg();
                encrypter.write((Botan::byte*) &data[hs], ms ); 
                encrypter.end_msg();
                encrypter.read((Botan::byte*) &data[hs], ms );
            }
            
            unsigned int retrieveHeaderSize(std::vector<byte>& data) const
                {return iv.as_string().length() + key.as_string().length();}
            
            void decryptMessage( std::vector<byte>& data )
            {
                // note - this will try make a valid header from the start of the data and use it to set the IV and message key. If the image is not valid or we are not on the intended recipients list this may well throw an exception.
                parseCryptoHeader(data);
            
                // perform the decryption, skipping the first <header size> bytes
                unsigned int hs = retrieveHeaderSize(data), ds = data.size(), ms = ds - hs;
                Botan::Pipe decrypter(get_cipher("AES-128/CFB", key, iv, Botan::DECRYPTION));
                decrypter.start_msg();
                decrypter.write((Botan::byte*) &data[hs], ms );  
                decrypter.end_msg();
                decrypter.read((Botan::byte*) &data[hs], ms );
            }
    };
    class ReedSolomonFec : public IFec
    {};
    class HaarConduitImage : public IConduitImage
    {};
    class Core : public ICore
    {};
    
}