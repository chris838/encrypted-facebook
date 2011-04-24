#ifndef EFB_BASICLIBRARY_H
#define EFB_BASICLIBRARY_H

// Standard libary includes
#include <bitset>
#include <iostream>
#include <fstream>
#include <iterator>
#include <numeric>
#include <sys/stat.h> 

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
                unsigned int final_size=0; // size before we insert into image
                
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
                head_size = crypto_.calculateHeaderSize( ids_vector.size() );
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
                
                // Pad the data to full length
                final_size = data.size();
                if (fec_.codeLength(final_size+3) > img.getMaxData()) {
                    std::cout << "File is too big." << std::endl;
                    return 1;
                }
                srand( time(NULL) );
                while ( fec_.codeLength( data.size()+3+1 ) < img.getMaxData() )
                {
                    data.push_back( (byte) rand() );
                }
                // Add the length to the end
                data.push_back( (final_size >> 0) & 0x000000ff );
                data.push_back( (final_size >> 8) & 0x000000ff );
                data.push_back( (final_size >> 16) & 0x000000ff );
                
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
                
                // delete the image object
                delete &img;
                
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
                
                // Remove padding outside FEC blocksize
                while ( fec_.codeLength( fec_.dataLength( data.size() ) ) != data.size() )
                {
                    data.pop_back();
                }
                
                // Correct errors
                try {fec_.decode( data );}
                catch (FecDecodeException &e) {
                  std::cout << "Error decoding FEC codes: " << e.what() << std::endl;
                  return 3;
                }
                
                // Remove padding
                unsigned int final_size =
                    0   |   (data[data.size()-3] << 0)
                        |   (data[data.size()-2] << 8)
                        |   (data[data.size()-1] << 16);
                while (data.size() > final_size) data.pop_back();
                
                // Retrieve the message key from the header and decrypt the data
                try {crypto_.decryptMessage(data);}
                catch (DecryptionException &e) {
                  std::cout << "Error decrypting: " << e.what() << std::endl;
                  return 4;
                }
                
                // Save data to a file, skipping the header
                head_size = crypto_.retrieveHeaderSize(data);
                data_file.open( data_filename, std::ios::binary);
                if(!data_file.is_open()) {
                  std::cout << "Error creating data file:" << std::endl;
                  return 1;
                }
                data_file.write((char*) &data[head_size], data.size()-head_size );
                
                // delete the image object
                delete &img;
                
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
                std::vector<byte> data;
                try {
                    data = string_codec_.fbReadyToBinary( str );
                } catch (StringDecodeException &e) {
                    std::cout << "UTF8 decode failed: " << e.what() << std::endl;
                }
                
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
            )
            {
                //return testImageCoding();
            
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
            
            
            //! Testing function for image coding methods
            unsigned int testImageCoding()
            {
                // Initialise
                IConduitImage& img = factory_.create_IConduitImage();
                unsigned int cap = img.getMaxData();
                std::vector<byte> data1, data2;
                srand( time(NULL) );
                //
                unsigned long data_left;
                unsigned int count;
                std::vector<unsigned int> terrors, ttotal, enctime, dectime;
                struct timeval start, end;
                long mtime, seconds, useconds;
                std::ofstream log_file;
                std::vector<byte> e;
                
                // For each JPEG quality factor
                for (int jpeg_rate = 80; jpeg_rate <=90; jpeg_rate ++ ) {
                    
                    
                    
                    // Create log file
                    std::stringstream s("");
                    s  << "/home/chris/Desktop/testing/scaled4_results_" << jpeg_rate << ".log";
                    struct stat stFileInfo;
                    int intStat = stat( s.str().c_str(),&stFileInfo);
                    if (intStat==0) continue;
                    
                    if (log_file.is_open()) log_file.close();
                    log_file.open( s.str().c_str() , std::ios::binary);
                    
                    
                    if(!log_file.is_open()) {
                      std::cout << "Error creating log file:" << std::endl;
                      return 1;
                    }
                    //log_file << "Image ID" << ", ";
                    //log_file << "Size" << ", ";
                    //log_file << "Errors" << ", ";
                    //log_file << "Encoding time" << ", ";
                    //log_file << "Decoding time" << std::endl;
                    
                    data_left = 1024*1024*10; // 10 MiB
                    data_left = data_left - (data_left % cap);
                    count = 0;
                    terrors.clear();
                    ttotal.clear();
                    enctime.clear();
                    dectime.clear();
                    
                    
                    
                    // For each test
                    while ( data_left > 0 ) {
                        
                        // Create a conduit image object
                        const char* template_filename =  "/home/chris/Desktop/src.bmp";
                        // Load the template image file into a ConduitImage object
                        try {img.load( template_filename );}
                        catch (cimg_library::CImgInstanceException &e) {
                            std::cout << "Error loading template image: " << e.what() << std::endl;
                            return 3;
                        }
                        
                        // Create work file
                        std::stringstream ss("");
                        ss << "/home/chris/Desktop/testing/file" << jpeg_rate << ".jpg";
                        std::string img_out_filename = ss.str();      
                        std::string img_in_filename = ss.str();      
                        
                        // Create a randomised byte vector to encode
                        data1.clear();
                        if (data_left > cap) {
                            // create a full cap file
                            data1.resize(cap);
                            for (unsigned int i=0; i<cap; i++) data1[i] = rand();
                            data_left -= cap;
                        } else {
                            // create a partial cap file
                            data1.resize(data_left);
                            for (unsigned int i=0; i<data_left; i++) data1[i] = rand();
                            data_left = 0;
                        }
                        
                        // Encode that file, timing how long this takes
                        // START TIMING
                        gettimeofday(&start, NULL);
                        try {img.implantData( data1 );}
                        catch (ConduitImageImplantException &e) {
                            std::cout << "Error implanting data: " << e.what() << std::endl;
                            return 4;
                        }
                        // STOP TIMING
                        gettimeofday(&end, NULL);
                        seconds  = end.tv_sec  - start.tv_sec;
                        useconds = end.tv_usec - start.tv_usec;
                        mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
                        enctime.push_back( mtime );
                        
                        // Save our final image (in a lossless format)
                        try {img.save_jpeg( img_out_filename.c_str(), jpeg_rate );}
                        catch (cimg_library::CImgInstanceException &e) {
                          std::cout << "Error saving output image: " << e.what() << std::endl;
                          return 3;
                        }
                        
                        // Decode that file, timing how long this takes
                        // Load the image file into a CImg object
                        try {img.load( img_in_filename.c_str() );}
                        catch (cimg_library::CImgInstanceException &e) {
                          std::cout <<  "Error loading source image: " << e.what() << std::endl;
                          return 1;
                        }
                        // Decode from image
                        data2.clear();
                        // START TIMING
                        gettimeofday(&start, NULL);
                        try {img.extractData( data2 );}
                        catch (ConduitImageExtractException &e) {
                            std::cout << "Error extracting data: " << e.what() << std::endl;
                            return 2;
                        }
                        // STOP TIMING
                        gettimeofday(&end, NULL);
                        seconds  = end.tv_sec  - start.tv_sec;
                        useconds = end.tv_usec - start.tv_usec;
                        mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
                        dectime.push_back( mtime );
                        
                        // Calculate the bit error rate (BER)
                        unsigned int total=0, errors=0;
                        for (unsigned int i=0; i<data1.size(); i++) {
                          byte error = data1[i] ^ data2[i];
                          e.push_back( error );
                          std::bitset<8> bs( error );
                          errors += bs.count();
                          total += 8;
                        }
                        terrors.push_back( errors );
                        ttotal.push_back( total );
                        std::cout << "Processed image " << count;
                        std::cout << ". Bytes left: " << data_left;
                        std::cout << ". Last errors: " << errors << " in " << total << " bits." << std::endl;
                        
                        // Log all the error rates and timing measurements
                        //log_file << count << ", ";
                        //log_file << total << ", ";
                        //log_file << errors << ", ";
                        //log_file << enctime.back() << ", ";
                        //log_file << dectime.back() << std::endl;
                        // Write out errors to a file
                        log_file.write((char*) &e[0], e.size() );
                        e.clear();
                        
                        count++;
                    }
                 
                }
                
                // Store in log file
                
                return 0;
            }
    
            //! Testing function for UTF-8 encoding
            unsigned int testUTF8Decode(std::vector<byte> data)
            {
                string_codec_.binaryToFbReady(data);
                
                return 0;
            }
            
    };
}

#endif //EFB_BASICLIBRARY_H