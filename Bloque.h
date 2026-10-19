#pragma once
#include "DArray.h"
#include "Transaccion.h"
#include "EncabezadoBloque.h"
#include <iostream>


struct Bloque {

    EncabezadoBloque     encabezado;
    DArray<Transaccion*> txList;    // punteros, no copias


    // Constructor
    Bloque() {
  
    }

    explicit Bloque(const EncabezadoBloque& enc) : encabezado(enc) {}

 
    void agregarTx(Transaccion* tx) {
        txList.agregar(tx);
        encabezado.txCount = txList.tamanoActual();
    }


    bool esValido() const {
        return encabezado.txCount > 0;
    }


    void imprimir() const {
        std::cout << "BLOQUE #" << encabezado.height
                  << "  |  tx=" << encabezado.txCount
                  << "  |  prev=" << encabezado.prevHash
                  << "  |  hash=" << encabezado.blockHash << std::endl;
        for (int i = 0; i < txList.tamanoActual(); i++) {
            Transaccion* tx = txList.obtenerEn(i);
            std::cout << "    TX [" << tx->id_ << "] " << tx->evento_
                      << " | " << tx->fecha_ << " " << tx->hora_
                      << " | user=" << tx->user_
                      << " | objeto=" << tx->objeto_
                      << " | estado=" << tx->estado_
                      << " | cmd=\"" << tx->comando_ << "\""
                      << " | priv=" << tx->privilegio_ << std::endl;
        }
    }
};
