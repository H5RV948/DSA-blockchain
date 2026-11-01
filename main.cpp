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
#include "TablaHash.h"
using namespace std;

const char* ARCHIVO_NDJSON  = "transacciones.ndjson";
const int   TAM_LINEA       = 1024;
const int   CAPACIDAD_MEMPOOL = 5;
const int   ALTURA_FALLA    = 3;   

// Registro de todas las transacciones 
DArray<Transaccion*> todasTx;

// Indices sobre la cadena
TablaHash<bool> txExiste;   // txId -> true
TablaHash<int>  txLedger;   // txId -> altura del bloque

// Inserta txId en ambos indices
void indexarTx(Transaccion* tx, int altura) {
    txExiste.insertar(tx->txId_, true);
    txLedger.insertar(tx->txId_, altura);
}

// Altura del bloque que contiene txId, o -1 si no existe
int consultarBloque(const char* txId) {
    int* altura = txLedger.buscar(txId);
    return (altura == nullptr) ? -1 : *altura;
}

// Recorre la cadena buscando el bloque con esa altura
bool buscarBloquePorAltura(ListaEnlazada<Bloque>& cadena, int altura, Bloque& destino) {
    bool encontrado = false;
    cadena.paraCada([&](Bloque& b) {
        if (!encontrado && b.encabezado.height == altura) {
            destino = b;
            encontrado = true;
        }
    });
    return encontrado;
}

// Solo encola si el txId no se ha procesado antes.
// Al encolar se marca en txExiste (pendiente: existe pero aun no tiene bloque en txLedger)
bool encolarSiNoDuplicada(Cola<Transaccion*>& mempool, Transaccion* tx) {
    bool* existe = txExiste.buscar(tx->txId_);   // nullptr si nunca se ha visto
    if (existe != nullptr && *existe) {
        cout << "  [ERROR] txId duplicado: " << tx->txId_ << " — transacción rechazada" << endl;
        return false;
    }
    if (!mempool.encolar(tx)) return false;   // mempool llena: no se marca
    txExiste.insertar(tx->txId_, true);
    return true;
}

// Agrega el bloque como nuevo tip e indexa sus transacciones
void confirmarBloque(ListaEnlazada<Bloque>& cadena, Bloque& b) {
    for (int i = 0; i < b.txList.tamanoActual(); i++)
        indexarTx(b.txList.obtenerEn(i), b.encabezado.height);
    cadena.agregarInicio(b);
}

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
    char txId[64], fecha[32], hora[32], user[64], evento[64], objeto[128], estado[32], comando[128];
    extraerCadena(linea, "tx_id",   txId,    64);
    extraerCadena(linea, "fecha",   fecha,   32);
    extraerCadena(linea, "hora",    hora,    32);
    extraerCadena(linea, "user",    user,    64);
    extraerCadena(linea, "evento",  evento,  64);
    extraerCadena(linea, "objeto",  objeto,  128);
    extraerCadena(linea, "estado",  estado,  32);
    extraerCadena(linea, "comando", comando, 128);
    int id         = extraerEntero(linea, "id");
    int privilegio = extraerEntero(linea, "privilegio");

    Transaccion* tx = new Transaccion(txId, fecha, hora, user, evento, objeto, estado, comando, id, privilegio);
    todasTx.agregar(tx);
    return tx;
}

