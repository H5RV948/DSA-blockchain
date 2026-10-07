#pragma once
#include <iostream>

// DArray<T> — arreglo dinámico genérico
// Módulo 1 del proyecto integrador Mini-Blockchain
// Versión actualizada con asignarEn() y fijarTamano() para Cola circular

template <typename T>
class DArray {
public:
    DArray() : datos(nullptr), tamano(0), capacidad(0) {}

    explicit DArray(int capInicial) : tamano(0), capacidad(capInicial) {
        datos = new T[capacidad];
    }

    DArray(const DArray& otro) : tamano(otro.tamano), capacidad(otro.capacidad) {
        datos = new T[capacidad];
        for (int i = 0; i < tamano; i++)
            datos[i] = otro.datos[i];
    }

    DArray& operator=(const DArray& otro) {
        if (this == &otro) return *this;
        delete[] datos;
        tamano    = otro.tamano;
        capacidad = otro.capacidad;
        datos = new T[capacidad];
        for (int i = 0; i < tamano; i++)
            datos[i] = otro.datos[i];
        return *this;
    }

    ~DArray() { delete[] datos; }

    // Agrega al final; redimensiona x2 si es necesario
    void agregar(const T& valor) {
        if (tamano == capacidad) redimensionar();
        datos[tamano++] = valor;
    }

    // Acceso por índice
    T& obtenerEn(int indice) { return datos[indice]; }
    const T& obtenerEn(int indice) const { return datos[indice]; }

    // Escritura directa en posición (para Cola circular)
    void asignarEn(int indice, const T& valor) { datos[indice] = valor; }

    // Fija el tamaño lógico sin agregar elementos (para Cola circular)
    void fijarTamano(int nuevoTamano) { tamano = nuevoTamano; }

    int tamanoActual() const { return tamano; }
    int capacidadActual() const { return capacidad; }
    bool estaVacio() const { return tamano == 0; }

private:
    T*  datos;
    int tamano;
    int capacidad;

    void redimensionar() {
        capacidad = (capacidad == 0) ? 1 : capacidad * 2;
        T* nuevos = new T[capacidad];
        for (int i = 0; i < tamano; i++)
            nuevos[i] = datos[i];
        delete[] datos;
        datos = nuevos;
    }
};
