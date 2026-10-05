// generador_tx.cpp
// Generador de transacciones para el Mini-Blockchain Educativo.
//
// USO:
//   Modo interactivo:  ./generador_tx [archivo.ndjson]
//   Modo automatico:   ./generador_tx archivo.ndjson N
//
//   archivo.ndjson  : nombre del archivo de salida (default: transacciones.ndjson)
//   N               : numero de transacciones a generar automaticamente
//
// El archivo generado contiene SOLO transacciones (lineas NDJSON).
// Estas transacciones se usaran como entrada para la simulacion principal,
// que las distribuira entre los nodos como candidatas para la mempool y los bloques.
//
// Compilar: g++ -std=c++17 generador_tx.cpp -o generador_tx

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <ctime>

// ── Parametros controlables del modo automatico ───────────────────────────────
// Modificar estos arreglos para ajustar el universo de datos generados.

const char* ALUMNOS[] = {
    "A001", "A002", "A003", "A004", "A005",
    "A006", "A007", "A008", "A009", "A010"
};
const int NUM_ALUMNOS = 10;

const char* ACTIVIDADES[] = {
    "ACT01", "ACT02", "ACT03", "ACT04", "ACT05"
};
const int NUM_ACTIVIDADES = 5;

const char* OPERACIONES[] = {
    "CREATE", "UPDATE", "VOID"
};
const int NUM_OPERACIONES = 3;
// Pesos de probabilidad para cada operacion (deben sumar 100)
const int PESO_CREATE = 70;   // 70% CREATE
const int PESO_UPDATE = 25;   // 25% UPDATE
const int PESO_VOID   = 5;    //  5% VOID

const double CALIF_MIN = 0.0;
const double CALIF_MAX = 10.0;

// ── Utilidades ────────────────────────────────────────────────────────────────

// Genera un txId unico basado en timestamp + contador secuencial.
// Formato: tx_TIMESTAMP_CONTADOR (ej. tx_1728000000_001)
void generarTxId(char* destino, int contador) {
    char buf[32];
    // Convertir tiempo a string
    long t = (long)time(nullptr);
    // Construir: "tx_" + tiempo + "_" + contador con 3 digitos
    strcpy(destino, "tx_");
    // Agregar timestamp
    long tmp = t;
    char tstr[20];
    int ti = 0;
    if (tmp == 0) { tstr[ti++] = '0'; }
    while (tmp > 0) { tstr[ti++] = '0' + (tmp % 10); tmp /= 10; }
    // Invertir
    for (int j = 0; j < ti / 2; j++) {
        char c = tstr[j]; tstr[j] = tstr[ti-1-j]; tstr[ti-1-j] = c;
    }
    tstr[ti] = '\0';
    strcat(destino, tstr);
    strcat(destino, "_");
    // Agregar contador con padding de 3 digitos
    buf[0] = '0' + (contador / 100) % 10;
    buf[1] = '0' + (contador / 10)  % 10;
    buf[2] = '0' + (contador)       % 10;
    buf[3] = '\0';
    strcat(destino, buf);
}

// Convierte double a string con 1 decimal (ej. 8.5 -> "8.5")
void doubleAStr(double valor, char* destino) {
    int entero = (int)valor;
    int decimal = (int)((valor - entero) * 10 + 0.5);
    if (decimal >= 10) { entero++; decimal = 0; }
    char buf[32];
    int i = 0;
    if (entero == 0) { buf[i++] = '0'; }
    int tmp = entero;
    int start = i;
    while (tmp > 0) { buf[i++] = '0' + (tmp % 10); tmp /= 10; }
    // Invertir la parte entera
    for (int j = start; j < start + (i - start) / 2; j++) {
        char c = buf[j]; buf[j] = buf[i-1-(j-start)]; buf[i-1-(j-start)] = c;
    }
    buf[i++] = '.';
    buf[i++] = '0' + decimal;
    buf[i] = '\0';
    strcpy(destino, buf);
}

