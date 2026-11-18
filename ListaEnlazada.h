#pragma once

#include <iostream>

template <class T>
class node
{
public:
    node(T data);
    T &getData();
    node<T> *getNext();
    void setData(T data);
    void setNext(node<T> *next);

private:
    T data;
    node<T> *next;
};

template <class T>
class ListaEnlazada
{
private:
    node<T> *head;
    node<T> *tail;
    int size;
    void vaciar();
    void copiar(const ListaEnlazada<T> &otra);

public:
    ListaEnlazada();
    ListaEnlazada(const ListaEnlazada<T> &otra);
    ListaEnlazada<T> &operator=(const ListaEnlazada<T> &otra);
    ~ListaEnlazada();

    void agregarInicio(T valor);
    void agregarFinal(T valor);
    bool eliminar(T valor);
    T obtenerEn(int indice);
    template <class Funcion>
    void paraCada(Funcion f);
    bool estaVacia();
    int tamanoActual();
};

template <class T>
inline node<T>::node(T data) : data(data), next(nullptr)
{
}

template <class T>
inline T &node<T>::getData()
{
    return data;
}

template <class T>
inline node<T> *node<T>::getNext()
{
    return next;
}

template <class T>
inline void node<T>::setData(T data)
{
    this->data = data;
}

template <class T>
inline void node<T>::setNext(node<T> *next)
{
    this->next = next;
}

template <class T>
inline ListaEnlazada<T>::ListaEnlazada()
{
    head = nullptr;
    tail = nullptr;
    size = 0;
}

template <class T>
inline ListaEnlazada<T>::ListaEnlazada(const ListaEnlazada<T> &otra)
{
    head = nullptr;
    tail = nullptr;
    size = 0;
    copiar(otra);
}

template <class T>
inline ListaEnlazada<T> &ListaEnlazada<T>::operator=(const ListaEnlazada<T> &otra)
{
    if (this != &otra)
    {
        vaciar();
        copiar(otra);
    }
    return *this;
}

template <class T>
inline ListaEnlazada<T>::~ListaEnlazada()
{
    vaciar();
}

template <class T>
inline void ListaEnlazada<T>::vaciar()
{
    node<T> *current = head;
    while (current != nullptr)
    {
        node<T> *temp = current;
        current = current->getNext();
        delete temp;
    }
    head = nullptr;
    tail = nullptr;
    size = 0;
}

template <class T>
inline void ListaEnlazada<T>::copiar(const ListaEnlazada<T> &otra)
{
    node<T> *current = otra.head;
    while (current != nullptr)
    {
        agregarFinal(current->getData());
        current = current->getNext();
    }
}

template <class T>
inline void ListaEnlazada<T>::agregarInicio(T valor)
{
    node<T> *newNode = new node<T>(valor);
    newNode->setNext(head);
    head = newNode;
    if (tail == nullptr)
        tail = newNode;
    size++;
}

template <class T>
inline void ListaEnlazada<T>::agregarFinal(T valor)
{
    node<T> *newNode = new node<T>(valor);
    if (tail == nullptr)
    {
        head = newNode;
        tail = newNode;
    }
    else
    {
        tail->setNext(newNode);
        tail = newNode;
    }
    size++;
}

template <class T>
inline bool ListaEnlazada<T>::eliminar(T valor)
{
    node<T> *prev = nullptr;
    node<T> *current = head;
    while (current != nullptr)
    {
        if (current->getData() == valor)
        {
            if (prev == nullptr)
                head = current->getNext();
            else
                prev->setNext(current->getNext());
            if (current == tail)
                tail = prev;
            delete current;
            size--;
            return true;
        }
        prev = current;
        current = current->getNext();
    }
    return false;
}

template <class T>
inline T ListaEnlazada<T>::obtenerEn(int indice)
{
    if (indice < 0 || indice >= size)
    {
        std::cout << "Indice fuera de rango" << std::endl;
        return T();
    }
    node<T> *current = head;
    for (int i = 0; i < indice; i++)
        current = current->getNext();
    return current->getData();
}

template <class T>
template <class Funcion>
inline void ListaEnlazada<T>::paraCada(Funcion f)
{
    node<T> *current = head;
    while (current != nullptr)
    {
        f(current->getData());
        current = current->getNext();
    }
}

template <class T>
inline bool ListaEnlazada<T>::estaVacia()
{
    return size == 0;
}

template <class T>
inline int ListaEnlazada<T>::tamanoActual()
{
    return size;
}