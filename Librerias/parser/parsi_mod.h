/*
 * Copyright (C) 2018 Sistemas Operativos - UTN FRBA. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Adaptacion de 'parsi' del github de sisoputnfrba (https://github.com/sisoputnfrba/parsi) para los requisitos del tp 1c 2019
 */

#ifndef PARSER_H_
#define PARSER_H_

	#include <stdlib.h>
	#include <stdio.h>
	#include <stdbool.h>
	#include <commons/string.h>

	typedef struct {
		bool valido;
		enum {
			SELECT,
			INSERT,
			CREATE,
			DESCRIBE,
			DROP,
			JOURNAL,
			ADDMEMORY,
			RUN
		} keyword;
		union {
			struct {
				char* nombreTabla;
				char* key;
			} SELECT;
			struct {
				char* nombreTabla;
				char* key;
				char* value;
				char* timestamp;
			} INSERT;
			struct {
				char* nombreTabla;
				char* tipoConsistencia;
				char* numeroParticiones;
				char* compactacionTime;
			} CREATE;
			struct {
				char* nombreTabla;
			} DESCRIBE;
			struct {
				char* nombreTabla;
			} DROP;
			struct {
			} JOURNAL;
			struct {
				char* numero;
				char* criterio;
			} ADDMEMORY;
			struct {
				char* path;
			} RUN;
		} argumentos;
		char** _raw; //Para uso de la liberación
	} Comando;

	/**
	* @NAME: parse
	* @DESC: interpreta una linea de un archivo ESI y
	*		 genera una estructura con el operador interpretado
	* @PARAMS:
	* 		line - Una linea de un archivo ESI
	*/
	Comando parse(char* line);
	
	/**
	* @NAME: destruir_operacion
	* @DESC: limpia la operacion generada por el parse
	* @PARAMS:
	* 		op - Una operacion obtenida del parse
	*/
	void destruir_operacion(Comando op);

#endif /* PARSER_H_ */
