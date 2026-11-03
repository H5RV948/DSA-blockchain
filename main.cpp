#include <iostream>
#include <cstring>
#include <ctime>     // time(): hora actual para transacciones nuevas
#include "DArray.h"
#include "Transaccion.h"
#include "Bloque.h"
#include "ListaEnlazada.h"
#include "Cola.h"
#include "Indices.h"
#include "CargadorNDJSON.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>   // SetConsoleOutputCP: para que "—" y los acentos se vean bien en la consola
#endif
using namespace std;

// ============================================================
// main.cpp — Entrega 3
// Demuestra los 4 flujos de la Parte 2:
//   2.1 Arranque: carga del NDJSON + construccion de indices
//   2.2 Intento de duplicado
//   2.3 Consulta al ledger por txId
//   2.4 Flujo completo: Mempool -> bloque -> indice
//
// Quien es dueno de la memoria:
//   `transacciones` guarda TODAS las Transaccion* creadas con new
//   y es la unica que hace delete (al final de main).
//   La cadena, la mempool y los indices solo guardan punteros o txIds.
// ============================================================

const char* ARCHIVO_NDJSON    = "transacciones.ndjson";
const int   CAPACIDAD_MEMPOOL = 5;
const char* NODO_LOCAL        = "NOD001";   // nodo de auditoria que corre este programa


// ── Funciones de apoyo ─────────────────────────────────────

void titulo(const char* texto) {
    cout << endl << "============================================================" << endl;
    cout << texto << endl;
    cout << "============================================================" << endl;
}

// Agrega `numero` al final de destino, con ceros a la izquierda hasta
// tener al menos `digitos` cifras. Ej: numero=15, digitos=5 -> "00015"
void agregarNumero(char* destino, long long numero, int digitos) {
    char cifras[24];
    int n = 0;
    do {
        cifras[n++] = '0' + (numero % 10);   // salen de derecha a izquierda
        numero /= 10;
    } while (numero > 0 || n < digitos);

    int fin = strlen(destino);
    for (int i = 0; i < n; i++)               // se copian al reves para que queden en orden
        destino[fin + i] = cifras[n - 1 - i];
    destino[fin + n] = '\0';
}

// Arma un txId con el formato NODxxx-NNNNN-marcaTiempo
void generarTxId(char* destino, const char* nodo, int secuencia, long long marcaTiempo) {
    strcpy(destino, nodo);
    strcat(destino, "-");
    agregarNumero(destino, secuencia, 5);
    strcat(destino, "-");
    agregarNumero(destino, marcaTiempo, 1);
}

// Guarda la transaccion en el registro de memoria y la regresa
Transaccion* registrar(DArray<Transaccion*>& transacciones, Transaccion* tx) {
    transacciones.agregar(tx);
    return tx;
}

// Recorre la cadena buscando el bloque con esa altura. O(n)
// Regresa un puntero al bloque dentro de la lista, o nullptr si no esta.
Bloque* buscarBloque(ListaEnlazada<Bloque>& cadena, int altura) {
    Bloque* encontrado = nullptr;
    cadena.paraCada([&](Bloque& b) {
        if (b.encabezado.height == altura)
            encontrado = &b;
    });
    return encontrado;
}

// Bloque vacio que va encima del tip: altura del tip + 1 y prev = hash del tip
Bloque crearBloqueCandidato(ListaEnlazada<Bloque>& cadena) {
    Bloque tip = cadena.obtenerEn(0);   // la cabeza de la lista es el bloque mas reciente
    return Bloque(EncabezadoBloque(tip.encabezado.height + 1, 0,
                                   tip.encabezado.blockHash, "0000000000000000"));
}

