#ifndef EFB_HAAR20KIBFACTORY_H
#define EFB_HAAR20KIBFACTORY_H

// eFB Library sub-component includes
#include "ILibFactory.h"
#include "crypto/BotanRSACrypto.h"
#include "fec/ReedSolomon255Fec.h"
#include "conduit_image/HaarConduitImage.h"
#include "string_codec/ShiftB0StringCodec.h"

namespace efb {
    
    //! Abstract factory which uses Haar wavelets to store data in images. Approx. capacity 20 KiB.
    /**
        This implementation uses the Haar wavelet method to store data in images. 3 bytes are stored in each 64-pixel block with an error rate of >1% (TODO - CHECK THIS). This rate is then reduced using Reed Solomon error correction (based on the Shifra library) with a code rate of (255,223). The final maximum capacity is approximately 20 KiB (or 21,185 bytes exactly). The Botan library is used for cryptographic functions. The standards implemented are AES-256 and RSA-2048 as reccomended by NIST "Recommendations for Key Management - Part 1: General (Revised) - page 63". A slightly shifted UTF8 codec is used to avoid problem characters with low UTF8 code point values and surrogate pairs.  
    */
    class Haar20KiBFactory : public ILibFactory
    {
            ICrypto& create_ICrypto() const {
                return *(new BotanRSACrypto<32,256>());
            }
            IConduitImage& create_IConduitImage() const {
                return *(new HaarConduitImage());
            }
            IFec& create_IFec() const {
                return *(new ReedSolomon255Fec());
            }
            IStringCodec& create_IStringCodec() const {
                return *(new ShiftB0StringCodec());
            }
    };
    
}

#endif //EFB_HAAR20KIBFACTORY_H