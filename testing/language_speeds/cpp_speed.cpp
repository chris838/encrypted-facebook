#include <botan/botan.h>
#include <botan/rsa.h>
#include <botan/pubkey.h>
#include <botan/look_pk.h>

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <sys/time.h>
// Standard libary includes
#include <bitset>
#include <iostream>
#include <fstream>
#include <iterator>
#include <numeric>

using namespace std;

int main( int argc, const char* argv[] )
{
    
    Botan::LibraryInitializer init;
    
    Botan::AutoSeeded_RNG rng;
    Botan::SymmetricKey key(rng, 16); // a random 256-bit key
    Botan::InitializationVector iv(rng, 16);
    
    // The algorithm we want is specified by a string
    Botan::Pipe p(get_cipher("AES-128/CFB", iv, key, Botan::ENCRYPTION));
    Botan::Pipe p2(get_cipher("AES-128/CFB", iv, key, Botan::DECRYPTION));
    
    stringstream ss (stringstream::in | stringstream::out);
    stringstream ss2 (stringstream::in | stringstream::out);
    
    for (unsigned int i=0; i<16*100000;i++) {
        ss << "a";
    }
    
                    struct timeval start, end;
                long mtime, seconds, useconds;
                
                        gettimeofday(&start, NULL);
    for (unsigned int i=0; i<1000; i++) {
    p.start_msg();
    ss >> p; 
    p.end_msg();
    p2.start_msg();
    ss >> p2;
    p2.end_msg();
    }
                        gettimeofday(&end, NULL);
                        seconds  = end.tv_sec  - start.tv_sec;
                        useconds = end.tv_usec - start.tv_usec;
                        mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
                        
    std::cout << mtime << std::endl;
}