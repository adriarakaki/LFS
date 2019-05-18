/*
 * Consola.h
 *
 *  Created on: 7 may. 2019
 *      Author: facundosalerno
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_
#define MAX_BUFFER_SIZE_FOR_LQL_LINE 100

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include "Kernel.h"

typedef enum PCB_DataType_e PCB_DataType;

//FUNCIONES
void *recibir_comandos(void *);
int new_lql(char* path);
int new_comando(PCB_DataType tipo, char *data);

void funcion_loca_de_testeo_de_concurrencia(void);

#endif /* CONSOLA_H_ */
