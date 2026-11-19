#include <iostream>
#include <cstring>
#include <ctime>    
#include "DArray.h"
#include "Transaccion.h"
#include "Bloque.h"
#include "ListaEnlazada.h"
#include "Cola.h"
#include "Indices.h"
#include "CargadorNDJSON.h"   // incluye HashMerkle.h
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>   // SetConsoleOutputCP: para que "—" y los acentos se vean bien en la consola
#endif
using namespace std;

const char* ARCHIVO_NDJSON    = "transacciones.ndjson";
const char* ARCHIVO_SALIDA    = "cadena_e4.ndjson";       // cadena guardada con merkle_root y block_hash
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

// El bloque ya trae sus transacciones: se calculan merkleRoot y blockHash
// (en ese orden, ver sellarBloque), se indexa y se vuelve el nuevo tip
void confirmarBloque(ListaEnlazada<Bloque>& cadena, Indices& indices, Bloque& b) {
    sellarBloque(b, cadena);
    indices.indexarBloque(b);
    cadena.agregarInicio(b);
}

// Junta punteros a los bloques en orden genesis -> tip (la lista va del tip al genesis)
void bloquesEnOrden(ListaEnlazada<Bloque>& cadena, DArray<Bloque*>& salida) {
    DArray<Bloque*> alReves;
    cadena.paraCada([&](Bloque& b) { alReves.agregar(&b); });
    for (int i = alReves.tamanoActual() - 1; i >= 0; i--)
        salida.agregar(alReves.obtenerEn(i));
}

// Entrega 4: recalcula el merkleRoot del bloque y lo compara con el del encabezado.
// Se compara con strcmp (contenido), NO con == (que compararia direcciones).
bool verificarBloque(const Bloque& b) {
    char recalculado[TAM_HASH];
    calcularMerkleRoot(b, recalculado);
    bool integro = strcmp(recalculado, b.encabezado.merkleRoot) == 0;

    if (integro)
        cout << "  [OK] Bloque #" << b.encabezado.height << " integro: merkleRoot "
             << recalculado << " coincide con el encabezado" << endl;
    else
        cout << "  [ALERTA] Bloque #" << b.encabezado.height << " COMPROMETIDO: merkleRoot recalculado "
             << recalculado << " != guardado " << b.encabezado.merkleRoot << endl;
    return integro;
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
    titulo("[E3] 2.1 ARRANQUE: carga del NDJSON + construccion de indices");
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
    titulo("[E3] 2.2 INTENTO DE DUPLICADO: txExiste.buscar(txId)");

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
    titulo("[E3] 2.3 CONSULTA AL LEDGER: consultarBloque(txId)");
    mostrarConsulta(cadena, indices, transacciones.obtenerEn(19)->txId_);   // existe
    cout << endl;
    mostrarConsulta(cadena, indices, "NOD009-00001-1790841672");            // nodo que no existe
}


// ── 2.4 Flujo completo ─────────────────────────────────────

void demoFlujoCompleto(ListaEnlazada<Bloque>& cadena, Cola<Transaccion*>& mempool,
                       Indices& indices, DArray<Transaccion*>& transacciones) {
    titulo("[E3] 2.4 FLUJO COMPLETO: Mempool -> bloque -> indice");

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
    cout << "   merkleRoot=" << nuevo.encabezado.merkleRoot
         << "  blockHash=" << nuevo.encabezado.blockHash
         << "  prevHash=" << nuevo.encabezado.prevHash << endl;

    cout << "5. Ahora si es encontrable:" << endl;
    mostrarConsulta(cadena, indices, id);

    cout << "6. Reintento de la misma transaccion:" << endl;
    enviarAMempool(mempool, indices, nueva);
}


// ══ Entrega 4 ══════════════════════════════════════════════

// ── E4-2.1 Carga con calculo de merkleRoot y blockHash ─────
// Los hashes ya se calcularon dentro de cargarNDJSON (sellarBloque):
// 1o calcularMerkleRoot, 2o calcularBlockHash. Aqui se imprimen.
void demoHashesCarga(ListaEnlazada<Bloque>& cadena) {
    titulo("[E4] 2.1 CARGA: merkleRoot y blockHash de cada bloque");
    DArray<Bloque*> bloques;
    bloquesEnOrden(cadena, bloques);
    for (int i = 0; i < bloques.tamanoActual(); i++) {
        const EncabezadoBloque& enc = bloques.obtenerEn(i)->encabezado;
        cout << "Bloque #" << enc.height
             << "  txs=" << enc.txCount
             << "  merkleRoot=" << enc.merkleRoot
             << "  blockHash=" << enc.blockHash << endl;
    }

    // Determinismo: calcular dos veces el mismo bloque da el mismo resultado
    Bloque* primero = bloques.obtenerEn(0);
    char r1[TAM_HASH], r2[TAM_HASH];
    calcularMerkleRoot(*primero, r1);
    calcularMerkleRoot(*primero, r2);
    cout << endl << "Determinismo (bloque #" << primero->encabezado.height << "): "
         << r1 << " vs " << r2 << " -> " << (strcmp(r1, r2) == 0 ? "IGUALES" : "DISTINTOS") << endl;

    // Encadenamiento: el prevHash de cada bloque es el blockHash del anterior
    bool encadenada = true;
    for (int i = 1; i < bloques.tamanoActual(); i++)
        if (strcmp(bloques.obtenerEn(i)->encabezado.prevHash,
                   bloques.obtenerEn(i - 1)->encabezado.blockHash) != 0)
            encadenada = false;
    cout << "Encadenamiento prevHash -> blockHash: " << (encadenada ? "CORRECTO" : "ROTO") << endl;
}

