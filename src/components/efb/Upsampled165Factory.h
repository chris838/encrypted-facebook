#ifndef EFB_UPSAMPLED169FACTORY_H
#define EFB_UPSAMPLED169FACTORY_H

// eFB Library sub-component includes
#include "ILibFactory.h"
#include "crypto/BotanRSACrypto.h"
#include "fec/ReedSolomon255Fec.h"
#include "conduit_image/Upsampled3ConduitImage.h"
#include "string_codec/ShiftB0StringCodec.h"

namespace efb {
    
    //! Abstract factory which uses upsampling to store data in images. Approx. capacity 165KiB.
    /**
        This implementation stores 3 bits in each 8-bit pixel using upsampling to achieve an error rate of <0.02%. This rate is then reduced using Reed Solomon error correction (based on the Shifra library) with a code rate of (255,223). The final maximum capacity is approximately 165 KiB (or 169926 bytes exactly). The Botan library is used for cryptographic functions. The standards implemented are AES-256 and RSA-2048 as reccomended by NIST "Recommendations for Key Management - Part 1: General (Revised) - page 63". A slightly shifted UTF8 codec is used to avoid problem characters with low UTF8 code point values and surrogate pairs.  
    */
    class Upsampled165Factory : public ILibFactory
    {
        public :
            ICrypto& create_ICrypto() const {
                return *(new BotanRSACrypto<32,256>());
            }
            IFec& create_IFec() const {
                return *(new ReedSolomon255Fec());
            }
            IConduitImage& create_IConduitImage() const {
                return *(new Upsampled3ConduitImage());
            }
            IStringCodec& create_IStringCodec() const {
                return *(new ShiftB0StringCodec());
            }
    };
    
}

#endif //EFB_UPSAMPLED169FACTORY_H