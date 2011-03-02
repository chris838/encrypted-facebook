#ifndef EFB_REEDSOLOMON15FEC_H
#define EFB_REEDSOLOMON15FEC_H

// Library sub-component includes
#include "SchifraFec.h"

namespace efb {
    
    //! Reed Solomon error correction using the Schifra library with 15-byte block size providing a (15,8) code rate.
    class ReedSolomon15Fec : public SchifraFec<15,8>
    {
        public :
            //! Default constructor.
            ReedSolomon15Fec() : SchifraFec<15,8>
            (
                4,      // field_descriptor
                0,    // generator_polynommial_index
                7,      // generator_polynommial_root_count
                schifra::galois::primitive_polynomial_size01,
                schifra::galois::primitive_polynomial01
            ) {}
    };
    
}

#endif //EFB_REEDSOLOMON15FEC_H