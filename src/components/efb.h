// Include required standard C++ headers.
#include <stdio.h>
#include <string>
#include <bitset>
#include <vector>
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

//! Namespace containing all the Encrypted Facebook C++ code.
/**
    This namespace contains several abstract base classes along with their concrete subclasses. In places we use abstract classes in place of interfaces - hence the 'I' prefix. Some abstract classes do not particularly conform to the definition of interface, however for continuity we still use the 'I' prefix to denote that these classes are abstract and must be extended.

    \par Main library class and Firefox interface
    
    The main libray (abstract) class \ref ICore implements IeFBLibrary - the external interface exposed to Firefox via the C wrapper code. When a concrete subclass is instantiated it requires a Facebook user ID with which a profile directory is created (if not present). Clean up of any cached images is performed both on construction and destruction.
    
    On instantiation the class also creates (as members) instances of the cryptograhic and error correction classes which group related functions. This class also contains methods for UTF8 string decoding and encoding.
    
    The class also forms part of what can be described as something similiar to an abstract factory pattern - in that any concrete subclass of \ref ICore determines (to some extent) which concrete cryptography, error correction and image subclasses are used. This is because there exists some interdependancy between exact implementations of these library sub-components. For example, the error correction implementation must match the bit error rate inccured when the image carrier implementation undergoes compression. 
    
    \par Cryptograhy and error correction algorithms
    
    The \ref ICrypto class defines an interface to the cryptography algorithms used by the rest of the library. These include algorithms for both symmetric and asymmetric encryption/decryption, and also for public/private key pair generation. The \ref IFec class similarly defines an interface to the forward error correction algorithms.
    
    For both library classes, subclass creation and destruction is managed by the main library class which will extend \ref ICore. A concrete \ref ICore subclass will select concrete subclasses of \ref IFec and \ref ICrypto.
    
    \par Conduit image class
    
    The \ref IConduitImage abstract class extends the CImg library class (CImg) by adding functionality to implant and extract data to the image in a reasonably JPEG compression immune fashion. Like the \ref IFec and \ref ICrypto library classes the concrete implemention is specified by the concrete implementation of \ref ICore.    
*/

