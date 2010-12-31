#ifndef DATA_CARRIER_IMG_CLASS_H
#define DATA_CARRIER_IMG_CLASS_H


#define JPEG_INTERNALS

#define cimg_verbosity 0     // Disable modal window in CImg exceptions.
#define cimg_use_jpeg 1
#include "CImg.h"

#include <vector>

//! Class representing a CImg<char> with support for implanting and extracting data.
/**
    This is a specific CImg subclass (each pixel being of type char) which supports data
    implantation and extraction in a relatively JPEG compression immune fashion. Implanted data
    will still require error correction coding to be fully recovered.
 */
struct DataCarrierImg : public cimg_library::CImg<char>
{
    private :
        //! Flag to state whether the image has been prepared for implantation
        bool _is_formatted;
        
        void write_size
        (
            unsigned short len
        );
        
        // Wavelet transforms for image encoding/decoding
        void
        Haar2D_DWT(
            unsigned int 			y0,
            unsigned int 			x0
        ) const;
        int div_floor(int a, int b) const;
        void
        Haar2D_DWTi(
            unsigned int 			y0,
            unsigned int 			x0
        ) const;
        
        // Encoding/decoding to a block
        void
        EncodeInBlock(
            unsigned int 			y0,
            unsigned int 			x0,
            unsigned char 			a,
            unsigned char 			b,
            unsigned char 			c
        ) const;
        void
        DecodeFromBlock(
            unsigned int 			y0,
            unsigned int 			x0,
            std::vector<char>		& data
        ) const;
        
    public :

        void
        format_for_implantation();

        void
        implant_data
        (
            std::vector<char>& data
        );
              
        std::vector<char>&
        extract_data() const;
        
        unsigned short
        read_size() const;
            
        //! Default constructor.
        DataCarrierImg() : cimg_library::CImg<char>(), _is_formatted(false) {}
};


using namespace cimg_library;

// The \ref DataCarrierImgExtractionException class is thrown when data extraction fails
struct DataCarrierImgExtractionException : public CImgException
{
    DataCarrierImgExtractionException(const char *const format, ...) { _cimg_exception_err("DataCarrierImgExtractionException",false); }
};

// The \ref DataCarrierImgImplantationException class is thrown when data implantation fails
struct DataCarrierImgImplantationException : public CImgException
{
  DataCarrierImgImplantationException(const char *const format, ...) { _cimg_exception_err("DataCarrierImgImplantationException",false); }
};

#endif // DATA_CARRIER_IMG_CLASS_H