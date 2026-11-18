#pragma once
#include <cstring>

// ============================================================
// EncabezadoBloque.h  (actualizado en la Entrega 4)
// Todos los campos hash son char[17]: 16 caracteres hex + '\0'.
//
//   prevHash   -> blockHash del bloque anterior (encadena la cadena)
//   merkleRoot -> raiz del arbol de Merkle de las transacciones
//   blockHash  -> hash de (height, txCount, prevHash, merkleRoot)
//
// Orden de calculo: primero merkleRoot, luego blockHash
// (blockHash incluye al merkleRoot).
// ============================================================

const int TAM_HASH = 17;

struct EncabezadoBloque {

    int  height;
    int  txCount;
    char prevHash[TAM_HASH];
    char blockHash[TAM_HASH];    // se calcula con calcularBlockHash()
    char merkleRoot[TAM_HASH];   // se calcula con calcularMerkleRoot()

    // Constructor default: todos los hashes en ceros
    EncabezadoBloque() : height(0), txCount(0) {
        strcpy(prevHash,   "0000000000000000");
        strcpy(blockHash,  "0000000000000000");
        strcpy(merkleRoot, "0000000000000000");
    }

    // Constructor completo. Si los hashes recibidos son mas largos
    // (por ejemplo los de 64 chars del NDJSON original) se truncan a 16.
    // merkleRoot empieza en ceros: se calcula al cerrar/confirmar el bloque.
    EncabezadoBloque(int h, int txc, const char* prev, const char* own)
        : height(h), txCount(txc) {
        copiarHash(prevHash, prev);
        copiarHash(blockHash, own);
        strcpy(merkleRoot, "0000000000000000");
    }

    // Copia a lo mas 16 caracteres y siempre deja el '\0'
    static void copiarHash(char* destino, const char* origen) {
        strncpy(destino, origen, TAM_HASH - 1);
        destino[TAM_HASH - 1] = '\0';
    }
};
