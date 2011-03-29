#ifndef EFB_SCHIFRAFEC_H
#define EFB_SCHIFRAFEC_H

// Shiffra Reed Solomon library includes
#include "schifra/schifra_galois_field.hpp"
#include "schifra/schifra_galois_field_polynomial.hpp"
#include "schifra/schifra_sequential_root_generator_polynomial_creator.hpp"
#include "schifra/schifra_reed_solomon_encoder.hpp"
#include "schifra/schifra_reed_solomon_decoder.hpp"
#include "schifra/schifra_reed_solomon_block.hpp"
#include "schifra/schifra_error_processes.hpp"

// Library sub-component includes
#include "IFec.h"

namespace efb {
    
    //! Schifra Reed Solomon error correction library template class where code rate is (N,M)
    template <int N, int M>
    class SchifraFec : public IFec
    {        
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
            
            //! Calculate the overall size after adding error correction.
            unsigned int codeLength( unsigned int data_length) const
            {
                unsigned int blocks = (data_length / M) + (data_length % M == 0 ? 0 : 1);
                return blocks * N;
            }
            
            //! Calculate the data size before adding error correction.
            unsigned int dataLength( unsigned int code_length) const
            {
                unsigned int blocks = code_length / N;
                return blocks * M;
            }
            
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
        
        private :
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
    };

}

#endif //EFB_SCHIFRAFEC_H