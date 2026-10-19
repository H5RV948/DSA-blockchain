#pragma once
#include <cstring>


struct EncabezadoBloque {

    int  height;       
    int  txCount;       
    char prevHash[65];  
    char blockHash[65]; 

    // Constructor default
  
    EncabezadoBloque() : height(0), txCount(0) {
        strcpy(prevHash, "0000000000000000");
        strcpy(blockHash, "0000000000000000");
    }

    // Constructor completo

    EncabezadoBloque(int h, int txc, const char* prev, const char* own)
        : height(h), txCount(txc) {
        strncpy(prevHash, prev, 64);
        prevHash[64] = '\0';
        strncpy(blockHash, own, 64);
        blockHash[64] = '\0';
    }
};