// ── E4-2.2 Verificacion de integridad: bloque sin modificar ─
void demoVerificacion(ListaEnlazada<Bloque>& cadena) {
    titulo("[E4] 2.2 VERIFICACION: verificarBloque() sobre bloques sin modificar");
    DArray<Bloque*> bloques;
    bloquesEnOrden(cadena, bloques);
    int integros = 0;
    for (int i = 0; i < bloques.tamanoActual(); i++)
        if (verificarBloque(*bloques.obtenerEn(i)))
            integros++;
    cout << endl << "Bloques integros: " << integros << "/" << bloques.tamanoActual() << endl;
}

// ── E4-2.3 Deteccion de manipulacion ────────────────────────
// Un "insider" cambia en memoria el resultado de un evento ya confirmado:
// un acceso que fue DENEGADO pasa a decir OTORGADO (o al reves) para
// ocultar / fabricar evidencia. El encabezado del bloque no se toca.
void demoManipulacion(ListaEnlazada<Bloque>& cadena) {
    titulo("[E4] 2.3 DETECCION DE MANIPULACION en un bloque confirmado");

    // Se busca la primera transaccion DENEGADA de la cadena (si no hay, la primera del bloque #1)
    Bloque* victima = nullptr;
    Transaccion* tx = nullptr;
    DArray<Bloque*> bloques;
    bloquesEnOrden(cadena, bloques);
    for (int i = 0; i < bloques.tamanoActual() && tx == nullptr; i++)
        for (int j = 0; j < bloques.obtenerEn(i)->numTx() && tx == nullptr; j++)
            if (strcmp(bloques.obtenerEn(i)->obtenerTx(j)->estado_, "DENEGADO") == 0) {
                victima = bloques.obtenerEn(i);
                tx = victima->obtenerTx(j);
            }
    if (tx == nullptr) {
        victima = bloques.obtenerEn(bloques.tamanoActual() > 1 ? 1 : 0);
        tx = victima->obtenerTx(0);
    }

    char blockHashAntes[TAM_HASH];
    strcpy(blockHashAntes, victima->encabezado.blockHash);

    cout << "Transaccion original en el bloque #" << victima->encabezado.height << ":" << endl << "  ";
    tx->imprimir();

    // 1. Guardar el valor original y modificar en memoria (no en el archivo)
    char* estadoOriginal = tx->estado_;
    const char* falso = (strcmp(estadoOriginal, "DENEGADO") == 0) ? "OTORGADO" : "DENEGADO";
    tx->estado_ = new char[strlen(falso) + 1];
    strcpy(tx->estado_, falso);

    cout << endl << "1. Manipulacion: estado " << estadoOriginal << " -> " << tx->estado_ << endl << "  ";
    tx->imprimir();
    cout << "   blockHash del encabezado sigue igual: " << victima->encabezado.blockHash
         << (strcmp(blockHashAntes, victima->encabezado.blockHash) == 0 ? " (sin cambios)" : " (CAMBIO)") << endl;

    cout << "2. verificarBloque sobre el bloque manipulado:" << endl;
    bool r1 = verificarBloque(*victima);
    cout << "   -> retorna " << (r1 ? "true" : "false") << endl;

    // 3. Restaurar el valor original
    delete[] tx->estado_;
    tx->estado_ = estadoOriginal;
    cout << "3. Restauracion: estado vuelve a " << tx->estado_ << endl;
    bool r2 = verificarBloque(*victima);
    cout << "   -> retorna " << (r2 ? "true" : "false") << endl;
}

// ── Extra: guardar la cadena con el encabezado completo ────
void demoGuardar(ListaEnlazada<Bloque>& cadena) {
    titulo("[E4] EXTRA: guardar la cadena con merkle_root y block_hash");
    if (!guardarNDJSON(ARCHIVO_SALIDA, cadena)) return;
    cout << "Cadena guardada en " << ARCHIVO_SALIDA << endl;

    // Se vuelve a cargar para comprobar que el merkle_root guardado coincide
    ListaEnlazada<Bloque> cadena2;
    Indices               indices2;
    DArray<Transaccion*>  transacciones2;
    if (cargarNDJSON(ARCHIVO_SALIDA, cadena2, indices2, transacciones2)) {
        bool iguales = cadena2.tamanoActual() == cadena.tamanoActual() &&
                       strcmp(cadena2.obtenerEn(0).encabezado.blockHash,
                              cadena.obtenerEn(0).encabezado.blockHash) == 0;
        cout << "Recarga: " << cadena2.tamanoActual() << " bloques, blockHash del tip "
             << (iguales ? "COINCIDE" : "NO COINCIDE") << endl;
    }
    for (int i = 0; i < transacciones2.tamanoActual(); i++)
        delete transacciones2.obtenerEn(i);
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
    demoHashesCarga(cadena);                                // E4-2.1
    demoDuplicados(mempool, indices, transacciones);
    demoConsulta(cadena, indices, transacciones);
    demoFlujoCompleto(cadena, mempool, indices, transacciones);
    demoVerificacion(cadena);                               // E4-2.2
    demoManipulacion(cadena);                               // E4-2.3
    demoGuardar(cadena);                                    // extra (obligatorio en E5)

    cout << endl;
    // Unico lugar donde se liberan las transacciones
    for (int i = 0; i < transacciones.tamanoActual(); i++)
        delete transacciones.obtenerEn(i);

    return 0;
}
