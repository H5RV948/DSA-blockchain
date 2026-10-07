#pragma once
#include <cstring>

// ============================================================
// EncabezadoBloque.h
// Proyecto Integrador Mini-Blockchain — TC1031
// ============================================================
// Contiene los metadatos de un bloque: su posición en la cadena,
// cuántas transacciones tiene, y los hashes de encadenamiento.
//
// NOTA: los campos prevHash y blockHash son marcadores de posición
// por ahora (se inicializan con "0000..."). La función hash real
// se implementará en la Entrega No. 3.
// ============================================================

struct EncabezadoBloque {

    int  height;        // posición del bloque en la cadena (0 = génesis)
    int  txCount;       // número de transacciones confirmadas en este bloque
    char prevHash[65];  // hash del bloque anterior (placeholder hasta Entrega 3)
    char blockHash[65]; // hash propio de este bloque  (placeholder hasta Entrega 3)

    // --------------------------------------------------------
    // Constructor por defecto
    // Inicializa height y txCount en 0.
    // Inicializa prevHash y blockHash con "0000000000000000".
    // --------------------------------------------------------
    EncabezadoBloque() {
        // TODO: implementar
        // Sugerencia: usar strcpy para inicializar los hashes
    }

    // --------------------------------------------------------
    // Constructor completo
    // Parámetros:
    //   h    — altura del bloque
    //   txc  — número de transacciones
    //   prev — hash del bloque anterior (cadena de hasta 64 chars)
    //   own  — hash propio del bloque   (cadena de hasta 64 chars)
    // --------------------------------------------------------
    EncabezadoBloque(int h, int txc, const char* prev, const char* own) {
        // TODO: implementar
        // Sugerencia: usar strncpy y asegurarse de terminar con '\0'
    }
};
