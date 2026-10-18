#pragma once
#include "DArray.h"
#include "Transaccion.h"
#include "EncabezadoBloque.h"
#include <iostream>

// ============================================================
// Bloque.h
// Proyecto Integrador Mini-Blockchain — TC1031
// ============================================================
// Un bloque agrupa un conjunto de transacciones confirmadas
// junto con su encabezado de metadatos.
//
// IMPORTANTE — propiedad de las transacciones:
//   Bloque NO es propietario de las Transaccion*.
//   El bloque almacena punteros a transacciones que fueron
//   creadas fuera de él (por ejemplo, extraídas de la Mempool).
//   Quien creó las transacciones es responsable de liberarlas.
//   Bloque NUNCA llama delete sobre sus Transaccion*.
// ============================================================

struct Bloque {

    EncabezadoBloque     encabezado;
    DArray<Transaccion*> txList;    // punteros, no copias

    // --------------------------------------------------------
    // Constructor por defecto
    // --------------------------------------------------------
    Bloque() {
        // El DArray y el EncabezadoBloque se inicializan
        // con sus propios constructores por defecto.
        // No es necesario hacer nada aquí.
    }

    // --------------------------------------------------------
    // Constructor con encabezado
    // Recibe un EncabezadoBloque ya construido.
    // --------------------------------------------------------
    explicit Bloque(const EncabezadoBloque& enc) : encabezado(enc) {}

    // --------------------------------------------------------
    // agregarTx
    // Agrega un puntero a transacción a la txList y actualiza
    // el contador txCount del encabezado.
    //
    // Parámetro:
    //   tx — puntero a la transacción a agregar (Bloque no la posee)
    // --------------------------------------------------------
    void agregarTx(Transaccion* tx) {
        txList.agregar(tx);
        encabezado.txCount = txList.tamanoActual();
    }

    // --------------------------------------------------------
    // esValido (validación simulada)
    // En esta entrega, un bloque se considera válido si tiene
    // al menos una transacción. La validación real con hashes
    // se implementará en la Entrega No. 3.
    //
    // Retorna: true si txCount > 0, false en caso contrario.
    // --------------------------------------------------------
    bool esValido() const {
        return encabezado.txCount > 0;
    }

    // --------------------------------------------------------
    // imprimir
    // Muestra en consola el encabezado del bloque y todas
    // sus transacciones, con el siguiente formato:
    //
    // BLOQUE #<height>  |  tx=<txCount>  |  prev=<prevHash>  |  hash=<blockHash>
    //     TX [<txId>] <operacion> | <campos de tu Transaccion>
    //     TX [<txId>] ...
    // --------------------------------------------------------
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