// Intenta meter una transaccion a la mempool.
// Se rechaza si su txId ya existe (confirmado o pendiente) o si la mempool esta llena.
bool enviarAMempool(Cola<Transaccion*>& mempool, Indices& indices, Transaccion* tx) {
    if (indices.esDuplicada(tx->txId_)) {
        cout << "  [ERROR] txId duplicado: " << tx->txId_ << " — transacción rechazada" << endl;
        return false;
    }
    if (!mempool.encolar(tx)) {
        cout << "  [ERROR] mempool llena: " << tx->txId_ << " no se encolo" << endl;
        return false;
    }
    // Decision del equipo: desde que entra a la mempool ya cuenta como existente,
    // asi un reenvio se rechaza aunque todavia no este en un bloque.
    indices.marcarPendiente(tx->txId_);
    return true;
}

// El bloque ya trae sus transacciones: se indexa y se vuelve el nuevo tip
void confirmarBloque(ListaEnlazada<Bloque>& cadena, Indices& indices, Bloque& b) {
    indices.indexarBloque(b);
    cadena.agregarInicio(b);
}

// Consulta el ledger e imprime el bloque donde esta el txId
void mostrarConsulta(ListaEnlazada<Bloque>& cadena, Indices& indices, const char* txId) {
    int altura = indices.consultarBloque(txId);
    cout << "consultarBloque(\"" << txId << "\") -> " << altura << endl;
    if (altura == -1) {
        cout << "  txId " << txId << " no encontrado en el ledger" << endl;
        return;
    }
    Bloque* b = buscarBloque(cadena, altura);
    if (b != nullptr) b->imprimir();
}


// ── 2.1 Arranque ───────────────────────────────────────────

bool demoCarga(ListaEnlazada<Bloque>& cadena, Indices& indices, DArray<Transaccion*>& transacciones) {
    titulo("2.1 ARRANQUE: carga del NDJSON + construccion de indices");
    if (!cargarNDJSON(ARCHIVO_NDJSON, cadena, indices, transacciones))
        return false;

    cout << "Bloques cargados:        " << cadena.tamanoActual() << endl;
    cout << "Transacciones cargadas:  " << transacciones.tamanoActual() << endl;
    cout << "Indexadas en txExiste:   " << indices.txExiste.tamano() << endl;
    cout << "Indexadas en txLedger:   " << indices.txLedger.tamano() << endl;
    cout << "Tablas: capacidad=" << indices.txLedger.capacidad()
         << ", factor de carga=" << indices.txLedger.factorCarga() << endl;

    cout << endl << "Cadena (del tip al genesis):" << endl;
    cadena.paraCada([](Bloque& b) {
        char primera[32], ultima[32];
        Transaccion::formatearTiempo(b.obtenerTx(0)->marcaTiempo_, primera, 32);
        Transaccion::formatearTiempo(b.obtenerTx(b.numTx() - 1)->marcaTiempo_, ultima, 32);
        cout << "  Bloque #" << b.encabezado.height << "  " << b.numTx() << " tx  "
             << primera << "  ->  " << ultima << endl;
    });
    return true;
}


// ── 2.2 Intento de duplicado ───────────────────────────────

void demoDuplicados(Cola<Transaccion*>& mempool, Indices& indices, DArray<Transaccion*>& transacciones) {
    titulo("2.2 INTENTO DE DUPLICADO: txExiste.buscar(txId)");

    // Caso 1: se reenvia una transaccion que ya viene en el NDJSON
    Transaccion* yaConfirmada = transacciones.obtenerEn(9);   // la decima que se cargo
    cout << "Caso 1: reenviar " << yaConfirmada->txId_ << " tal cual viene en el NDJSON" << endl;
    enviarAMempool(mempool, indices, yaConfirmada);

    // Caso 2: transaccion inventada (no esta en el NDJSON) que reusa el txId de otra
    const char* idAjeno = transacciones.obtenerEn(19)->txId_;   // txId de la vigesima cargada
    Transaccion* inventada = registrar(transacciones, new Transaccion(
        idAjeno, time(nullptr), "intruso", "LOGIN",
        "sistema", "OTORGADO", "login intruso", 2));
    cout << endl << "Caso 2: transaccion inventada que reusa el txId " << inventada->txId_ << endl;
    cout << "  ";
    inventada->imprimir();
    enviarAMempool(mempool, indices, inventada);

    cout << endl << "Transacciones en la mempool despues de los intentos: "
         << mempool.tamanoActual() << "/" << mempool.capacidadActual() << endl;
}