// Escribe una transaccion en formato NDJSON al archivo.
// Formato: linea JSON sin sangria (las transacciones sueltas no tienen bloque padre)
void escribirTransaccion(std::ofstream& archivo,
                          const char* txId,
                          const char* operacion,
                          const char* idAlumno,
                          const char* actividad,
                          double calificacion,
                          long long marcaTiempo) {
    char califStr[16];
    doubleAStr(calificacion, califStr);

    archivo << "{\"tipo\":\"tx\""
            << ",\"tx_id\":\"" << txId << "\""
            << ",\"operacion\":\"" << operacion << "\""
            << ",\"id_alumno\":\"" << idAlumno << "\""
            << ",\"actividad\":\"" << actividad << "\""
            << ",\"calificacion\":" << califStr
            << ",\"marca_tiempo\":" << marcaTiempo
            << "}" << std::endl;
}

// Selecciona una operacion aleatoria respetando los pesos definidos
const char* operacionAleatoria() {
    int r = rand() % 100;
    if (r < PESO_CREATE) return OPERACIONES[0];       // CREATE
    if (r < PESO_CREATE + PESO_UPDATE) return OPERACIONES[1]; // UPDATE
    return OPERACIONES[2];                             // VOID
}

// Genera una calificacion aleatoria con 1 decimal entre CALIF_MIN y CALIF_MAX
double califAleatoria() {
    int pasos = (int)((CALIF_MAX - CALIF_MIN) * 10);
    return CALIF_MIN + (rand() % (pasos + 1)) / 10.0;
}

// ── Modo interactivo ──────────────────────────────────────────────────────────

void modoInteractivo(std::ofstream& archivo, int& contadorTx) {
    char txId[64], operacion[32], idAlumno[32], actividad[32];
    double calificacion;
    char respuesta[8];

    std::cout << std::endl;
    std::cout << "=== Modo interactivo ===" << std::endl;
    std::cout << "Ingrese cada campo. Deje txId en blanco para generarlo automaticamente." << std::endl;
    std::cout << "Escriba 'fin' en cualquier campo para terminar." << std::endl;

    while (true) {
        std::cout << std::endl << "--- Transaccion #" << (contadorTx + 1) << " ---" << std::endl;

        // txId
        std::cout << "txId (Enter = autogenerar): ";
        std::cin.getline(txId, sizeof(txId));
        if (strcmp(txId, "fin") == 0) break;
        if (strlen(txId) == 0) {
            generarTxId(txId, contadorTx + 1);
            std::cout << "  -> txId generado: " << txId << std::endl;
        }

        // operacion
        std::cout << "Operacion [CREATE/UPDATE/VOID]: ";
        std::cin.getline(operacion, sizeof(operacion));
        if (strcmp(operacion, "fin") == 0) break;
        if (strlen(operacion) == 0) strcpy(operacion, "CREATE");

        // idAlumno
        std::cout << "ID alumno: ";
        std::cin.getline(idAlumno, sizeof(idAlumno));
        if (strcmp(idAlumno, "fin") == 0) break;

        // actividad
        std::cout << "Actividad: ";
        std::cin.getline(actividad, sizeof(actividad));
        if (strcmp(actividad, "fin") == 0) break;

        // calificacion
        char califBuf[32];
        std::cout << "Calificacion (0.0 - 10.0): ";
        std::cin.getline(califBuf, sizeof(califBuf));
        if (strcmp(califBuf, "fin") == 0) break;
        calificacion = (strlen(califBuf) > 0) ? atof(califBuf) : 0.0;

        // marca de tiempo
        long long marcaTiempo = (long long)time(nullptr) + contadorTx;

        escribirTransaccion(archivo, txId, operacion, idAlumno, actividad,
                            calificacion, marcaTiempo);
        contadorTx++;
        std::cout << "  -> Transaccion guardada." << std::endl;

        // Continuar?
        std::cout << "Agregar otra? [s/n]: ";
        std::cin.getline(respuesta, sizeof(respuesta));
        if (respuesta[0] == 'n' || respuesta[0] == 'N') break;
    }
}

// ── Modo automatico ───────────────────────────────────────────────────────────