//namespace efb {

    typedef unsigned short  Unicode2Bytes;
    typedef unsigned int    Unicode4Bytes;
    typedef unsigned char   byte;
    
    typedef long long int FacebookId;
    
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

    //! Library interface definition.
    class IeFBLibrary
    {
        public :
            //! Initialise library with Facebook ID and extension directory.
            virtual unsigned int initialise() = 0;
            //! Close the library and wipe any volatile directories.
            virtual void close() = 0;
            
            //! Encrypt a file into an image for the supplied array of recipients.
            virtual unsigned int encryptFileInImage
            (
                const char ids[],
                const unsigned int len,
                const char*  data_filename,
                const char*  img_out_filename
            ) = 0;
            //! Attempt to extract and decrypt a file from an image.
            virtual unsigned int decryptFileFromImage
            (
                const char*  img_in_filename,
                const char*  data_filename
            ) = 0;
            
            //! Take a message string and encrypt into a Facebook-ready string. Both will be null terminated.
            virtual unsigned int encryptString
            (
                const char ids[],
                const unsigned int len,
                const char*  input,
                      char*  output
            ) = 0;
            //! Take string from Facebook and decrypt to a message string. Both will be null terminated.
            virtual unsigned int decryptString(
                const char*  input,
                      char*  output
            ) = 0;
            
            //! Debug function for calculating BER
            virtual unsigned int calculateBitErrorRate
            (
                const char*  file1_path,
                const char*  file2_path
            ) const = 0;
    };
    
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
    };
    
    //! Abstract class definining the interface for the error correction algorithms.
    class IFec
    {
        public :
            //! Encode data by appending error correction codes.
            virtual void encode( std::vector<byte>& data)=0;
            //! Decode (correct) data in place.
            virtual void decode( std::vector<byte>& data)=0;
    };

    //! Abstract class defining a conduit image, with functionality for implanting and extracting data.
    class IConduitImage : public cimg_library::CImg<byte>
    {
        public :
            //! Get the maximum ammount of data that can be stored in this implementation.
            virtual unsigned int getMaxData() = 0;
            //! Check how much data (if any) is stored in the current image.
            virtual unsigned short readSize() = 0;
            //! Implant data.
            virtual void implantData( std::vector<byte>& data ) = 0;
            //! Extract data.
            virtual void extractData( std::vector<byte>& data ) = 0;
    };
    
    //! Core library class.
    /**
        This is the main library class. Other library components (cryptography and error correction) are grouped into further classes and instantiated as attribute members.
    */
    class ICore : public IeFBLibrary
    {
        //! Abstract factory pattern object creater.
        virtual IConduitImage* create_IConduitImage() = 0;
        //! Abstract factory pattern object creater.
        virtual ICrypto* create_ICrypto() = 0;
        //! Abstract factory pattern object creater.
        virtual IFec* create_IFec() = 0;
        
        protected :
            //! Instance of the cryptographic library code.
            ICrypto* crypto;
            //! Instance of the forward error correction library code.
            IFec* fec;
            // Default constructor
            ICore( ICrypto* crypto, IFec* fec ) :
                crypto( crypto ),
                fec( fec )
            {}
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

            unsigned int retrieveHeaderSize(std::vector<byte>& data) const
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
    //! Schifra Reed Solomon error correction library where code rate is (N,M)
    template <int N, int M>
    class SchifraFec : public IFec
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
        schifra::reed_solomon::encoder<N,N-M> encoder_;
        schifra::reed_solomon::decoder<N,N-M> decoder_;
    
        //! Generate FEC code from a message   
        void encodeBlock(
            std::string  	& message,
            std::string	& fec
        ) const
        {
            // Pad out to full size
            message = message + std::string(
                data_length_ - message.length(),static_cast<byte>(0x00));
            // Instantiate RS Block For Codec
            schifra::reed_solomon::block<N,N-M> block;
            // Transform message into Reed-Solomon encoded codeword
            if (!encoder_.encode(message,block))
                throw FecEncodeException("Error - Critical encoding failure!");
            fec = std::string(fec_length_,static_cast<byte>(0x00));
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
                throw FecDecodeException("Error - Critical decoding failure!");
            message = std::string(data_length_,static_cast<byte>(0x00));
            block.data_to_string(message);
        }
        
        public :
            //! Default constructor.
            SchifraFec(
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
            
            //! Encode data by appending error correction codes.
            void encode( std::vector<byte>& data) {}
            //! Decode (correct) data in place.
            void decode( std::vector<byte>& data) {}
            
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
                32      // generator_polynommial_root_count
            ) {}
    };
    //! Conduit image class which uses the Haar wavelet tranform to store data in low frequency image components.
    class HaarConduitImage : public IConduitImage
    {
    
        //! Flag to state whether the image has been prepared for implantation
        bool is_formatted_;
        
        //! Format the image in preparation for implantation.
        /**
            This operation will resize the image to 720x720x1 and truncate the colour channels, as only data storage in single-channel (greyscale) image is supported. JPEG compression requires an (imperfect) colour space transform from RGB to YCrCb which complicates using colour images for data storage. Even worse - Facebook's JPEG compression process uses chrominance subsampling. However, this does mean that discarding the additional two chrominance channels only results in a %50 reduction in maximum potential data storage capacity.        
         */
        void formatForImplantation()
        {
            // Format the image to 720x720, greyscale, single slice (resample using Lanczos)
            resize(720,720,1,-1,6);
            channel(0);
            
            is_formatted_ = true;
        }
        
        //! Encode 3 bytes of data to the Haar coefficients of an 8x8 block
        /**
            This operation assumes we have performed the Haar transform (two iterations) on the block in question. We store the provided 3 bytes of data in the most significant bits of 4 Haar coefficents which represent the low-frequency component of the image.
         */
        void encodeInBlock
        (
            byte 	    a,
            byte 	    b,
            byte 	    c,
            short int       temp[8][8]
        )
        {  
          temp[0][0] 	= (a & 0xfc) | 0x02;
          temp[1][0] 	= (b & 0xfc) | 0x02;
          temp[0][1] 	= (c & 0xfc) | 0x02;
          temp[1][1]	= ((a & 0x03) <<6) | ((b & 0x03) <<4) | ((c & 0x03) <<2) | 0x02;
        }
        
               
        //! Helper function to decode triple modular redundancy coded bytes
        byte tripleModR
        (
          byte a,
          byte b,
          byte c
        ) const
        {
          return ((a&b)|(a&c)|(b&c));
        }
        
        void decodeFromBlock(
            short int temp[8][8],
            std::vector<byte> & data
        ) const
        {
            byte p1,p2,p3,p4;
            p1 = temp[0][0];
            p2 = temp[1][0];
            p3 = temp[0][1];
            p4 = temp[1][1];
            
            byte a,b,c;
            a = (p1 & 0xfc) | ((p4 & 0xc0) >> 6);
            b = (p2 & 0xfc) | ((p4 & 0x30) >> 4);
            c = (p3 & 0xfc) | ((p4 & 0x0c) >> 2);
            
            data.push_back( a );
            data.push_back( b );
            data.push_back( c );
        }
        
        
        //! Perform the Haar Discrete Wavelet transform on an 8x8 temp block.
        /**
            Perform the Haar Discrete Wavelet Transform (with lifting scheme so the inverse can be performed losslessly). The result is written to the supplied 8x8 array, since we cannot do this in place (we would need an extra sign bit for 3/4 of the 64 pixels).
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
                    temp2[i][j] = divFloor( operator()(x0+2*i, y0+j) +
                                            operator()(x0+2*i+1, y0+j),
                                            2);
                    // difference
                    temp2[4+i][j] = operator()(x0+2*i, y0+j) -
                                    operator()(x0+2*i+1, y0+j);
                }
            }
            // For each column...
            for (unsigned int i=0; i<8; i++) {
                // Perform 1D Haar transfrom with integer lifting
                for (unsigned int j=0; j<4; j++) {
                    // average
                    temp[i][j] = divFloor( temp2[i][2*j] + temp2[i][2*j+1], 2);
                    // difference
                    temp[i][4+j] = temp2[i][2*j] - temp2[0][2*j+1];
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
        
        void truncateCoefficients( short int& p1, short int& p2, short int& m)
        {
            // If p1 or p2 lie outside the range 0-255 we must rectify this, however we *MUST* also preserve their mean value (m) as this contains data.
            if (p1<0) {p1 = 0; p2 = 2*m;}
            if (p1>255) {p1 = 255; p2 = 2*m-255;}
            if (p2<0) {p2 = 0; p1 = 2*m;}
            if (p2>255) {p2 = 255; p1 = 2*m-255;}
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
                    if (j<4) truncateCoefficients(p1,p2,temp[i][j]);
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

        public :
        
            //! Default constructor.
            HaarConduitImage() : is_formatted_(false)
            {}
            
            //! Maximum data that can be stored with this implementation.
            unsigned int getMaxData()
            {
                return (90*90*3) - 6;
            }
        
            //! Writes the size of the image (2 bytes) to the last two blocks, using triple modular redundancy.
            void writeSize
            (
                unsigned short len    
            )
            {
                short int temp[8][8];
                byte len_hi,len_lo;
                len_hi = (byte) (len >> 8);
                len_lo = (byte) len;
                //
                haar2dDwt (89*8, 88*8, temp);
                encodeInBlock(len_hi, len_hi, len_hi, temp );
                haar2dDwti(temp, 89*8, 88*8);
                //
                haar2dDwt (89*8, 89*8, temp);
                encodeInBlock(len_lo, len_lo, len_lo, temp );
                haar2dDwti(temp, 89*8, 89*8);
            }
            
            //! Attempt to read the size tag of the image, describing how many bytes of data are stored.
            unsigned short readSize()
            {
                short int temp[8][8];
                std::vector<byte> len_blocks;
                //
                haar2dDwt ( 89*8, 88*8, temp );
                decodeFromBlock( temp, len_blocks);
                haar2dDwti( temp, 89*8, 88*8 );
                //
                haar2dDwt ( 89*8, 89*8, temp );
                decodeFromBlock( temp, len_blocks);
                haar2dDwti( temp, 89*8, 89*8 );
                //
                byte len_hi, len_lo;
                len_hi = tripleModR( len_blocks[0], len_blocks[1], len_blocks[2] );
                len_lo = tripleModR( len_blocks[3], len_blocks[4], len_blocks[5] );
                
                return ((len_hi << 8) | len_lo);
            }
            
            //! Implant a specified vector<byte> into the image.
            /**
                This operation implants a data vector into the image, using 3 bytes per 8x8 pixel block. The final two blocks are reserved for storing the length of the data vector, so it may be easily recovered. Unused blocks are padded with random data. This function may throw ConduitImageExtractException, normally if the data provided is too large to store.
                
                /param data The vector<byte> that we wish to implant in the image. Must be no more than 24294 bytes (90x90 blocks, 3 bytes per block, subtract 6-byte length tag).       
             */
            void implantData
            (
                std::vector<byte>& data
            )
            {
                // Check the image is properly formatted
                if (!is_formatted_)
                    formatForImplantation();
                    
                // Check the data isn't too large, note down size and pad out to a multiple of three
                if (data.size() > getMaxData())
                    throw ConduitImageImplantException("Too much data");
                unsigned short len = data.size(); // store this for length tag later
                std::srand ( time(NULL) );
                while ( data.size() % 3 != 0) data.push_back( (char) std::rand() ); // random padding
                                
                // Loop through the image in 8x8 blocks
                unsigned int idx, i, j;
                short int temp[8][8];
                for (unsigned int block_i=0; block_i<90; block_i++) {
                  for (unsigned int block_j=0; block_j<90; block_j++) {
                        i = block_i*8; j = block_j*8;
                        // Apply the Haar transform to the block (two iterations)
                        haar2dDwt (i, j, temp);
                        // Write out 3 bytes of data to the approximation coefficients
                        idx = 3*((block_i*90) + block_j);
                        if ( idx+2 < data.size() ) {
                            // 3 bytes to one 8x8 block
                            encodeInBlock(data[idx], data[idx+1], data[idx+2], temp );
                        } else {
                            // finish with random bytes
                            encodeInBlock((byte) std::rand(), (byte) std::rand(), (byte) std::rand(), temp);
                        }
                        // Apply the inverse transform and write back to the image
                        haar2dDwti(temp, i, j);
                  }    
                }
                
                // Write the size of the data stored to the last two image blocks
                writeSize(len);
            }
            
            //! Attempt to allocate and return a new vector<byte> containing the extracted data.
            /**
                This operation allocates a new vector<byte> big of a size denoted by the length tag at the end (last two 8x8 pixel blocks) of the image. Extraction of the data is then attempted. This function may well throw ConduitImageExtractException since the image may be a normal Facebook image and not contain anything.
             */
            void extractData
            (
                std::vector<byte>&  data
            )
            {
            
                // Read the length tag from the image, check it is not too large .
                unsigned short len = readSize();
                if (len > getMaxData() )
                    throw ConduitImageImplantException("Length tag too large.");
                    
                // Reserve enough space to store the extracted data + possible padding
                data.reserve( len+3 );
                
                // Loop through the image in 8x8 blocks
                int i, j; unsigned int idx;
                short int temp[8][8];
                for (unsigned int block_i=0; block_i<90; block_i++) {
                    for (unsigned int block_j=0; block_j<90; block_j++) {
                      
                        i = block_i*8; j = block_j*8;
                        
                        // Apply the transform (two iterations) to the block
                        haar2dDwt (i, j, temp);
                        // Read in 3 bytes of data from the approximation coefficients
                        decodeFromBlock(temp, data);
                        // exit if we have enough data
                        idx = 3*((block_i*90) + block_j);
                        if ( idx+2 > len) block_i = block_j = 90;
                  }
                }
                
                // Remove any padding bytes at the end
                while (data.size() > len) data.pop_back();
            }
    };
    
    //! General library implementation using Reed Solomon FEC and Haar wavelet method for images.
    /**
        This implementation uses the Haar wavelet method to store data in images. Errors are corrected with the Schifra Reed Solomon library using a (255,223) code rate. Crytographic protocols are provided by the Botan library using AES-128 and 2048-bit public key RSA.
    */
    class Core : public ICore
   {
        public :
            //!Default contructor
            Core() :
                ICore( create_ICrypto(), create_IFec() )
            {}
            
            //! Initialise library with Facebook ID and extension directory.
            unsigned int initialise() {return 0;}
            //! Close the library and wipe any volatile directories.
            void close() {}
            
            unsigned int encryptFileInImage
            (
                const char ids[],
                const unsigned int len,
                const char*  data_filename,
                const char*  img_out_filename
            )
            {
                // !!!TODO!!! - For now we use a specific template image located on the desktop
                const char* template_filename =  "/home/chris/Desktop/src.bmp";
                //	 
                std::ifstream 	data_file; // input data file
                std::vector<byte> data; // byte array for our data bytes we wish to transfer
                IConduitImage* 	img = create_IConduitImage(); // conduit image object
                data.reserve( img->getMaxData() ); // we know the max number of items possible to store
                unsigned int head_size, data_size; // size of the raw data we are sending

                // Load the file, leaving room for the encryption header
                head_size = crypto->calculateHeaderSize(len);
                data_file.open( data_filename, std::ios::binary );
                if(!data_file.is_open()) {
                    std::cout << "Error opening data file." << std::endl;
                    return 1;
                }
                data_file.seekg(0, std::ios::end);
                data_size = data_file.tellg(); // get the length of the file 
                // read the file into the data byte vector
                data_file.seekg(0, std::ios::beg);
                data.resize(head_size + data_size);
                data_file.read((char*) &data[head_size], data_size);
            
                // Generate header and encrypt the data
                std::vector<FacebookId> ids_vector = std::vector<FacebookId>(ids, ids+len);
                try {crypto->encryptMessage(ids_vector, data);}
                catch (EncryptionException &e) {
                  std::cout << "Error encrypting: " << e.what() << std::endl;
                  return 4;
                }
             /* 
                // Add error correction code
                try {fec->encode( data );}
                catch (FecEncodeException &e) {
                  std::cout << "Error adding error correction code: " << e.what() << std::endl;
                  return 2;
                }
            */
                // Load the template image file into a ConduitImage object
                try {img->load( template_filename );}
                catch (cimg_library::CImgInstanceException &e) {
                  std::cout << "Error loading template image: " << e.what() << std::endl;
                  return 3;
                }
              
                // Store the data vector in the image
                try {img->implantData( data );}
                catch (ConduitImageImplantException &e) {
                    std::cout << "Error implanting data: " << e.what() << std::endl;
                    return 4;
                }
              
                // Save our final image (in a lossless format)
                try {img->save( img_out_filename );}
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
                IConduitImage*      img = create_IConduitImage();        // source image object
                std::vector<byte>   data;       // for data bytes we wish to transfer
                unsigned int head_size;         // size of header so we can skip
                
                // Load the source image file into a CImg object
                try {img->load( img_in_filename );}
                catch (cimg_library::CImgInstanceException &e) {
                  std::cout <<  "Error loading source image: " << e.what() << std::endl;
                  return 1;
                }
                
                // Check that the dimensions are exactly 720x720
                if (img->width() != 720 || img->height() != 720) {
                  std::cout << "Error extracting data: wrong image dimensions." << std::endl;
                  return 2;
                }
                
                // Decode from image using Haar DWT
                try {img->extractData( data );}
                catch (ConduitImageExtractException &e) {
                    std::cout << "Error extracting data: " << e.what() << std::endl;
                    return 2;
                }
                /*
                // Correct errors
                try {fec->decode( data );}
                catch (FecDecodeException &e) {
                  std::cout << "Error decoding FEC codes: " << e.what() << std::endl;
                  return 3;
                }
                */
                // Retrieve the message key from the header and decrypt the data
                try {crypto->decryptMessage(data);}
                catch (DecryptionException &e) {
                  std::cout << "Error decrypting: " << e.what() << std::endl;
                  return 4;
                }
                
                // Save data to a file, skipping the header
                head_size = crypto->retrieveHeaderSize(data);
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
            unsigned int encryptString
            (
                const char ids[],
                const unsigned int len,
                const char*  input,
                      char*  output
            ) {return 0;}
            //! Take string from Facebook and decrypt to a message string. Both will be null terminated.
            unsigned int decryptString(
                const char*  input,
                      char*  output
            ) {return 0;}
            
            //! Debug function for calculating BER
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
                unsigned int total=0, errors=0;
                for (unsigned int i=0; i<data1.size(); i++) {
                  unsigned char error = data1[i] ^ data2[i];
                  std::bitset<8> bs( error );
                  errors += bs.count();
                  total += 8;
                }
                std::cout << ((float) errors) / total << std::endl;

                return 0;
            }
            
            IConduitImage* create_IConduitImage() {return new HaarConduitImage();}
            ICrypto* create_ICrypto() {return new BotanCrypto<16,256>();}
            IFec* create_IFec() {return new ReedSolomon255Fec();}
    };
// }