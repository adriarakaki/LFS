/*
 * Memoria.h
 *
 *  Created on: 13 abr. 2019
 *      Author:	fdalmaup
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <readline/readline.h>
#include <cliente/cliente.h>
#include <parser/parser_comando.h>
#include <server_multithread/server_multithread.h>
#include <pthread.h>
#include <semaphore.h>

#include "Consola.h"
#include "APIMemoria.h"

#define RED "\x1b[31m"
#define STD "\x1b[0m"

//ESTRUCTURAS
typedef struct{
	char *ip;
	char *puerto;
	char *ip_fileSystem;
	char *puerto_fileSystem;
	char *ip_seeds;
	char *puerto_seeds;
	char *tamanio_memoria;
	char *numero_memoria;
}Config_final_data;

//Estructuras de memoria

typedef unsigned long long timestamp_t;
typedef struct{
	uint16_t key;
	timestamp_t timestamp;
	char* value;
}marco;  		// puede tener un __attribute__((packed, aligned(1))) para evitar el padding

typedef struct{
	t_list* paginas;
}tabla_de_paginas;

typedef struct{
	int numeroPagina;
	bool flagModificado;
	marco* marco;
	timestamp_t timestampUso;
}pagina;

typedef struct{
	t_list* listaSegmentos;
}tabla_de_segmentos;

typedef struct{
	char* pathTabla;
	tabla_de_paginas* tablaPaginas;
}segmento;

//Bloque de Memoria

typedef struct{
	void* memoria;
	int index;		//Indica nro de bytes ocupados
	int cantMaxPaginas;
}memoria_principal;

//GLOBALES

/*Las de log y config son globales para poder acceder a ellos desde cualquier lado, pudiendo leer del config en tiempo de ejecucion y escribir en los logs sin
* pasarlos por parametro
*/
t_log* logger_visible;
t_log* logger_invisible;
t_config* configFile;
Config_final_data config;

pthread_t idConsola;
tabla_de_segmentos tablaSegmentos;
memoria_principal memoriaPrincipal;

//FUNCIONES
int configuracion_inicial();
t_log* iniciar_logger(bool);
t_config* leer_config(void);
void extraer_data_config(Config_final_data *, t_config* );
void ver_config(Config_final_data *config, t_log* logger_visible);
int conectarLFS(Config_final_data *config, t_log* logger_invisible);
int handshakeLFS(int socketLFS);
int threadConnection(int serverSocket, void *funcionThread);

int inicializar_memoriaPrincipal(Config_final_data config);

int iniciar_consola();

int ejecutarOperacion(char*);


#endif /* MEMORIA_H_ */
