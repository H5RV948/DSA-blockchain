#pragma once
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include "DArray.h"
#include "Transaccion.h"
#include "Bloque.h"
#include "ListaEnlazada.h"
#include "Indices.h"
#include "HashMerkle.h"

// ============================================================
// CargadorNDJSON.h
// Lee el archivo NDJSON linea por linea, sin STL.
// Cada linea es un objeto JSON con un campo "tipo":
//   "BLOQUE"      -> empieza un bloque nuevo
//   "TRANSACCION" -> pertenece al ultimo BLOQUE que se leyo
//
// Entrega 4: al cerrar cada bloque se calcula su merkleRoot y
// despues su blockHash (en ese orden). Ademas guardarNDJSON()
// escribe la cadena con el encabezado completo.
// ============================================================

const int TAM_LINEA = 1024;

// Busca "clave":"valor" en la linea y copia el valor en destino.
// Si la clave no esta, destino queda vacio.
void leerTexto(const char* linea, const char* clave, char* destino, int tamDestino) {
    char patron[64];                 // queda como:  "clave":"
    strcpy(patron, "\"");
    strcat(patron, clave);
    strcat(patron, "\":\"");

    destino[0] = '\0';
    const char* inicio = strstr(linea, patron);
    if (inicio == nullptr) return;
    inicio += strlen(patron);        // saltar el patron, quedar al inicio del valor

    int i = 0;
    while (inicio[i] != '"' && inicio[i] != '\0' && i < tamDestino - 1) {
        destino[i] = inicio[i];
        i++;
    }
    destino[i] = '\0';
}

// Busca "clave":numero y regresa el numero, o -1 si la clave no esta.
// Regresa long long porque marca_tiempo no cabe en un int de forma segura.
long long leerNumero(const char* linea, const char* clave) {
    char patron[64];                 // queda como:  "clave":
    strcpy(patron, "\"");
    strcat(patron, clave);
    strcat(patron, "\":");

    const char* inicio = strstr(linea, patron);
    if (inicio == nullptr) return -1;
    return atoll(inicio + strlen(patron));
}

// Crea (con new) la transaccion descrita en una linea de tipo TRANSACCION
Transaccion* leerTransaccion(const char* linea) {
    char txId[64], user[64], evento[64], objeto[128], estado[32], comando[128];
    leerTexto(linea, "tx_id",   txId,    64);
    leerTexto(linea, "user",    user,    64);
    leerTexto(linea, "evento",  evento,  64);
    leerTexto(linea, "objeto",  objeto,  128);
    leerTexto(linea, "estado",  estado,  32);
    leerTexto(linea, "comando", comando, 128);
    long long marcaTiempo = leerNumero(linea, "marca_tiempo");
    int       privilegio  = leerNumero(linea, "privilegio");

    return new Transaccion(txId, marcaTiempo, user, evento, objeto, estado, comando, privilegio);
}

// Calcula los hashes del encabezado de un bloque.
// prevHash = blockHash del tip actual (o ceros si es el genesis).
// Orden obligatorio: 1o merkleRoot, 2o blockHash (blockHash incluye al merkleRoot).
void sellarBloque(Bloque& b, ListaEnlazada<Bloque>& cadena) {
    if (cadena.estaVacia())
        strcpy(b.encabezado.prevHash, "0000000000000000");
    else
        strcpy(b.encabezado.prevHash, cadena.obtenerEn(0).encabezado.blockHash);

    calcularMerkleRoot(b, b.encabezado.merkleRoot);   // 1o
    calcularBlockHash(b, b.encabezado.blockHash);     // 2o
}

// Termina el bloque que se estaba leyendo: calcula sus hashes, lo indexa y lo pone como nuevo tip.
// merkleLeido: merkle_root que traia el archivo ("" si no traia), para verificarlo.
void cerrarBloque(Bloque& b, int txDeclaradas, const char* merkleLeido,
                  ListaEnlazada<Bloque>& cadena, Indices& indices) {
    if (b.numTx() != txDeclaradas)
        std::cout << "AVISO: el bloque #" << b.encabezado.height << " dice tx_count=" << txDeclaradas
                  << " pero trae " << b.numTx() << " transacciones" << std::endl;

    sellarBloque(b, cadena);

    // Si el archivo ya traia merkle_root (guardado con guardarNDJSON), solo se verifica
    if (merkleLeido[0] != '\0' && strcmp(merkleLeido, b.encabezado.merkleRoot) != 0)
        std::cout << "ALERTA: el bloque #" << b.encabezado.height << " no coincide con el merkle_root del archivo ("
                  << merkleLeido << " != " << b.encabezado.merkleRoot << ")" << std::endl;

    indices.indexarBloque(b);
    cadena.agregarInicio(b);   // agregarInicio: el bloque mas reciente queda en la cabeza
}

