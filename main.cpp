#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include "DArray.h"
#include "Transaccion.h"
#include "EncabezadoBloque.h"
#include "Bloque.h"
#include "ListaEnlazada.h"
#include "Pila.h"
#include "Cola.h"
using namespace std;

const char* ARCHIVO_NDJSON  = "transacciones.ndjson";
const int   TAM_LINEA       = 1024;
const int   CAPACIDAD_MEMPOOL = 5;
const int   ALTURA_FALLA    = 3;   

// Registro de todas las transacciones 
DArray<Transaccion*> todasTx;

void extraerCadena(const char* linea, const char* clave, char* destino, int tam) {
    char patron[64];
    strcpy(patron, "\"");
    strcat(patron, clave);
    strcat(patron, "\":\"");

    destino[0] = '\0';
    const char* inicio = strstr(linea, patron);
    if (inicio == nullptr) return;
    inicio += strlen(patron);

    int i = 0;
    while (inicio[i] != '"' && inicio[i] != '\0' && i < tam - 1) {
        destino[i] = inicio[i];
        i++;
    }
    destino[i] = '\0';
}

// Regresa el valor entero de "clave":numero (o -1 si no existe).
int extraerEntero(const char* linea, const char* clave) {
    char patron[64];
    strcpy(patron, "\"");
    strcat(patron, clave);
    strcat(patron, "\":");

    const char* inicio = strstr(linea, patron);
    if (inicio == nullptr) return -1;
    return atoi(inicio + strlen(patron));
}


Transaccion* crearTxDesdeLinea(const char* linea) {
    char fecha[32], hora[32], user[64], evento[64], objeto[128], estado[32], comando[128];
    extraerCadena(linea, "fecha",   fecha,   32);
    extraerCadena(linea, "hora",    hora,    32);
    extraerCadena(linea, "user",    user,    64);
    extraerCadena(linea, "evento",  evento,  64);
    extraerCadena(linea, "objeto",  objeto,  128);
    extraerCadena(linea, "estado",  estado,  32);
    extraerCadena(linea, "comando", comando, 128);
    int id         = extraerEntero(linea, "id");
    int privilegio = extraerEntero(linea, "privilegio");

    Transaccion* tx = new Transaccion(fecha, hora, user, evento, objeto, estado, comando, id, privilegio);
    todasTx.agregar(tx);
    return tx;
}

// Crea una transacción nueva y la registra
Transaccion* crearTx(const char* fecha, const char* hora, const char* user, const char* evento,
                     const char* objeto, const char* estado, const char* comando, int id, int privilegio) {
    Transaccion* tx = new Transaccion(fecha, hora, user, evento, objeto, estado, comando, id, privilegio);
    todasTx.agregar(tx);
    return tx;
}


bool cargarCadena(const char* ruta, ListaEnlazada<Bloque>& cadena) {
    ifstream archivo(ruta);
    if (!archivo.is_open()) {
        cout << "ERROR: no se pudo abrir " << ruta << endl;
        return false;
    }

    char linea[TAM_LINEA];
    char tipo[32], prevHash[65], blockHash[65];
    Bloque actual;
    bool hayBloque = false;
    int txEsperadas = 0;

    while (archivo.getline(linea, TAM_LINEA)) {
        int len = strlen(linea);
        if (len > 0 && linea[len - 1] == '\r') linea[len - 1] = '\0';
        if (linea[0] == '\0') continue;

        extraerCadena(linea, "tipo", tipo, 32);

        if (strcmp(tipo, "BLOQUE") == 0) {
            // Cerrar el bloque anterior antes de empezar uno nuevo
            if (hayBloque) {
                if (actual.encabezado.txCount != txEsperadas)
                    cout << "AVISO: bloque #" << actual.encabezado.height << " declara tx_count="
                         << txEsperadas << " pero tiene " << actual.encabezado.txCount << endl;
                cadena.agregarInicio(actual);
            }
            int altura = extraerEntero(linea, "block_index");
            txEsperadas = extraerEntero(linea, "tx_count");
            extraerCadena(linea, "prev_hash",  prevHash,  65);
            extraerCadena(linea, "block_hash", blockHash, 65);

            // txCount empieza en 0: agregarTx() lo va actualizando
            actual = Bloque(EncabezadoBloque(altura, 0, prevHash, blockHash));
            hayBloque = true;
        } else if (strcmp(tipo, "TRANSACCION") == 0) {
            if (!hayBloque) {
                cout << "AVISO: transaccion sin bloque, se ignora" << endl;
                continue;
            }
            actual.agregarTx(crearTxDesdeLinea(linea));
        }
    }

    if (hayBloque) {
        if (actual.encabezado.txCount != txEsperadas)
            cout << "AVISO: bloque #" << actual.encabezado.height << " declara tx_count="
                 << txEsperadas << " pero tiene " << actual.encabezado.txCount << endl;
        cadena.agregarInicio(actual);
    }
    return true;
}