// ── 2.3 Consulta al ledger ─────────────────────────────────

void demoConsulta(ListaEnlazada<Bloque>& cadena, Indices& indices, DArray<Transaccion*>& transacciones) {
    titulo("2.3 CONSULTA AL LEDGER: consultarBloque(txId)");
    mostrarConsulta(cadena, indices, transacciones.obtenerEn(19)->txId_);   // existe
    cout << endl;
    mostrarConsulta(cadena, indices, "NOD009-00001-1790841672");            // nodo que no existe
}


// ── 2.4 Flujo completo ─────────────────────────────────────

void demoFlujoCompleto(ListaEnlazada<Bloque>& cadena, Cola<Transaccion*>& mempool,
                       Indices& indices, DArray<Transaccion*>& transacciones) {
    titulo("2.4 FLUJO COMPLETO: Mempool -> bloque -> indice");

    // El evento ocurre "ahora" en el nodo local, que ya registro 14 transacciones en el NDJSON
    long long ahora = time(nullptr);
    char id[64];
    generarTxId(id, NODO_LOCAL, 15, ahora);

    Transaccion* nueva = registrar(transacciones, new Transaccion(
        id, ahora, "jperez", "LOGOUT", "sistema", "OTORGADO", "logout jperez", 0));

    cout << "1. Nueva transaccion:" << endl << "   ";
    nueva->imprimir();

    cout << "2. Es duplicada? " << (indices.esDuplicada(id) ? "SI" : "NO") << endl;

    cout << "3. Enviar a la mempool:" << endl;
    if (enviarAMempool(mempool, indices, nueva))
        cout << "   encolada (" << mempool.tamanoActual() << "/" << mempool.capacidadActual() << ")" << endl;
    cout << "   Pendiente: txExiste=" << (indices.esDuplicada(id) ? "SI" : "NO")
         << " | consultarBloque -> " << indices.consultarBloque(id) << endl;
    cout << "   Reenvio mientras sigue en la mempool:" << endl;
    enviarAMempool(mempool, indices, nueva);

    cout << "4. El lider mueve la mempool (FIFO) a un bloque nuevo y lo confirma" << endl;
    Bloque nuevo = crearBloqueCandidato(cadena);
    while (!mempool.estaVacia()) {
        nuevo.agregarTx(mempool.frenteActual());   // frenteActual() lee el primero
        mempool.desencolar();                      // desencolar() lo saca
    }
    confirmarBloque(cadena, indices, nuevo);
    cout << "   Bloque #" << nuevo.encabezado.height << " confirmado e indexado en txExiste y txLedger" << endl;

    cout << "5. Ahora si es encontrable:" << endl;
    mostrarConsulta(cadena, indices, id);

    cout << "6. Reintento de la misma transaccion:" << endl;
    enviarAMempool(mempool, indices, nueva);
}


int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);   // la consola de Windows no usa UTF-8 por defecto
#endif

    ListaEnlazada<Bloque> cadena;                      // tip en la cabeza, genesis al final
    Indices               indices;                     // txExiste y txLedger
    Cola<Transaccion*>    mempool(CAPACIDAD_MEMPOOL);  // transacciones pendientes
    DArray<Transaccion*>  transacciones;               // duena de todas las Transaccion*

    if (!demoCarga(cadena, indices, transacciones))
        return 1;
    demoDuplicados(mempool, indices, transacciones);
    demoConsulta(cadena, indices, transacciones);
    demoFlujoCompleto(cadena, mempool, indices, transacciones);

    titulo("METRICA");
    cout << indices.txLedger.tamano() << " transacciones indexadas de "
         << cadena.tamanoActual() << " bloques, consulta O(1) promedio" << endl;

    // Unico lugar donde se liberan las transacciones
    for (int i = 0; i < transacciones.tamanoActual(); i++)
        delete transacciones.obtenerEn(i);

    return 0;
}
