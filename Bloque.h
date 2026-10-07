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
    explicit Bloque(const EncabezadoBloque& enc) {
        // TODO: implementar
        // Sugerencia: asignar enc al campo encabezado
    }

    // --------------------------------------------------------
    // agregarTx
    // Agrega un puntero a transacción a la txList y actualiza
    // el contador txCount del encabezado.
    //
    // Parámetro:
    //   tx — puntero a la transacción a agregar (Bloque no la posee)
    // --------------------------------------------------------
    void agregarTx(Transaccion* tx) {
        // TODO: implementar
        // 1. Agregar tx al DArray txList
        // 2. Actualizar encabezado.txCount
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
        // TODO: implementar
        return false; // reemplazar con la condición correcta
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
        // TODO: implementar
        // Sugerencia: usar std::cout para el encabezado,
        // luego recorrer txList con un for y llamar tx->imprimir()
        // en cada elemento.
    }
};
