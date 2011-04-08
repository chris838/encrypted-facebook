#include "c_wrapper.h"
#include <iostream>

int main( int argc, const char* argv[] )
{
    
    IeFBLibrary* lib;
    
    lib = create_IeFBLibrary("1234567","asdasd/asdasd");
      
      
      
    calculateBitErrorRate( lib,"","" );

}