void modoAutomatico(std::ofstream& archivo, int cantidad, int& contadorTx) {
    std::cout << std::endl;
    std::cout << "=== Modo automatico: generando " << cantidad
              << " transacciones ===" << std::endl;

    // Mostrar parametros activos
    std::cout << "Alumnos:     " << NUM_ALUMNOS << " (A001-A0"
              << (NUM_ALUMNOS < 10 ? "0" : "") << NUM_ALUMNOS << ")" << std::endl;
    std::cout << "Actividades: " << NUM_ACTIVIDADES
              << " (ACT01-ACT0" << NUM_ACTIVIDADES << ")" << std::endl;
    std::cout << "Operaciones: CREATE=" << PESO_CREATE << "% "
              << "UPDATE=" << PESO_UPDATE << "% "
              << "VOID=" << PESO_VOID << "%" << std::endl;
    std::cout << "Calificacion: " << CALIF_MIN << " - " << CALIF_MAX << std::endl;
    std::cout << std::endl;

    long long tiempoBase = (long long)time(nullptr);

    for (int i = 0; i < cantidad; i++) {
        char txId[64];
        generarTxId(txId, contadorTx + 1);

        const char* operacion = operacionAleatoria();
        const char* idAlumno  = ALUMNOS[rand() % NUM_ALUMNOS];
        const char* actividad = ACTIVIDADES[rand() % NUM_ACTIVIDADES];
        double calificacion   = califAleatoria();

        // VOID siempre calificacion 0
        if (strcmp(operacion, "VOID") == 0) calificacion = 0.0;

        long long marcaTiempo = tiempoBase + i;

        escribirTransaccion(archivo, txId, operacion, idAlumno, actividad,
                            calificacion, marcaTiempo);
        contadorTx++;

        // Progreso cada 10 tx
        if ((i + 1) % 10 == 0 || i == cantidad - 1) {
            std::cout << "  Generadas: " << (i + 1) << "/" << cantidad << std::endl;
        }
    }
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    // Determinar nombre del archivo y modo
    const char* nombreArchivo = "transacciones.ndjson";
    int cantidadAuto = 0;
    bool esAutomatico = false;

    if (argc >= 2) nombreArchivo = argv[1];
    if (argc >= 3) {
        cantidadAuto = atoi(argv[2]);
        esAutomatico = (cantidadAuto > 0);
    }

    std::cout << "=====================================" << std::endl;
    std::cout << "  Generador de Transacciones NDJSON  " << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << "Archivo: " << nombreArchivo << std::endl;
    std::cout << "Modo:    " << (esAutomatico ? "automatico" : "interactivo") << std::endl;

    // Verificar si el archivo existe
    bool modoAppend = false;
    {
        std::ifstream prueba(nombreArchivo);
        if (prueba.is_open()) {
            prueba.close();
            std::cout << std::endl;
            std::cout << "El archivo '" << nombreArchivo << "' ya existe." << std::endl;
            std::cout << "  [a] Agregar al final (append)" << std::endl;
            std::cout << "  [s] Sobreescribir (borrar contenido anterior)" << std::endl;
            std::cout << "  [c] Cancelar" << std::endl;
            std::cout << "Opcion: ";
            char opcion[4];
            std::cin.getline(opcion, sizeof(opcion));
            if (opcion[0] == 'c' || opcion[0] == 'C') {
                std::cout << "Operacion cancelada." << std::endl;
                return 0;
            }
            modoAppend = (opcion[0] == 'a' || opcion[0] == 'A');
        }
    }

    // Abrir archivo
    std::ofstream archivo(nombreArchivo,
        modoAppend ? std::ios::app : std::ios::trunc);
    if (!archivo.is_open()) {
        std::cerr << "Error: no se pudo abrir el archivo '" << nombreArchivo << "'" << std::endl;
        return 1;
    }

    // Inicializar semilla aleatoria
    srand((unsigned int)time(nullptr));

    int contadorTx = 0;

    if (esAutomatico) {
        modoAutomatico(archivo, cantidadAuto, contadorTx);
    } else {
        modoInteractivo(archivo, contadorTx);
    }

    archivo.close();

    std::cout << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << "Transacciones escritas: " << contadorTx << std::endl;
    std::cout << "Archivo: " << nombreArchivo << std::endl;
    std::cout << "=====================================" << std::endl;

    return 0;
}
