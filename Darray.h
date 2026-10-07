#pragma once
 
#include <iostream>
using namespace std;

// Arreglo dinamico generico, implementado desde cero.
// No utiliza std::vector ni ninguna clase de la biblioteca estandar.
template <typename T>
class DArray {
private:
    T* datos;
    int capacidad;
    int tamano;
 
    // Duplica (o ajusta a) la capacidad interna y copia los elementos existentes.
    void redimensionar(int nuevaCapacidad) {
        T* nuevoDatos = new T[nuevaCapacidad];
        for (int i = 0; i < tamano; i++) {
            nuevoDatos[i] = datos[i];
        }
        delete[] datos;
        datos = nuevoDatos;
        capacidad = nuevaCapacidad;
    }
 
public:
    // Constructor: capacidad inicial por defecto = 4
    DArray(int capacidadInicial = 4) {
        capacidad = capacidadInicial > 0 ? capacidadInicial : 4;
        tamano = 0;
        datos = new T[capacidad];
    }
 
    // Constructor de copia (regla de los 3: administramos memoria cruda)
    DArray(const DArray<T>& otro) {
        capacidad = otro.capacidad;
        tamano = otro.tamano;
        datos = new T[capacidad];
        for (int i = 0; i < tamano; i++) {
            datos[i] = otro.datos[i];
        }
    }
 
    // Operador de asignacion
    DArray<T>& operator=(const DArray<T>& otro) {
        if (this == &otro) return *this;
 
        delete[] datos;
        capacidad = otro.capacidad;
        tamano = otro.tamano;
        datos = new T[capacidad];
        for (int i = 0; i < tamano; i++) {
            datos[i] = otro.datos[i];
        }
        return *this;
    }
 
    // Destructor
    ~DArray() {
        delete[] datos;
    }
 
    // Agrega un elemento al final, redimensionando si es necesario.
    void agregar(const T& elemento) {
        if (tamano == capacidad) {
            redimensionar(capacidad * 2);
        }
        datos[tamano] = elemento;
        tamano++;
    }
 
    // Devuelve el elemento en la posicion indice (sin verificacion de limites,
    // se asume indice valido; los equipos pueden agregar manejo de errores).
    T& obtener(int indice) {
        return datos[indice];
    }
 
    const T& obtener(int indice) const {
        return datos[indice];
    }
 
    // Elimina el elemento en la posicion indice, desplazando los siguientes.
    void eliminarEn(int indice) {
        if (indice < 0 || indice >= tamano) return;
        for (int i = indice; i < tamano - 1; i++) {
            datos[i] = datos[i + 1];
        }
        tamano--;
    }
    
    
    // CONCATENAR
    void concatenar(const DArray<T>& otro) {
        for (int i = 0; i < otro.tamanoActual(); i++) {
            this->agregar(otro.obtener(i));
        }
    }
    
    // SUMAR
    DArray* sumar(const DArray<T>& otro1 , const DArray<T>& otro2) {
        
        if (otro1.tamanoActual() != otro2.tamanoActual()) return nullptr;
        
        
        DArray<T>* res = new DArray<T>();
          
        for (int i = 0; i < otro1.tamanoActual(); i++) {
            T suma = otro1.obtener(i) + otro2.obtener(i);
            res->agregar(suma);
        }
        
        return res;
    }
 
    int tamanoActual() const {
        return tamano;
    }

    bool estaVacio() const {
        return tamano == 0;
    }
 
    int capacidadActual() const {
        return capacidad;
    }
    
    // imprimir DArray
    void printDArray(unsigned int n){
        cout << endl;
        for (unsigned int i = 0; i < n; i++){
            cout << this->obtener(i) << " ";
        }
        cout << endl;
}
    
};
