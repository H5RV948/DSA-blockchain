// TablaHash.h — Tabla hash genérica con linear probing y tombstone
// TC1031 · Mini-Blockchain · 
// Convenciones: sin STL, sin std::string, nombres en español
//
// Restricciones del curso:
//   - Clave: solo char* (comparación con strcmp, hash sobre bytes)
//   - Valor: cualquier tipo copiable
//   - Sin eliminación real: se marca con lápida (tombstone) para no romper probing
//   - Factor de carga máximo 0.7 → rehash automático al duplicar capacidad

#pragma once
#include <cstring>   // strcmp, strcpy, strlen
#include <cstdlib>   // malloc, free
#include <cstdio>    // printf (solo en imprimir())
#include <cassert>

// ──────────────────────────────────────────────
// Funciones hash sobre char*
// ──────────────────────────────────────────────

// djb2: Daniel J. Bernstein, 1991
// hash = 5381; para cada byte c: hash = hash * 33 + c
// El 33 (== 2^5 + 1) mezcla bien con desplazamiento + suma.
// Simple, rápida, buena distribución para cadenas cortas.
inline unsigned long long djb2(const char* clave) {
    unsigned long long hash = 5381;
    while (*clave) {
        hash = hash * 33 + (unsigned char)(*clave);
        ++clave;
    }
    return hash;
}

// FNV-1a: Fowler–Noll–Vo, variante "a" (XOR primero, luego multiplica)
// Mejor avalanche que djb2 para cadenas con prefijos comunes (ej: NOD001-*)
// Constantes para 64 bits: primo=1099511628211, offset=14695981039346656037ULL
inline unsigned long long fnv1a(const char* clave) {
    unsigned long long hash = 14695981039346656037ULL;
    while (*clave) {
        hash ^= (unsigned char)(*clave);
        hash *= 1099511628211ULL;
        ++clave;
    }
    return hash;
}

// ──────────────────────────────────────────────
// TablaHash<Valor>
// Clave siempre char*; Valor es el tipo parametrizado.
// ──────────────────────────────────────────────
template<typename Valor>
class TablaHash {
public:
    // ---- tipos de celda ----
    enum class EstadoCelda { VACIA, OCUPADA, LAPIDA };

    struct Celda {
        char*       clave;
        Valor       valor;
        EstadoCelda estado;
        Celda() : clave(nullptr), valor(), estado(EstadoCelda::VACIA) {}
    };

    // ── Constructor ──────────────────────────────────────
    // capacidadInicial debe ser potencia de 2 (simplifica el módulo).
    explicit TablaHash(int capacidadInicial = 16)
        : _cap(capacidadInicial), _tam(0), _lapidas(0)
    {
        assert(capacidadInicial > 0);
        _tabla = new Celda[_cap];
    }

    // ── Regla de los 3 ───────────────────────────────────
    ~TablaHash() { _liberarTodo(); }

    TablaHash(const TablaHash& otro) : _cap(otro._cap), _tam(otro._tam), _lapidas(otro._lapidas) {
        _tabla = new Celda[_cap];
        for (int i = 0; i < _cap; ++i) {
            _tabla[i].estado = otro._tabla[i].estado;
            _tabla[i].valor  = otro._tabla[i].valor;
            if (otro._tabla[i].estado == EstadoCelda::OCUPADA) {
                _tabla[i].clave = _copiarClave(otro._tabla[i].clave);
            } else {
                _tabla[i].clave = nullptr;
            }
        }
    }

    TablaHash& operator=(const TablaHash& otro) {
        if (this == &otro) return *this;
        _liberarTodo();
        _cap     = otro._cap;
        _tam     = otro._tam;
        _lapidas = otro._lapidas;
        _tabla   = new Celda[_cap];
        for (int i = 0; i < _cap; ++i) {
            _tabla[i].estado = otro._tabla[i].estado;
            _tabla[i].valor  = otro._tabla[i].valor;
            if (otro._tabla[i].estado == EstadoCelda::OCUPADA) {
                _tabla[i].clave = _copiarClave(otro._tabla[i].clave);
            } else {
                _tabla[i].clave = nullptr;
            }
        }
        return *this;
    }

    // ── insertar ─────────────────────────────────────────
    // Pre: clave != nullptr.
    // Si la clave ya existe, actualiza el valor.
    // Rehash automático si factor de carga > 0.7.
    void insertar(const char* clave, const Valor& valor) {
        if (_factorCarga() > 0.7f) _rehash();

        int idx = _buscarSlot(clave);
        if (_tabla[idx].estado == EstadoCelda::OCUPADA &&
            strcmp(_tabla[idx].clave, clave) == 0) {
            // actualizar
            _tabla[idx].valor = valor;
            return;
        }
        // nuevo
        _tabla[idx].clave  = _copiarClave(clave);
        _tabla[idx].valor  = valor;
        _tabla[idx].estado = EstadoCelda::OCUPADA;
        ++_tam;
    }

