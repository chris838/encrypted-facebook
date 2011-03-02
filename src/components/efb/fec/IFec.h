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
            //! Encode data by appending error correction codes.
            virtual void encode( std::vector<byte>& data) const =0;
            //! Decode (correct) data in place.
            virtual void decode( std::vector<byte>& data) const =0;
    };

}

#endif //EFB_IFEC_H