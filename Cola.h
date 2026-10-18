#pragma once
#include "DArray.h"

// ============================================================
// Cola.h
// Proyecto Integrador Mini-Blockchain — TC1031
// ============================================================
// Cola circular genérica de capacidad fija, construida por
// composición sobre DArray<T>. La capacidad se define en el
// constructor y NO crece.
//
// La lógica circular la maneja Cola: los índices frente y final
// avanzan con aritmética modular (i + 1) % capacidad, y se usa
// obtenerEn()/asignarEn() de DArray para leer y escribir.
//
// Uso en el sistema: Mempool = Cola<Transaccion*>.
// La Cola NO es propietaria de los punteros que guarda.
// ============================================================

template <typename T>
class Cola {
public:
    // --------------------------------------------------------
    // Constructor: reserva un bloque fijo de 'cap' posiciones.
    // fijarTamano(cap) marca todas las posiciones como usables
    // en el DArray; el conteo real de elementos lo lleva Cola.
    // --------------------------------------------------------
    explicit Cola(int cap) : datos(cap < 1 ? 1 : cap), frente(0), final(0), cuenta(0) {
        capacidad = (cap < 1) ? 1 : cap;
        datos.fijarTamano(capacidad);
    }

    // Inserta al final, O(1). Si está llena NO inserta y regresa false.
    bool encolar(const T& elemento) {
        if (estaLlena()) return false;
        datos.asignarEn(final, elemento);
        final = (final + 1) % capacidad;
        cuenta++;
        return true;
    }

    // Elimina el frente, O(1). Regresa false si estaba vacía.
    bool desencolar() {
        if (estaVacia()) return false;
        frente = (frente + 1) % capacidad;
        cuenta--;
        return true;
    }

    // Devuelve el elemento al frente sin eliminarlo.
    // Si la cola está vacía regresa T() (nullptr para punteros).
    T frenteActual() const {
        if (estaVacia()) return T();
        return datos.obtenerEn(frente);
    }

    bool estaVacia() const { return cuenta == 0; }
    bool estaLlena() const { return cuenta == capacidad; }
    int tamanoActual() const { return cuenta; }
    int capacidadActual() const { return capacidad; }

private:
    DArray<T> datos;   // bloque de memoria fijo
    int frente;        // índice del primer elemento
    int final;         // índice donde se insertará el siguiente
    int cuenta;        // elementos actualmente en la cola
    int capacidad;     // capacidad fija
};
