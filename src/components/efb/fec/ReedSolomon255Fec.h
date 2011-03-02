#ifndef EFB_REEDSOLOMON255FEC_H
#define EFB_REEDSOLOMON255FEC_H

// Library sub-component includes
#include "SchifraFec.h"

namespace efb {
    
    //! Reed Solomon error correction using the Schifra library with 255-byte block size providing a (255,223) code rate.
    class ReedSolomon255Fec : public SchifraFec<255,223>
    {
        public :
            //! Default constructor.
            ReedSolomon255Fec() : SchifraFec<255,223>
            (
                8,      // field_descriptor
                120,    // generator_polynommial_index
                32,      // generator_polynommial_root_count
                schifra::galois::primitive_polynomial_size06,
                schifra::galois::primitive_polynomial06
            ) {}
    };

}

#endif //EFB_REEDSOLOMON255FEC_H