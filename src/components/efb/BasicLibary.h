#ifndef EFB_BASICLIBRARY_H
#define EFB_BASICLIBRARY_H

// Standard libary includes
#include <bitset>
#include <iostream>
#include <fstream>

// eFB Library sub-component includes
#include "IeFBLibrary.h"
#include "ILibFactory.h"
    
namespace efb {
        
    //! Basic library implementation.
    /**
        This library implementation takes a Facebook ID on instantiation which defines the directory within which file I/O takes place.
    */
    class BasicLibary : public IeFBLibrary
    {        
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
              
        private :
            const ILibFactory& factory_;
            ICrypto& crypto_; // not const, has state (key and iv)
            const IFec& fec_;
            const IStringCodec& string_codec_;
            const FacebookId id_;
            const std::string working_directory_;
    };
}

#endif //EFB_BASICLIBRARY_H