/*
 * Compactador.h
 *
 *  Created on: 24 jun. 2019
 *      Author: juanmaalt
 */

#ifndef COMPACTADOR_H_
#define COMPACTADOR_H_

#include "adhoc/FuncionesComp.h"
#include <commons/config.h>

/*GLOBALES*/

/*FUNCIONES*/
void* compactar(void* nombreTabla);
void waitSemaforoTabla(char *tabla);
void postSemaforoTabla(char *tabla);


#endif /* COMPACTADOR_H_ */
