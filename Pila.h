#pragma once

#include <iostream>
#include "DArray.h"

template <class T>
class Pila
{
private:
    DArray<T> datos;

public:
    void apilar(T elemento);
    void desapilar();
    T tope();
    bool estaVacia();
    int tamanoActual();
};

template <class T>
inline void Pila<T>::apilar(T elemento)
{
    datos.agregar(elemento);
}

template <class T>
inline void Pila<T>::desapilar()
{
    if (!datos.estaVacio())
        datos.fijarTamano(datos.tamanoActual() - 1);
}

template <class T>
inline T Pila<T>::tope()
{
    if (datos.estaVacio())
    {
        std::cout << "Pila vacia" << std::endl;
        return T();
    }
    return datos.obtenerEn(datos.tamanoActual() - 1);
}

template <class T>
inline bool Pila<T>::estaVacia()
{
    return datos.estaVacio();
}

template <class T>
inline int Pila<T>::tamanoActual()
{
    return datos.tamanoActual();
}