#ifndef EFB_ILIBFACTORY_H
#define EFB_ILIBFACTORY_H

/**
################################################################################
    This file contains the interface definition for the abstract factory.
################################################################################
*/

// eFB Library sub-component includes
#include "crypto/ICrypto.h"
#include "fec/IFec.h"
#include "conduit_image/IConduitImage.h"
#include "string_codec/IStringCodec.h"

namespace efb {
    
    //! Abstract factory interface.
    /**
        An abstract factory class used to generate library sub-components. It defines a group of compatible or complementary sub-component implementations.
    */
    class ILibFactory
    {
        public :
            //! Conduit image object creater.
            virtual IConduitImage& create_IConduitImage() const = 0;
            //! Cryptography library object creater.
            virtual ICrypto& create_ICrypto() const = 0;
            //! Forward error correction library object creater.
            virtual IFec& create_IFec() const = 0;
            //! String codec object creater.
            virtual IStringCodec& create_IStringCodec() const = 0;
    };
    
}

#endif //EFB_ILIBFACTORY_H