// Crea una transacción nueva y la registra
Transaccion* crearTx(const char* txId, const char* fecha, const char* hora, const char* user, const char* evento,
                     const char* objeto, const char* estado, const char* comando, int id, int privilegio) {
    Transaccion* tx = new Transaccion(txId, fecha, hora, user, evento, objeto, estado, comando, id, privilegio);
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
            Transaccion* tx = crearTxDesdeLinea(linea);
            actual.agregarTx(tx);
            indexarTx(tx, actual.encabezado.height);
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
    cout << "    TX [" << tx->txId_ << "] " << tx->evento_ << " | user=" << tx->user_
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
    
    separador("CARGA DEL NDJSON EN ListaEnlazada<Bloque> + INDICES HASH");
    if (!cargarCadena(ARCHIVO_NDJSON, cadena)) return 1;

    cout << "Bloques cargados: " << cadena.tamanoActual()
         << " | Transacciones cargadas: " << todasTx.tamanoActual() << endl;
    cout << "Indexadas en txExiste: " << txExiste.tamano()
         << " (cap=" << txExiste.capacidad() << ", fc=" << txExiste.factorCarga() << ")" << endl;
    cout << "Indexadas en txLedger: " << txLedger.tamano()
         << " (cap=" << txLedger.capacidad() << ", fc=" << txLedger.factorCarga() << ")" << endl;
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
    pendientes[0] = crearTx("TX-00000043", "2026-10-02", "10:00:05", "jperez",  "LOGIN",          "sistema",                 "OTORGADO", "login jperez",            43, 0);
    pendientes[1] = crearTx("TX-00000044", "2026-10-02", "10:02:17", "jperez",  "LEER_ARCHIVO",   "/home/jperez/notas.txt",  "OTORGADO", "cat notas.txt",           44, 0);
    pendientes[2] = crearTx("TX-00000045", "2026-10-02", "10:05:40", "mlopez",  "ELIMINAR_ARCHIVO","/finanzas/balance.xlsx",  "DENEGADO", "rm balance.xlsx",         45, 0);
    pendientes[3] = crearTx("TX-00000046", "2026-10-02", "10:06:02", "admin",   "BLOQUEAR_USUARIO","usuario:mlopez",          "OTORGADO", "usermod -L mlopez",       46, 2);
    pendientes[4] = crearTx("TX-00000047", "2026-10-02", "10:09:33", "rgarcia", "LOGIN",          "sistema",                 "DENEGADO", "login rgarcia",           47, 0);
    pendientes[5] = crearTx("TX-00000048", "2026-10-02", "10:10:01", "rgarcia", "LOGIN",          "sistema",                 "OTORGADO", "login rgarcia",           48, 0);
    pendientes[6] = crearTx("TX-00000049", "2026-10-02", "10:12:45", "admin",   "LOGOUT",         "sistema",                 "OTORGADO", "logout admin",            49, 2);

    // Encolar 5 transacciones
    cout << "Encolando transacciones:" << endl;
    for (int i = 0; i < 5; i++) {
        bool ok = encolarSiNoDuplicada(mempool, pendientes[i]);
        cout << "  encolar TX [" << pendientes[i]->id_ << "] " << pendientes[i]->evento_
             << (ok ? "  -> aceptada" : "  -> RECHAZADA") << "  (" << mempool.tamanoActual()
             << "/" << mempool.capacidadActual() << ")" << endl;
    }

    // Encolar con la Mempool llena
    cout << endl << "Mempool llena? " << (mempool.estaLlena() ? "SI" : "NO") << endl;
    bool aceptada = encolarSiNoDuplicada(mempool, pendientes[5]);
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
        bool ok = encolarSiNoDuplicada(mempool, pendientes[i]);
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
    cout << endl << "Bloque candidato confirmado, indexado y agregado con agregarInicio():" << endl;
    candidato.imprimir();
    confirmarBloque(cadena, candidato);
    cout << "Bloques en la cadena: " << cadena.tamanoActual()
         << " | Nuevo tip: #" << cadena.obtenerEn(0).encabezado.height
         << " | txIds indexados: " << txExiste.tamano() << endl;

    separador("INTENTO DE DUPLICADO: txExiste.buscar(txId)");
    // Caso 1: txId que viene del NDJSON, con datos inventados distintos
    Transaccion* dupNdjson = crearTx("TX-00000010", "2026-10-03", "11:00:00", "intruso", "LOGIN",
                                     "sistema", "OTORGADO", "login intruso", 10, 0);
    // Caso 2: txId que no esta en el NDJSON, pero ya se confirmo en el bloque candidato
    Transaccion* dupNuevo  = crearTx("TX-00000045", "2026-10-03", "11:05:00", "mlopez", "ELIMINAR_ARCHIVO",
                                     "/finanzas/balance.xlsx", "DENEGADO", "rm balance.xlsx", 45, 0);
    Cola<Transaccion*> mempool2(CAPACIDAD_MEMPOOL);
    cout << "Reenviando TX [" << dupNdjson->txId_ << "] (ya existe en el NDJSON):" << endl;
    encolarSiNoDuplicada(mempool2, dupNdjson);
    cout << "Reenviando TX [" << dupNuevo->txId_ << "] (confirmada en el bloque #"
         << consultarBloque(dupNuevo->txId_) << ", no en el NDJSON):" << endl;
    encolarSiNoDuplicada(mempool2, dupNuevo);
    cout << "Transacciones en la mempool despues de los intentos: " << mempool2.tamanoActual() << endl;

    separador("CONSULTA AL LEDGER: consultarBloque(txId)");
    const char* consultas[2] = { "TX-00000020", "TX-99999999" };
    for (int i = 0; i < 2; i++) {
        int altura = consultarBloque(consultas[i]);
        cout << "consultarBloque(\"" << consultas[i] << "\") -> " << altura << endl;
        Bloque encontrado;
        if (altura != -1 && buscarBloquePorAltura(cadena, altura, encontrado)) {
            encontrado.imprimir();
        } else {
            cout << "  txId " << consultas[i] << " no encontrado en el ledger" << endl;
        }
        cout << endl;
    }

    separador("FLUJO COMPLETO: Mempool -> bloque -> indice");
    Transaccion* nueva = crearTx("TX-00000050", "2026-10-02", "10:20:00", "jperez", "LOGOUT",
                                 "sistema", "OTORGADO", "logout jperez", 50, 0);
    cout << "1. Nueva TX [" << nueva->txId_ << "] creada" << endl;
    cout << "2. consultarBloque antes de confirmar -> " << consultarBloque(nueva->txId_) << endl;
    cout << "3. Es duplicado? " << (txExiste.buscar(nueva->txId_) != nullptr ? "SI" : "NO") << endl;
    if (encolarSiNoDuplicada(mempool2, nueva))
        cout << "4. Encolada en la mempool (" << mempool2.tamanoActual() << "/"
             << mempool2.capacidadActual() << ")" << endl;
    cout << "   Pendiente: txExiste=" << (txExiste.buscar(nueva->txId_) != nullptr ? "SI" : "NO")
         << " | consultarBloque -> " << consultarBloque(nueva->txId_) << endl;
    cout << "   Reenvio mientras sigue en la mempool:" << endl;
    encolarSiNoDuplicada(mempool2, nueva);

    Bloque tip2 = cadena.obtenerEn(0);
    Bloque nuevoBloque(EncabezadoBloque(tip2.encabezado.height + 1, 0,
                                        tip2.encabezado.blockHash, "0000000000000000"));
    while (!mempool2.estaVacia()) {
        nuevoBloque.agregarTx(mempool2.frenteActual());
        mempool2.desencolar();
    }
    confirmarBloque(cadena, nuevoBloque);
    cout << "5. Confirmada en el bloque #" << nuevoBloque.encabezado.height
         << " e insertada en txExiste y txLedger" << endl;

    int alturaNueva = consultarBloque(nueva->txId_);
    cout << "6. consultarBloque(\"" << nueva->txId_ << "\") -> " << alturaNueva << endl;
    Bloque verificado;
    if (buscarBloquePorAltura(cadena, alturaNueva, verificado)) verificado.imprimir();
    cout << "7. Reintento de la misma TX:" << endl;
    encolarSiNoDuplicada(mempool2, nueva);
    cout << endl << "Metrica: " << txExiste.tamano() << " transacciones indexadas de "
         << cadena.tamanoActual() << " bloques, busqueda O(1) promedio" << endl;

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
