#ifndef EFB_ICONDUITIMAGE_H
#define EFB_ICONDUITIMAGE_H

// Standard library includes
#include <vector>

// CImg library includes with JPEG support
#define JPEG_INTERNALS
#define cimg_verbosity 0     // Disable modal window in CImg exceptions.
#define cimg_use_jpeg 1
#include "CImg.h"

// Library sub-component includes
#include "../Common.h"

namespace efb {
    
    // Conduit image exceptions
    struct ConduitImageImplantException : public ImplantException {
        ConduitImageImplantException(const std::string &err) : ImplantException(err) {} };
    struct ConduitImageExtractException : public ExtractException {
        ConduitImageExtractException(const std::string &err) : ExtractException(err) {} };
        
    //! Abstract class defining a conduit image, with functionality for implanting and extracting data.
    class IConduitImage : public cimg_library::CImg<byte>
    {
        public :
            //! Get the maximum ammount of data that can be stored in this implementation.
            virtual unsigned int getMaxData() = 0;
            //! Implant data.
            virtual void implantData( std::vector<byte>& data ) = 0;
            //! Extract data.
            virtual void extractData( std::vector<byte>& data ) = 0;
    };
    
}
    
#endif //EFB_ICONDUITIMAGE_H
    