void imprimirBloque(Bloque& b) {
    b.imprimir();
}

void imprimirTx(Transaccion*& tx) {
    cout << "    TX [" << tx->id_ << "] " << tx->evento_ << " | user=" << tx->user_
         << " | objeto=" << tx->objeto_ << " | estado=" << tx->estado_ << endl;
}

Pila<int> rutaValidacion;
bool fallaDetectada = false;
int  alturaFallida  = -1;

bool pasaValidacion(const Bloque& b) {
    return b.esValido() && b.encabezado.height != ALTURA_FALLA;
}

// Se llama con paraCada(): recorre desde el tip (cabeza) hacia el génesis
void validarBloque(Bloque& b) {
    if (fallaDetectada) return;   // ya se detuvo la validación

    rutaValidacion.apilar(b.encabezado.height);
    if (pasaValidacion(b)) {
        cout << "  Bloque #" << b.encabezado.height << " ... OK" << endl;
    } else {
        cout << "  Bloque #" << b.encabezado.height << " ... FALLA" << endl;
        fallaDetectada = true;
        alturaFallida = b.encabezado.height;
    }
}

void separador(const char* titulo) {
    cout << endl << "============================================================" << endl;
    cout << titulo << endl;
    cout << "============================================================" << endl;
}

int main() {
    ListaEnlazada<Bloque> cadena;
    
    separador("CARGA DEL NDJSON EN ListaEnlazada<Bloque>");
    if (!cargarCadena(ARCHIVO_NDJSON, cadena)) return 1;

    cout << "Bloques cargados: " << cadena.tamanoActual()
         << " | Transacciones cargadas: " << todasTx.tamanoActual() << endl;
    cout << "Recorrido con paraCada() (del tip al genesis):" << endl << endl;
    cadena.paraCada(imprimirBloque);

    separador("tx_list: DArray<Transaccion*> y ListaEnlazada<Transaccion*>");
    Bloque tip = cadena.obtenerEn(0);

    ListaEnlazada<Transaccion*> txLista;
    for (int i = 0; i < tip.txList.tamanoActual(); i++)
        txLista.agregarFinal(tip.txList.obtenerEn(i));  

    cout << "Bloque #" << tip.encabezado.height << " con DArray (acceso por indice, O(1)):" << endl;
    for (int i = 0; i < tip.txList.tamanoActual(); i++) {
        Transaccion* tx = tip.txList.obtenerEn(i);
        imprimirTx(tx);
    }
    cout << endl << "Mismo bloque con ListaEnlazada (recorrido con paraCada):" << endl;
    txLista.paraCada(imprimirTx);

    int medio = txLista.tamanoActual() / 2;
    cout << endl << "Acceso a la posicion " << medio << ":" << endl;
    cout << "  DArray.obtenerEn(" << medio << ")       -> TX [" << tip.txList.obtenerEn(medio)->id_
         << "]  (salto directo, O(1))" << endl;
    cout << "  ListaEnlazada.obtenerEn(" << medio << ") -> TX [" << txLista.obtenerEn(medio)->id_
         << "]  (recorre " << medio << " nodos, O(n))" << endl;

    separador("MEMPOOL: Cola<Transaccion*> circular");
    Cola<Transaccion*> mempool(CAPACIDAD_MEMPOOL);
    cout << "Mempool creada con capacidad fija = " << mempool.capacidadActual() << endl << endl;

    // Crear transacciones de prueba 
    Transaccion* pendientes[7];
    pendientes[0] = crearTx("2026-10-02", "10:00:05", "jperez",  "LOGIN",          "sistema",                 "OTORGADO", "login jperez",            43, 0);
    pendientes[1] = crearTx("2026-10-02", "10:02:17", "jperez",  "LEER_ARCHIVO",   "/home/jperez/notas.txt",  "OTORGADO", "cat notas.txt",           44, 0);
    pendientes[2] = crearTx("2026-10-02", "10:05:40", "mlopez",  "ELIMINAR_ARCHIVO","/finanzas/balance.xlsx",  "DENEGADO", "rm balance.xlsx",         45, 0);
    pendientes[3] = crearTx("2026-10-02", "10:06:02", "admin",   "BLOQUEAR_USUARIO","usuario:mlopez",          "OTORGADO", "usermod -L mlopez",       46, 2);
    pendientes[4] = crearTx("2026-10-02", "10:09:33", "rgarcia", "LOGIN",          "sistema",                 "DENEGADO", "login rgarcia",           47, 0);
    pendientes[5] = crearTx("2026-10-02", "10:10:01", "rgarcia", "LOGIN",          "sistema",                 "OTORGADO", "login rgarcia",           48, 0);
    pendientes[6] = crearTx("2026-10-02", "10:12:45", "admin",   "LOGOUT",         "sistema",                 "OTORGADO", "logout admin",            49, 2);

    // Encolar 5 transacciones
    cout << "Encolando transacciones:" << endl;
    for (int i = 0; i < 5; i++) {
        bool ok = mempool.encolar(pendientes[i]);
        cout << "  encolar TX [" << pendientes[i]->id_ << "] " << pendientes[i]->evento_
             << (ok ? "  -> aceptada" : "  -> RECHAZADA") << "  (" << mempool.tamanoActual()
             << "/" << mempool.capacidadActual() << ")" << endl;
    }

    // Encolar con la Mempool llena
    cout << endl << "Mempool llena? " << (mempool.estaLlena() ? "SI" : "NO") << endl;
    bool aceptada = mempool.encolar(pendientes[5]);
    cout << "  encolar TX [" << pendientes[5]->id_ << "] " << pendientes[5]->evento_
         << (aceptada ? "  -> aceptada" : "  -> RECHAZADA (mempool llena)") << endl;

    // Formar el bloque candidato
    Bloque tipActual = cadena.obtenerEn(0);
    Bloque candidato(EncabezadoBloque(tipActual.encabezado.height + 1, 0,
                                      tipActual.encabezado.blockHash, "0000000000000000"));
    cout << endl << "El lider extrae 3 transacciones (FIFO) para el bloque candidato #"
         << candidato.encabezado.height << ":" << endl;
    for (int i = 0; i < 3; i++) {
        Transaccion* tx = mempool.frenteActual();
        mempool.desencolar();
        candidato.agregarTx(tx);
        cout << "  desencolar -> TX [" << tx->id_ << "] " << tx->evento_ << endl;
    }
    cout << "Quedan en la mempool: " << mempool.tamanoActual() << endl;

    //
    cout << endl << "Segunda vuelta de la cola circular (reutiliza posiciones liberadas):" << endl;
    for (int i = 5; i < 7; i++) {
        bool ok = mempool.encolar(pendientes[i]);
        cout << "  encolar TX [" << pendientes[i]->id_ << "] " << pendientes[i]->evento_
             << (ok ? "  -> aceptada" : "  -> RECHAZADA") << "  (" << mempool.tamanoActual()
             << "/" << mempool.capacidadActual() << ")" << endl;
    }
    cout << "Orden de salida (debe ser 46, 47, 48, 49):" << endl;
    while (!mempool.estaVacia()) {
        Transaccion* tx = mempool.frenteActual();
        mempool.desencolar();
        candidato.agregarTx(tx);
        cout << "  desencolar -> TX [" << tx->id_ << "] " << tx->evento_ << endl;
    }

    // El bloque candidato se confirma y se vuelve el nuevo tip
    cout << endl << "Bloque candidato confirmado y agregado con agregarInicio():" << endl;
    candidato.imprimir();
    cadena.agregarInicio(candidato);
    cout << "Bloques en la cadena: " << cadena.tamanoActual()
         << " | Nuevo tip: #" << cadena.obtenerEn(0).encabezado.height << endl;

    separador("ROLLBACK: validacion hacia atras con Pila<int>");
    cout << "Falla simulada en el bloque #" << ALTURA_FALLA << endl;
    cout << "Recorriendo del tip al genesis:" << endl;
    cadena.paraCada(validarBloque);

    if (fallaDetectada) {
        cout << endl << "Falla detectada en el bloque #" << alturaFallida << endl;
        cout << "Ruta recorrida (desapilando, de la falla hacia el tip):" << endl << "  ";
        bool primero = true;
        while (!rutaValidacion.estaVacia()) {
            if (!primero) cout << " -> ";
            cout << "#" << rutaValidacion.tope();
            rutaValidacion.desapilar();
            primero = false;
        }
        cout << endl;
    } else {
        cout << endl << "Toda la cadena es valida." << endl;
    }

    separador("LIBERANDO MEMORIA");
    for (int i = 0; i < todasTx.tamanoActual(); i++)
        delete todasTx.obtenerEn(i);
    cout << "Transacciones liberadas: " << todasTx.tamanoActual() << endl;

    return 0;
}