    // ── buscar ───────────────────────────────────────────
    // Devuelve puntero al valor o nullptr si no existe.
    Valor* buscar(const char* clave) {
        int inicio = (int)(_hashFn(clave) % (unsigned long)_cap);
        for (int i = 0; i < _cap; ++i) {
            int idx = (inicio + i) % _cap;
            if (_tabla[idx].estado == EstadoCelda::VACIA) return nullptr;
            if (_tabla[idx].estado == EstadoCelda::OCUPADA &&
                strcmp(_tabla[idx].clave, clave) == 0)
                return &_tabla[idx].valor;
        }
        return nullptr;
    }

    const Valor* buscar(const char* clave) const {
        int inicio = (int)(_hashFn(clave) % (unsigned long)_cap);
        for (int i = 0; i < _cap; ++i) {
            int idx = (inicio + i) % _cap;
            if (_tabla[idx].estado == EstadoCelda::VACIA) return nullptr;
            if (_tabla[idx].estado == EstadoCelda::OCUPADA &&
                strcmp(_tabla[idx].clave, clave) == 0)
                return &_tabla[idx].valor;
        }
        return nullptr;
    }

    // ── contiene ─────────────────────────────────────────
    bool contiene(const char* clave) const { return buscar(clave) != nullptr; }

    // ── tamaño / capacidad ───────────────────────────────
    int tamano()    const { return _tam;  }
    int capacidad() const { return _cap;  }
    float factorCarga() const { return _factorCarga(); }

    // ── imprimir (diagnóstico) ───────────────────────────
    void imprimir() const {
        printf("TablaHash [tam=%d cap=%d fc=%.2f]\n", _tam, _cap, _factorCarga());
        for (int i = 0; i < _cap; ++i) {
            if (_tabla[i].estado == EstadoCelda::OCUPADA)
                printf("  [%3d] %s\n", i, _tabla[i].clave);
            else if (_tabla[i].estado == EstadoCelda::LAPIDA)
                printf("  [%3d] <lapida>\n", i);
        }
    }

private:
    Celda* _tabla;
    int    _cap;
    int    _tam;
    int    _lapidas;

    // Función hash elegida: FNV-1a (mejor para txIds con prefijos comunes)
    static unsigned long _hashFn(const char* c) { return fnv1a(c); }

    float _factorCarga() const {
        return (float)(_tam + _lapidas) / (float)_cap;
    }

    // Devuelve el índice donde insertar (puede ser lápida o vacío)
    // shortcut: no maneja tabla 100% llena; el rehash previo lo evita
    int _buscarSlot(const char* clave) const {
        int inicio  = (int)(_hashFn(clave) % (unsigned long)_cap);
        int primerL = -1; // primera lápida encontrada
        for (int i = 0; i < _cap; ++i) {
            int idx = (inicio + i) % _cap;
            if (_tabla[idx].estado == EstadoCelda::VACIA)
                return (primerL != -1) ? primerL : idx;
            if (_tabla[idx].estado == EstadoCelda::LAPIDA && primerL == -1)
                primerL = idx;
            if (_tabla[idx].estado == EstadoCelda::OCUPADA &&
                strcmp(_tabla[idx].clave, clave) == 0)
                return idx; // actualización
        }
        return primerL; // solo lápidas; ya había espacio
    }

    void _rehash() {
        int    capVieja  = _cap;
        Celda* tablaVieja = _tabla;

        _cap    *= 2;
        _tam     = 0;
        _lapidas = 0;
        _tabla   = new Celda[_cap];

        for (int i = 0; i < capVieja; ++i) {
            if (tablaVieja[i].estado == EstadoCelda::OCUPADA) {
                insertar(tablaVieja[i].clave, tablaVieja[i].valor);
                delete[] tablaVieja[i].clave;
            }
        }
        delete[] tablaVieja;
    }

    static char* _copiarClave(const char* src) {
        int n = (int)strlen(src) + 1;
        char* dst = new char[n];
        memcpy(dst, src, n);
        return dst;
    }

    void _liberarTodo() {
        for (int i = 0; i < _cap; ++i)
            if (_tabla[i].estado == EstadoCelda::OCUPADA)
                delete[] _tabla[i].clave;
        delete[] _tabla;
    }
};
