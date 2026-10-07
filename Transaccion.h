#pragma once

#include <iostream>
#include <cstring>
using namespace std;

struct Transaccion {
  char *fecha_, *hora_, *user_, *evento_, *objeto_, *estado_, *comando_; /* 
      - objeto es sobre lo que se hizo el comando
      - estado es si fue otorgado o denegado
      - evento fue lo que paso
      */
  int id_, privilegio_; /*
     - privilegio lo puse aqui porque pienso que lo vamos a definir como: 0 -> admin, 1-> user (algo asi)
     - el id es el numero de la transaccion (o del usuario lo podemos ver)
      */

  // Constructor por defecto
  Transaccion() : id_(-1), privilegio_(-1) {
      fecha_ = new char[1]{'\0'}; // '\0' -> caracter nulo (indica el final de una cadena txt)
      hora_ = new char[1]{'\0'};
      user_ = new char[1]{'\0'};
      evento_ = new char[1]{'\0'};
      objeto_ = new char[1]{'\0'};
      estado_ = new char[1]{'\0'};
      comando_ = new char[1]{'\0'};     
  }
  
  // Constructor
  Transaccion(const char* fecha, const char* hora, const char* user, const char* evento, const char* objeto, const char* estado, const char* comando, int id, int privilegio) : 
    id_(id), privilegio_(privilegio) {
        fecha_ = new char[strlen(fecha) + 1];
        hora_ = new char[strlen(hora) + 1];
        user_ = new char[strlen(user) + 1];
        evento_ = new char[strlen(evento) + 1];
        objeto_ = new char[strlen(objeto) + 1];
        estado_ = new char[strlen(estado) + 1];
        comando_ = new char[strlen(comando) + 1];
  
        strcpy(fecha_, fecha);
        strcpy(hora_, hora);
        strcpy(user_, user);
        strcpy(evento_, evento);
        strcpy(objeto_, objeto);
        strcpy(estado_, estado);
        strcpy(comando_, comando);
    }

    // Constructor de copia
    Transaccion(const Transaccion &o) :
      id_(o.id_), privilegio_(o.privilegio_) {
          fecha_ = new char[strlen(o.fecha_) + 1];
          hora_ = new char[strlen(o.hora_) + 1];
          user_ = new char[strlen(o.user_) + 1];
          evento_ = new char[strlen(o.evento_) + 1];
          objeto_ = new char[strlen(o.objeto_) + 1];
          estado_ = new char[strlen(o.estado_) + 1];
          comando_ = new char[strlen(o.comando_) + 1];
    
          strcpy(fecha_, o.fecha_);
          strcpy(hora_, o.hora_);
          strcpy(user_, o.user_);
          strcpy(evento_, o.evento_);
          strcpy(objeto_, o.objeto_);
          strcpy(estado_, o.estado_);
          strcpy(comando_, o.comando_);
      }

  // Destructor 
  ~Transaccion() {
      delete[] fecha_;
      delete[] hora_;
      delete[] user_;
      delete[] evento_;
      delete[] objeto_;
      delete[] estado_;
      delete[] comando_;
  }

  // Operador de asignacion
  Transaccion& operator=(const Transaccion &o) {
      if (this == &o) return *this;

      delete[] fecha_;
      delete[] hora_;
      delete[] user_;
      delete[] evento_;
      delete[] objeto_;
      delete[] estado_;
      delete[] comando_;

      fecha_ = new char[strlen(o.fecha_) + 1];
      hora_ = new char[strlen(o.hora_) + 1];
      user_ = new char[strlen(o.user_) + 1];
      evento_ = new char[strlen(o.evento_) + 1];
      objeto_ = new char[strlen(o.objeto_) + 1];
      estado_ = new char[strlen(o.estado_) + 1];
      comando_ = new char[strlen(o.comando_) + 1];

      strcpy(fecha_,o.fecha_);
      strcpy(hora_,o.hora_);
      strcpy(user_,o.user_);
      strcpy(evento_,o.evento_);
      strcpy(objeto_,o.objeto_);
      strcpy(estado_,o.estado_);
      strcpy(comando_, o.comando_);
      
      id_ = o.id_;
      privilegio_ = o.privilegio_;
    
      return *this; // creo que funciona
  }

  // METODOS
  void imprimir() {
      cout << "Fecha: " << this->fecha_ << "    |   Hora: " << this->hora_ << endl;
      cout << "User: " << this->user_ << "      |   Evento: " << this->evento_ << endl;
      cout << "Objeto: " << this->objeto_ << "      |   Estado: " << this->estado_ << endl;
      cout << "Comando: " << this->comando_ << "        |   ID: " << this->id_ << "     |   Privilegio: " << this->privilegio_ << endl;
  }
  
};
