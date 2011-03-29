#ifndef EFB_IFEC_H
#define EFB_IFEC_H

// Standard library includes
#include <vector>

// Library sub-component includes
#include "../Common.h"

namespace efb {
    
    // Forward error correction exceptions
    struct FecEncodeException : public ImplantException {
        FecEncodeException(const std::string &err) : ImplantException(err) {} };
    struct FecDecodeException : public ExtractException {
        FecDecodeException(const std::string &err) : ExtractException(err) {} };
        
    //! Abstract class definining the interface for the error correction algorithms.
    class IFec
    {
        public :
            //! Calculate the overall size after adding error correction.
            virtual unsigned int codeLength( unsigned int data_length) const = 0;
            //! Calculate the data size before adding error correction.
            virtual unsigned int dataLength( unsigned int code_length) const = 0;
            //! Encode data by appending error correction codes.
            virtual void encode( std::vector<byte>& data) const =0;
            //! Decode (correct) data in place.
            virtual void decode( std::vector<byte>& data) const =0;
    };

}

#endif //EFB_IFEC_H