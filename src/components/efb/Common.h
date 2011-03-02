#ifndef EFB_EXCEPTIONS_H
#define EFB_EXCEPTIONS_H

/**
################################################################################
    This file contains definitions common across library components.
################################################################################
*/

// Standard library includes
#include <string>
#include <sstream>
#include <stdexcept>

namespace efb {
    // Declaration of implant and extract exceptions.    
    struct ImplantException : public std::runtime_error {
        ImplantException(const std::string &err) : std::runtime_error(err) {} };
    struct ExtractException : public std::runtime_error {
        ExtractException(const std::string &err) : std::runtime_error(err) {} };
        
    // Typedef for 'byte'
    typedef unsigned char   byte;
    
    // Key exception, thrown when dealing with identities and cryptographic keys.
    struct IdException : public std::runtime_error {
        IdException(const std::string &err) : std::runtime_error(err) {} };
        
    //! Structure for representing Facebook IDs, which we assume to be passed on instantiation as 15-16 byte ASCII character sequences. They can be represented by an 8-byte integer type. 
    struct FacebookId
    {
        unsigned long long int val;
        
        FacebookId( const char* str )
        {
            // Convert string to a numeric values so that key representation takes mimimal space.
            std::stringstream ss;
            ss << str;
            if ( (ss >> val).fail() ) throw IdException("Error converting Facebook ID to numeric value.");
        }
        
        FacebookId( std::string str )
        {
            // Convert string to a numeric values so that key representation takes mimimal space.
            std::stringstream ss;
            ss << str;
            if ( (ss >> val).fail() ) throw IdException("Error converting Facebook ID to numeric value.");
        }
        
        FacebookId() : val(0) {}        
    };
    
    //! Overloaded operator so type can be used in a keymap
    bool operator< (const FacebookId& fid1, const FacebookId& fid2)
    {
        return fid1.val < fid2.val;
    }
}

#endif //EFB_EXCEPTIONS_H