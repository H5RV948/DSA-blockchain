#pragma once
#include <iostream>
#include "DArray.h"
#include "Transaccion.h"
#include "EncabezadoBloque.h"

// ============================================================
// Bloque.h
// Un bloque = encabezado (altura, hashes) + lista de transacciones.
//
// El bloque guarda PUNTEROS a transacciones y NO es dueno de ellas:
// nunca hace delete. Las libera quien las creo (ver main.cpp).
// ============================================================

struct Bloque {
    EncabezadoBloque     encabezado;
    DArray<Transaccion*> txList;

    Bloque() {}
    explicit Bloque(const EncabezadoBloque& enc) : encabezado(enc) {}

    // Agrega una transaccion y mantiene txCount sincronizado
    void agregarTx(Transaccion* tx) {
        txList.agregar(tx);
        encabezado.txCount = txList.tamanoActual();
    }

    int numTx() const { return txList.tamanoActual(); }

    // i-esima transaccion del bloque (0 <= i < numTx())
    Transaccion* obtenerTx(int i) const { return txList.obtenerEn(i); }

    // Validacion simulada de la Entrega 2: valido si tiene transacciones
    bool esValido() const { return encabezado.txCount > 0; }

    void imprimir() const {
        std::cout << "BLOQUE #" << encabezado.height
                  << "  |  tx=" << encabezado.txCount
                  << "  |  prev=" << encabezado.prevHash
                  << "  |  hash=" << encabezado.blockHash << std::endl;
        for (int i = 0; i < numTx(); i++) {
            std::cout << "    ";
            obtenerTx(i)->imprimir();
        }
    }
};
