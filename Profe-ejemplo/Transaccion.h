#pragma once
#include <iostream>
#include <cstring>

// ============================================================
// Transaccion.h
// Proyecto Integrador Mini-Blockchain — TC1031
// ============================================================
// Una transacción representa un evento atómico e inmutable:
// algo que ocurrió, quién lo hizo, cuándo, y sobre qué objeto.
//
// ADAPTACIÓN REQUERIDA:
//   Los campos marcados con [ADAPTAR] deben reemplazarse con
//   los campos propios de la aplicación de tu equipo, definidos
//   en la Entrega No. 1.
//
//   Los campos txId, operacion y marcaTiempo son obligatorios
//   en todos los dominios — no los elimines.
//
// REGLA DE LOS 3:
//   Como la estructura usa char* con memoria dinámica,
//   debes implementar:
//     - Constructor de copia
//     - Operador de asignación
//     - Destructor
//   Si alguno falta, el compilador generará versiones que hacen
//   copia superficial y producirán doble liberación (double free).
// ============================================================

struct Transaccion {

    // ── Campos obligatorios (no modificar) ──────────────────
    char*     txId;         // identificador único: tx_TIMESTAMP_NNN
    char*     operacion;    // tipo de evento (ej. CREATE, UPDATE, VOID)
    long long marcaTiempo;  // timestamp Unix del momento del evento

    // ── Campos del dominio propio [ADAPTAR] ─────────────────
    // Reemplaza estos campos con los de tu aplicación.
    // Usa char* para cadenas y tipos primitivos para valores numéricos.
    // Ejemplo para registro de calificaciones:
    //   char*  idProfesor;
    //   char*  idAlumno;
    //   char*  actividad;
    //   double calificacion;
    //
    // TODO: declarar aquí los campos de tu dominio
    // char*  campo1;
    // char*  campo2;
    // double valorNumerico;


    // --------------------------------------------------------
    // Constructor por defecto
    // Inicializa TODOS los char* con cadenas vacías válidas.
    // NUNCA dejar un char* en nullptr — causa errores al imprimir
    // o al copiar sin verificar.
    // --------------------------------------------------------
    Transaccion() : marcaTiempo(0) {
        txId      = new char[1]; txId[0]      = '\0';
        operacion = new char[1]; operacion[0] = '\0';

        // TODO: inicializar los char* de tu dominio igual que arriba
        // campo1 = new char[1]; campo1[0] = '\0';
    }

    // --------------------------------------------------------
    // Constructor completo
    // Recibe todos los campos por valor y los copia con strcpy.
    //
    // TODO: agregar los parámetros de tu dominio y copiarlos.
    // --------------------------------------------------------
    Transaccion(const char* id, const char* op, long long tiempo
                /* TODO: agregar parámetros de tu dominio */)
        : marcaTiempo(tiempo)
    {
        txId      = new char[strlen(id) + 1]; strcpy(txId,      id);
        operacion = new char[strlen(op) + 1]; strcpy(operacion, op);

        // TODO: copiar los campos de tu dominio igual que arriba
        // campo1 = new char[strlen(c1) + 1]; strcpy(campo1, c1);
    }

    // --------------------------------------------------------
    // Regla de los 3 — constructor de copia (copia profunda)
    // Se llama cuando se pasa una Transaccion por valor o se
    // inicializa una con otra: Transaccion b = a;
    // --------------------------------------------------------
    Transaccion(const Transaccion& otra) : marcaTiempo(otra.marcaTiempo) {
        txId      = new char[strlen(otra.txId)      + 1]; strcpy(txId,      otra.txId);
        operacion = new char[strlen(otra.operacion) + 1]; strcpy(operacion, otra.operacion);

        // TODO: copiar los char* de tu dominio igual que arriba
    }

    // --------------------------------------------------------
    // Regla de los 3 — operador de asignación
    // Se llama cuando se asigna una Transaccion existente a otra:
    // b = a;  (cuando b ya fue construida)
    //
    // IMPORTANTE: verificar autoasignación (if this == &otra)
    // y liberar la memoria existente antes de copiar.
    // --------------------------------------------------------
    Transaccion& operator=(const Transaccion& otra) {
        if (this == &otra) return *this;

        // Liberar memoria existente
        delete[] txId;
        delete[] operacion;
        // TODO: delete[] de los char* de tu dominio

        // Copiar campos primitivos
        marcaTiempo = otra.marcaTiempo;

        // Copiar char* con copia profunda
        txId      = new char[strlen(otra.txId)      + 1]; strcpy(txId,      otra.txId);
        operacion = new char[strlen(otra.operacion) + 1]; strcpy(operacion, otra.operacion);

        // TODO: copiar los char* de tu dominio igual que arriba

        return *this;
    }

    // --------------------------------------------------------
    // Regla de los 3 — destructor
    // Libera TODA la memoria reservada con new[].
    // Si algún char* no se libera aquí, habrá memory leak.
    // --------------------------------------------------------
    ~Transaccion() {
        delete[] txId;
        delete[] operacion;
        // TODO: delete[] de los char* de tu dominio
    }

    // --------------------------------------------------------
    // imprimir
    // Muestra en consola todos los campos de la transacción.
    // Adaptar el formato a los campos de tu dominio.
    // --------------------------------------------------------
    void imprimir() const {
        std::cout << "    TX [" << txId << "] " << operacion
                  << " | t=" << marcaTiempo;
        // TODO: agregar los campos de tu dominio
        // << " | campo1=" << campo1
        // << " | valor=" << valorNumerico
        std::cout << std::endl;
    }
};
