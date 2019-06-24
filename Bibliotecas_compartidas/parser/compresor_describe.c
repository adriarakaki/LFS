/*
 * compresor_describe.c
 *
 *  Created on: 20 jun. 2019
 *      Author: facusalerno
 */

#include "compresor_describe.h"

void concatenar_tabla(char **source, char *tabla, char* consistencia, int numeroParticiones, int tiempoEntreCompactacion){
	if(!(string_equals_ignore_case(consistencia, "SC")) && !(string_equals_ignore_case(consistencia, "HSC")) && !(string_equals_ignore_case(consistencia, "EC")))
		return;
	if(*source == NULL)
			 *source = string_from_format("%s", tabla);
		else
			string_append(source, tabla);
	string_append(source, ";");
	string_append(source, consistencia);
	string_append(source, ";");
	string_append(source, string_from_format("%d", numeroParticiones));
	string_append(source, ";");
	string_append(source, string_from_format("%d", tiempoEntreCompactacion));
	string_append(source, ";");
}



char **descomprimir_describe(char *compresion){
	return string_split(compresion, ";");
}



void destruir_split_tablas(char **split){
	if(split){
		string_iterate_lines(split, (void *)free);
		free(split);
	}
}
