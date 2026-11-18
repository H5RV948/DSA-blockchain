#ifndef HASH_MERKLE_H
#define HASH_MERKLE_H

// HashMerkle.h — Módulo del curso TC1031
// Proporciona una función hash de 16 caracteres hexadecimales,
// el cálculo del merkleRoot y el hash del encabezado de bloque.
//
// NO MODIFICAR ESTE ARCHIVO.
// Si los campos de tu Transaccion tienen nombres distintos,
// adapta SOLO la función serializarTx() en tu propia copia.

#include <cstring>
#include <cstdio>
#include "DArray.h"
#include "Bloque.h"

// ─────────────────────────────────────────────────────────────
// hashSimple
// Calcula un hash de 16 caracteres hexadecimales para cualquier
// cadena de entrada. Basada en FNV-1a con mezcla de bits.
//
// Parámetros:
//   entrada  — cadena a hashear (terminada en '\0')
//   salida   — arreglo de al menos 17 chars; se escribe el hash
//              y el terminador '\0'
// ─────────────────────────────────────────────────────────────
void hashSimple(const char* entrada, char* salida) {
    unsigned long long h = 14695981039346656037ULL;   // FNV offset basis
    for (const char* p = entrada; *p; ++p) {
        h ^= (unsigned char)*p;
        h *= 1099511628211ULL;                        // FNV prime
        h ^= (h >> 33);
        h *= 0xff51afd7ed558ccdULL;
        h ^= (h >> 33);
    }
    const char* hex = "0123456789abcdef";
    for (int i = 15; i >= 0; i--) {
        salida[i] = hex[h & 0xF];
        h >>= 4;
    }
    salida[16] = '\0';
}

// ─────────────────────────────────────────────────────────────
// serializarTx  (interna, no forma parte de la API pública)
// Produce una cadena con todos los campos de la transacción
// concatenados, suficiente para detectar cualquier modificación.
//
// ADAPTADA (Equipo 4 — auditoría de accesos privilegiados):
// usa los campos de nuestra Transaccion: txId_, marcaTiempo_,
// user_, evento_, objeto_, estado_, comando_ y privilegio_.
// Es la ÚNICA parte modificada de este módulo.
// ─────────────────────────────────────────────────────────────
static void serializarTx(const Transaccion* tx, char* buf, int bufLen) {
    // Concatenar todos los campos relevantes separados por '|'.
    // El separador '|' no aparece en ningún dato real del proyecto,
    // por lo que no hay riesgo de colisión entre transacciones distintas.
    // snprintf trunca si buf es pequeño; 512 chars es más que suficiente.
    char ts[24];
    snprintf(ts, sizeof(ts), "%lld", (long long)tx->marcaTiempo_);
    snprintf(buf, bufLen, "%s|%s|%s|%s|%s|%s|%s|%d",
        tx->txId_,
        ts,
        tx->user_,
        tx->evento_,
        tx->objeto_,
        tx->estado_,
        tx->comando_,
        tx->privilegio_);
}

// ─────────────────────────────────────────────────────────────
// calcularMerkleRoot
// Construye el árbol de Merkle de las transacciones de un bloque
// y escribe la raíz en destino.
//
// Algoritmo (bottom-up, nivel por nivel con DArray):
//   1. Hoja i = hashSimple( serialización de tx[i] )
//   2. Si el nivel tiene un número impar de nodos, el último
//      se duplica antes de subir.
//   3. Nodo padre = hashSimple( hijo_izq + hijo_der )
//   4. Se repite hasta que queda un solo nodo: el merkleRoot.
//
// Caso especial: bloque sin transacciones → destino = "0000000000000000"
//
// Parámetros:
//   bloque   — bloque cuyos hashes de transacciones se combinan
//   destino  — arreglo de al menos 17 chars; recibe el merkleRoot
// ─────────────────────────────────────────────────────────────
void calcularMerkleRoot(const Bloque& bloque, char* destino) {
    int n = bloque.numTx();
    if (n == 0) {
        strcpy(destino, "0000000000000000");
        return;
    }

    // nivel actual: hashes de las hojas
    DArray<char*> nivel;
    for (int i = 0; i < n; i++) {
        char serial[512];
        serializarTx(bloque.obtenerTx(i), serial, sizeof(serial));
        char* h = new char[17];
        hashSimple(serial, h);
        nivel.agregar(h);
    }

    // subir nivel por nivel hasta la raíz
    while (nivel.tamanoActual() > 1) {
        // duplicar el último nodo si el nivel es impar
        if (nivel.tamanoActual() % 2 != 0) {
            char* copia = new char[17];
            strcpy(copia, nivel.obtenerEn(nivel.tamanoActual() - 1));
            nivel.agregar(copia);
        }

        DArray<char*> siguiente;
        for (int i = 0; i < nivel.tamanoActual(); i += 2) {
            char concat[33];                          // 16 + 16 + '\0'
            strcpy(concat, nivel.obtenerEn(i));
            strcat(concat, nivel.obtenerEn(i + 1));
            char* h = new char[17];
            hashSimple(concat, h);
            siguiente.agregar(h);
        }

        for (int i = 0; i < nivel.tamanoActual(); i++)
            delete[] nivel.obtenerEn(i);

        nivel = siguiente;
    }

    strcpy(destino, nivel.obtenerEn(0));
    delete[] nivel.obtenerEn(0);
}

// ─────────────────────────────────────────────────────────────
// calcularBlockHash
// Calcula el hash del encabezado del bloque actual.
// El hash combina los metadatos del bloque (height, txCount,
// prevHash, merkleRoot) en una sola cadena y aplica hashSimple.
//
// IMPORTANTE: calcularMerkleRoot debe haberse llamado antes
// y el resultado debe estar en encabezado.merkleRoot.
//
// Parámetros:
//   bloque   — bloque cuyo hash se calcula
//   destino  — arreglo de al menos 17 chars; recibe el blockHash
// ─────────────────────────────────────────────────────────────
void calcularBlockHash(const Bloque& bloque, char* destino) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%d|%d|%s|%s",
        bloque.encabezado.height,
        bloque.encabezado.txCount,
        bloque.encabezado.prevHash,
        bloque.encabezado.merkleRoot);
    hashSimple(buf, destino);
}

#endif // HASH_MERKLE_H
