#pragma once
#include "DArray.h"

template <typename T>
class Cola {
public:
    explicit Cola(int cap) : datos(cap < 1 ? 1 : cap), frente(0), final(0), cuenta(0) {
        capacidad = (cap < 1) ? 1 : cap;
        datos.fijarTamano(capacidad);
    }

    bool encolar(const T& elemento) {
        if (estaLlena()) return false;
        datos.asignarEn(final, elemento);
        final = (final + 1) % capacidad;
        cuenta++;
        return true;
    }

    bool desencolar() {
        if (estaVacia()) return false;
        frente = (frente + 1) % capacidad;
        cuenta--;
        return true;
    }

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
