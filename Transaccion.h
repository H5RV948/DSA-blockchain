#pragma once
#include <iostream>
#include <cstring>
#include <ctime>     // gmtime y strftime: para convertir marcaTiempo a texto

// ============================================================
// Transaccion.h
// Una transaccion = un evento de acceso privilegiado que ya ocurrio.
//   Ej: "admin ejecuto 'useradd jperez' (CREAR_USUARIO) sobre
//        usuario:jperez el 2026-10-01 08:03:45 UTC -> OTORGADO"
//
// txId = NODxxx-NNNNN-marcaTiempo
//   NODxxx      nodo de auditoria que registro el evento
//   NNNNN       contador propio de ese nodo
//   marcaTiempo momento del evento (timestamp Unix)
// Cada nodo numera sus transacciones sin depender de los demas,
// y como el nodo va en el ID, dos nodos no pueden generar el mismo.
//
// Los textos son char* con memoria dinamica, por eso implementa
// la regla de los 3 (copia, asignacion, destructor).
// ============================================================

struct Transaccion {
    char* txId_;      // identificador unico, ej. "NOD002-00007-1790851350" (clave de las TablaHash)
    long long marcaTiempo_;   // segundos desde 1970-01-01 00:00:00 UTC (timestamp Unix)
    char* user_;      // quien hizo la accion
    char* evento_;    // que paso: LOGIN, CREAR_USUARIO, CAMBIAR_PERMISOS...
    char* objeto_;    // sobre que se hizo: archivo, usuario o "sistema"
    char* estado_;    // OTORGADO o DENEGADO
    char* comando_;   // comando que se ejecuto
    int   privilegio_;   // 0 = usuario normal, 2 = administrador

    // Constructor por defecto: cadenas vacias validas (nunca nullptr)
    Transaccion() : marcaTiempo_(0), privilegio_(-1) {
        txId_    = copiarTexto("");
        user_    = copiarTexto("");
        evento_  = copiarTexto("");
        objeto_  = copiarTexto("");
        estado_  = copiarTexto("");
        comando_ = copiarTexto("");
    }

    // Constructor completo
    Transaccion(const char* txId, long long marcaTiempo, const char* user,
                const char* evento, const char* objeto, const char* estado,
                const char* comando, int privilegio)
        : marcaTiempo_(marcaTiempo), privilegio_(privilegio) {
        txId_    = copiarTexto(txId);
        user_    = copiarTexto(user);
        evento_  = copiarTexto(evento);
        objeto_  = copiarTexto(objeto);
        estado_  = copiarTexto(estado);
        comando_ = copiarTexto(comando);
    }

    // ── Regla de los 3 ──────────────────────────────────────

    // Constructor de copia: cada char* se copia a memoria nueva (copia profunda)
    Transaccion(const Transaccion& otra)
        : marcaTiempo_(otra.marcaTiempo_), privilegio_(otra.privilegio_) {
        copiarCampos(otra);
    }

    // Operador de asignacion: liberar lo que habia y copiar lo nuevo
    Transaccion& operator=(const Transaccion& otra) {
        if (this == &otra) return *this;   // a = a
        liberar();
        copiarCampos(otra);
        marcaTiempo_ = otra.marcaTiempo_;
        privilegio_  = otra.privilegio_;
        return *this;
    }

    // Destructor
    ~Transaccion() {
        liberar();
    }

    // Imprime la transaccion en una sola linea
    void imprimir() const {
        char fechaHora[32];
        formatearTiempo(marcaTiempo_, fechaHora, 32);
        std::cout << "TX [" << txId_ << "] " << evento_
                  << " | " << fechaHora
                  << " | user=" << user_
                  << " | objeto=" << objeto_
                  << " | estado=" << estado_
                  << " | cmd=\"" << comando_ << "\""
                  << " | priv=" << privilegio_ << std::endl;
    }

    // Convierte un timestamp Unix a texto "AAAA-MM-DD hh:mm:ss UTC"
    static void formatearTiempo(long long marcaTiempo, char* destino, int tamDestino) {
        time_t segundos = (time_t)marcaTiempo;
        tm* partes = gmtime(&segundos);      // separa en anio, mes, dia, hora... (en UTC)
        strftime(destino, tamDestino, "%Y-%m-%d %H:%M:%S UTC", partes);
    }

private:
    // Reserva memoria nueva y copia el texto. Quien la recibe la debe liberar.
    static char* copiarTexto(const char* texto) {
        char* copia = new char[strlen(texto) + 1];
        strcpy(copia, texto);
        return copia;
    }

    void copiarCampos(const Transaccion& otra) {
        txId_    = copiarTexto(otra.txId_);
        user_    = copiarTexto(otra.user_);
        evento_  = copiarTexto(otra.evento_);
        objeto_  = copiarTexto(otra.objeto_);
        estado_  = copiarTexto(otra.estado_);
        comando_ = copiarTexto(otra.comando_);
    }

    void liberar() {
        delete[] txId_;
        delete[] user_;
        delete[] evento_;
        delete[] objeto_;
        delete[] estado_;
        delete[] comando_;
    }
};
