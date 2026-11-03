#pragma once
#include "TablaHash.h"
#include "Bloque.h"

// ============================================================
// Indices.h  (Entrega 3)
// Dos tablas hash encima de la cadena de bloques:
//
//   txExiste : txId -> true      "ya vimos este txId?"       (duplicados)
//   txLedger : txId -> altura    "en que bloque quedo?"      (consulta)
//
// Las tablas NO guardan la transaccion, solo dicen si existe y
// donde esta. La cadena de bloques sigue siendo la fuente de verdad.
// ============================================================

struct Indices {
    TablaHash<bool> txExiste;
    TablaHash<int>  txLedger;

    // true si el txId ya existe (confirmado o pendiente en la mempool). O(1) promedio
    bool esDuplicada(const char* txId) {
        bool* existe = txExiste.buscar(txId);   // nullptr = nunca se ha visto
        return existe != nullptr;
    }

    // Altura del bloque donde esta el txId, o -1 si no esta en ningun bloque. O(1) promedio
    int consultarBloque(const char* txId) {
        int* altura = txLedger.buscar(txId);
        if (altura == nullptr) return -1;
        return *altura;
    }

    // La transaccion entro a la mempool: ya existe, pero todavia no tiene bloque
    void marcarPendiente(const char* txId) {
        txExiste.insertar(txId, true);
    }

    // Mete todas las transacciones de un bloque confirmado en los dos indices
    void indexarBloque(const Bloque& b) {
        for (int i = 0; i < b.numTx(); i++) {
            const char* txId = b.obtenerTx(i)->txId_;
            txExiste.insertar(txId, true);
            txLedger.insertar(txId, b.encabezado.height);
        }
    }
};
