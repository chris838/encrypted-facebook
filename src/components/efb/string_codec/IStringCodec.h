#ifndef EFB_ISTRINGCODEC_H
#define EFB_ISTRINGCODEC_H

// Standard libary includes
#include <vector>

// eFB Library sub-component includes
#include "../Common.h"

namespace efb {
    
    // String decode exception
    struct StringDecodeException : public ExtractException {
        StringDecodeException(const std::string &err) : ExtractException(err) {} };
    
    //! Abstract class defining a codec to encode and decode strings to and from Facebook.
    class IStringCodec
    {
        public :
            virtual std::string binaryToFbReady( std::vector<byte>& data) const = 0;
            virtual std::vector<byte> fbReadyToBinary( std::string& str ) const = 0; 
    };

}

#endif //EFB_ISTRINGCODEC_H