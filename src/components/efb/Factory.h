#ifndef EFB_FACTORY_H
#define EFB_FACTORY_H

/**
    This file contains the interface definition for the abstract factory along with definitions for any classes which implement that interface.
*/

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
    
        //! Haar wavelet AES-256/RSA-2048 based abstract factory pattern.
    /**
        This implementation uses the Haar wavelet method to store data in images. Errors are corrected with the Schifra Reed Solomon library using a (255,223) code rate. Cryptographic protocols are provided by the Botan library using AES-256 and 2048-bit public key RSA.
    */
    class Haar256Factory : public ILibFactory
    {
            IConduitImage& create_IConduitImage() const {
                return *(new HaarConduitImage());
            }
            ICrypto& create_ICrypto() const {
                return *(new BotanRSACrypto<32,256>());
            }
            IFec& create_IFec() const {
                return *(new ReedSolomon255Fec());
            }
            IStringCodec& create_IStringCodec() const {
                return *(new ShiftB0StringCodec());
            }
    };
    
    //! AES-256/RSA-2048 based abstract factory pattern, using upsampling to store data in images.
    /**
        This implementation stores 4 bits in each 8-bit byte. Errors are corrected with the Schifra Reed Solomon library using a (255,223) code rate. Cryptographic protocols are provided by the Botan library using AES-256 and 2048-bit public key RSA.
    */
    class Upsampled256Factory : public ILibFactory
    {
            IConduitImage& create_IConduitImage() const {
                return *(new Upsampled4ConduitImage());
            }
            ICrypto& create_ICrypto() const {
                return *(new BotanRSACrypto<32,256>());
            }
            IFec& create_IFec() const {
                return *(new ReedSolomon255Fec());
            }
            IStringCodec& create_IStringCodec() const {
                return *(new ShiftB0StringCodec());
            }
    };

}

#endif //EFB_FACTORY_H