// Carga todo el archivo en la cadena y construye los indices.
// Cada transaccion creada se guarda en `transacciones`, que es la
// duena de esa memoria (main hace los delete al final).
bool cargarNDJSON(const char* ruta, ListaEnlazada<Bloque>& cadena, Indices& indices,
                  DArray<Transaccion*>& transacciones) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        std::cout << "ERROR: no se pudo abrir " << ruta << std::endl;
        return false;
    }

    char linea[TAM_LINEA];
    char tipo[32];
    Bloque actual;             // bloque que se esta armando
    bool hayBloque = false;    // ya se leyo al menos un BLOQUE?
    int  txDeclaradas = 0;     // tx_count que dice el archivo para el bloque actual
    char merkleLeido[TAM_HASH] = "";   // merkle_root que dice el archivo (si lo trae)

    while (archivo.getline(linea, TAM_LINEA)) {
        int largo = strlen(linea);
        if (largo > 0 && linea[largo - 1] == '\r')   // archivo guardado en Windows
            linea[largo - 1] = '\0';
        if (linea[0] == '\0') continue;               // linea vacia

        leerTexto(linea, "tipo", tipo, 32);

        if (strcmp(tipo, "BLOQUE") == 0) {
            if (hayBloque)
                cerrarBloque(actual, txDeclaradas, merkleLeido, cadena, indices);

            char prevHash[65], blockHash[65];
            leerTexto(linea, "prev_hash",  prevHash,  65);
            leerTexto(linea, "block_hash", blockHash, 65);
            leerTexto(linea, "merkle_root", merkleLeido, TAM_HASH);
            int altura   = leerNumero(linea, "block_index");
            txDeclaradas = leerNumero(linea, "tx_count");

            // txCount empieza en 0 y agregarTx() lo va subiendo
            actual = Bloque(EncabezadoBloque(altura, 0, prevHash, blockHash));
            hayBloque = true;
        }
        else if (strcmp(tipo, "TRANSACCION") == 0) {
            if (!hayBloque) {
                std::cout << "AVISO: transaccion antes de cualquier bloque, se ignora" << std::endl;
                continue;
            }
            Transaccion* tx = leerTransaccion(linea);
            transacciones.agregar(tx);
            actual.agregarTx(tx);
        }
    }

    // el ultimo bloque no tiene otro BLOQUE despues que lo cierre
    if (hayBloque)
        cerrarBloque(actual, txDeclaradas, merkleLeido, cadena, indices);
    return true;
}


// ── Entrega 4: guardar la cadena con el encabezado completo ──

// Escribe una transaccion como linea NDJSON
void escribirTransaccion(std::ofstream& salida, const Transaccion* tx) {
    salida << "{\"tipo\":\"TRANSACCION\""
           << ",\"tx_id\":\""    << tx->txId_    << "\""
           << ",\"marca_tiempo\":" << tx->marcaTiempo_
           << ",\"user\":\""     << tx->user_    << "\""
           << ",\"evento\":\""   << tx->evento_  << "\""
           << ",\"objeto\":\""   << tx->objeto_  << "\""
           << ",\"estado\":\""   << tx->estado_  << "\""
           << ",\"comando\":\""  << tx->comando_ << "\""
           << ",\"privilegio\":"  << tx->privilegio_ << "}\n";
}

// Guarda toda la cadena (del genesis al tip) con prev_hash, merkle_root y block_hash.
// Al volver a cargar este archivo, cargarNDJSON verifica el merkle_root guardado.
bool guardarNDJSON(const char* ruta, ListaEnlazada<Bloque>& cadena) {
    std::ofstream salida(ruta);
    if (!salida.is_open()) {
        std::cout << "ERROR: no se pudo crear " << ruta << std::endl;
        return false;
    }

    // La lista va del tip al genesis; se juntan los punteros para escribirla al reves
    DArray<Bloque*> bloques;
    cadena.paraCada([&](Bloque& b) { bloques.agregar(&b); });

    for (int i = bloques.tamanoActual() - 1; i >= 0; i--) {
        const Bloque* b = bloques.obtenerEn(i);
        salida << "{\"tipo\":\"BLOQUE\""
               << ",\"block_index\":" << b->encabezado.height
               << ",\"tx_count\":"    << b->encabezado.txCount
               << ",\"prev_hash\":\""   << b->encabezado.prevHash   << "\""
               << ",\"merkle_root\":\"" << b->encabezado.merkleRoot << "\""
               << ",\"block_hash\":\""  << b->encabezado.blockHash  << "\"}\n";
        for (int j = 0; j < b->numTx(); j++)
            escribirTransaccion(salida, b->obtenerTx(j));
    }
    return true;
}
