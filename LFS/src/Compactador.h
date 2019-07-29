/*
 * Compactador.h
 *
 *  Created on: 24 jun. 2019
 *      Author: juanmaalt
 */

#ifndef COMPACTADOR_H_
#define COMPACTADOR_H_

#include "adhoc/FuncionesComp.h"
//#include "adhoc/FuncionesAPI.h"
#include <commons/config.h>


typedef struct dirent EntradaDirectorio;

/*GLOBALES*/

/*FUNCIONES*/
void* compactar(void* nombreTabla);
void bloquearTabla(char *tabla);
void desbloquearTabla(char *tabla);
void bloquearSelect(char *tabla);
void desbloquearSelect(char *tabla);
void esperarFinCompactacion(char *tabla);
void esperarFinCompactacionSelect(char *tabla);
void reiniciarSemaforos(char *tabla);


#endif /* COMPACTADOR_H_ */
