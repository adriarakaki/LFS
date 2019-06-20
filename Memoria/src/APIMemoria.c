/*
 * APIMemoria.c
 *
 *  Created on: 15 may. 2019
 *      Author: fdalmaup
 */
#define JournalTestNumber 3

#include "APIMemoria.h"

Operacion ejecutarOperacion(char* input, bool esDeConsola) {
	Comando *parsed = malloc(sizeof(Comando));
	Operacion retorno;
	*parsed = parsear_comando(input);
	usleep(vconfig.retardoMemoria() * 1000);
	if (parsed->valido) {
		switch (parsed->keyword) {
		case SELECT:
			retorno = selectAPI(input, *parsed);
			break;
		case INSERT:
			if ((strlen(parsed->argumentos.INSERT.value) + 1) > tamanioValue) {
				printf("TamValue teorico: %d\nTamValue real: %d\n",
						tamanioValue,
						(strlen(parsed->argumentos.INSERT.value) + 1));
				retorno.Argumentos.ERROR.mensajeError = string_from_format(
						"Error en el tamanio del value.");
				retorno.TipoDeMensaje = ERROR;
			} else {
				retorno = insertAPI(input, *parsed);
			}

			break;
		case CREATE:
			retorno = createAPI(input, *parsed);
			break;
		case DESCRIBE:
			break;
		case DROP:
			retorno = dropAPI(input, *parsed);
			break;
		case JOURNAL:
			retorno = journalAPI();
			break;
		default:

			fprintf(stderr, RED"No se pude interpretar el enum: %d"STD"\n",
					parsed->keyword);
		}

		destruir_comando(*parsed);
		free(parsed);
		free(input);
		return retorno;
	} else {
		fprintf(stderr, RED"La request no es valida"STD"\n");

		destruir_comando(*parsed);
		free(parsed);
	}

	retorno.TipoDeMensaje = ERROR;
	retorno.Argumentos.ERROR.mensajeError = string_from_format(
			"Error en la recepcion del resultado.");

	free(input);

	return retorno;
}

/*
 * La operación Select permite la obtención del valor de una key dentro de una tabla. Para esto, se utiliza la siguiente nomenclatura:
 SELECT [NOMBRE_TABLA] [KEY]
 Ej:
 SELECT TABLA1 3

 Esta operación incluye los siguientes pasos:
 1. Verifica si existe el segmento de la tabla solicitada y busca en las páginas del mismo si contiene key solicitada.
 Si la contiene, devuelve su valor y finaliza el proceso.

 2. Si no la contiene, envía la solicitud a FileSystem para obtener el valor solicitado y almacenarlo.

 3. Una vez obtenido se debe solicitar una nueva página libre para almacenar el mismo.

 En caso de no disponer de una página libre, se debe ejecutar el algoritmo de reemplazo y,
 en caso de no poder efectuarlo por estar la memoria full, ejecutar el Journal de la memoria.

 * */

Operacion selectAPI(char* input, Comando comando) {
	Operacion resultadoSelect;
	resultadoSelect.TipoDeMensaje = ERROR;

	segmento_t *segmentoSeleccionado = NULL;

	uint16_t keyBuscada = atoi(comando.argumentos.SELECT.key); //TODO: se verifica que la key sea numerica?

	registroTablaPag_t *registroBuscado = NULL;

	if (verificarExistenciaSegmento(comando.argumentos.SELECT.nombreTabla,&segmentoSeleccionado)) {
		if (contieneKey(segmentoSeleccionado, keyBuscada, &registroBuscado)) {

			resultadoSelect = tomarContenidoPagina(*registroBuscado);

			resultadoSelect.TipoDeMensaje = REGISTRO;
			return resultadoSelect;
		}else{
			printf(YEL"APIMemoria.c: select: no encontro la key. Enviar a LFS la request"STD"\n"); //TODO: meter en log
			//TODO: Enviar a LFS la request
			enviarRequestFS(input);

			resultadoSelect.TipoDeMensaje = REGISTRO;
			resultadoSelect=recibirRequestFS();
			//INSERTAR VALOR EN BLOQUE DE MEMORIA Y METER CREAR REGISTRO EN TABLA DE PAGINAS DEL SEGMENTO
			//Recibo el valor (el marcoRecibido/registro entero ya parseado al ser recibido como un char*)
			//
			//solicitarPagina(segmentoSeleccionado,marcoRecibido);

			return resultadoSelect;

		}

	}/*else{ //NO EXISTE EL SEGMENTO
		printf(YEL"APIMemoria.c: select: no se encontro el path. Enviar a LFS la request"STD"\n");
		enviarRequestFS(input);

		resultadoSelect.TipoDeMensaje = REGISTRO;
		resultadoSelect=recibirRequestFS();



	 }*/

	resultadoSelect.Argumentos.ERROR.mensajeError = string_from_format(
			"NO SE HA ENCONTRADO LA KEY");
	return resultadoSelect;

}
/*
 Pasos:
 1. Verifica si existe el segmento de la tabla en la memoria principal.
 De existir, busca en sus páginas si contiene la key solicitada y de contenerla actualiza el valor insertando el Timestamp actual.
 En caso que no contenga la Key, se solicita una nueva página para almacenar la misma.
 Se deberá tener en cuenta que si no se disponen de páginas libres
 aplicar el algoritmo de reemplazo (LRU) y en caso de que la memoria se encuentre full iniciar el proceso Journal.

 2. En caso que no se encuentre el segmento en memoria principal, se creará y se agregará la nueva Key con el Timestamp actual,
 junto con el nombre de la tabla en el segmento. Para esto se debe generar el nuevo segmento y solicitar una nueva página
 (aplicando para este último la misma lógica que el punto anterior).

 Cabe destacar que la memoria no verifica la existencia de las tablas al momento de insertar nuevos valores. De esta forma,
 no tiene la necesidad de guardar la metadata del sistema en alguna estructura administrativa. En el caso que al momento de realizar el Journaling
 una tabla no exista, deberá informar por archivo de log esta situación, pero el proceso deberá actualizar correctamente las tablas que sí existen.

 Cabe aclarar que esta operatoria en ningún momento hace una solicitud directa al FileSystem, la misma deberá manejarse siempre
 dentro de la memoria principal.
 * */

// INSERT [NOMBRE_TABLA] [KEY] “[VALUE]”
//Ej:
//INSERT TABLA1 3 “Mi nombre es Lissandra”
Operacion insertAPI(char* input, Comando comando) {

	Operacion resultadoInsert;
	resultadoInsert.TipoDeMensaje = ERROR;

	segmento_t *segmentoSeleccionado = NULL;

	uint16_t keyBuscada = atoi(comando.argumentos.INSERT.key); //TODO: se verifica que la key sea numerica?

	registroTablaPag_t *registroBuscado = NULL;
	//marco_t *marcoBuscado=NULL;

	//Verifica si existe el segmento de la tabla en la memoria principal.
	if (verificarExistenciaSegmento(comando.argumentos.INSERT.nombreTabla,
			&segmentoSeleccionado)) {
		//Busca en sus páginas si contiene la key solicitada y de contenerla actualiza el valor insertando el Timestamp actual.
		if (contieneKey(segmentoSeleccionado, keyBuscada, &registroBuscado)) {

			//En los request solo se utilizarán las comillas (“”)
			//para enmascarar el Value que se envíe. No se proveerán request con comillas en otros puntos.

			//TODO: Funcion para actualizar key

			//remover_comillas(comando.argumentos.INSERT.value);

			void * direccionMarco = memoriaPrincipal.memoria
					+ memoriaPrincipal.tamanioMarco * registroBuscado->nroMarco;
			timestamp_t tsActualizado = getCurrentTime();

			memcpy(direccionMarco, &tsActualizado, sizeof(timestamp_t));
			strcpy(direccionMarco + sizeof(timestamp_t) + sizeof(uint16_t),
					comando.argumentos.INSERT.value);

			printf("Se realizo el INSERT\n");

			resultadoInsert.TipoDeMensaje = TEXTO_PLANO;
			resultadoInsert.Argumentos.TEXTO_PLANO.texto = string_from_format(
					"INSERT REALIZADO CON EXITO");

			//mostrarContenidoPagina(*registroBuscado); //Para ver lo que se inserto

			return resultadoInsert;

		} else {//No contiene la KEY, se solicita una nueva página para almacenar la misma.

			insertarPaginaDeSegmento(comando.argumentos.INSERT.value,
					keyBuscada, segmentoSeleccionado);

			resultadoInsert.TipoDeMensaje = TEXTO_PLANO;
			resultadoInsert.Argumentos.TEXTO_PLANO.texto = string_from_format(
					"INSERT REALIZADO CON EXITO");

			return resultadoInsert;
		}
	} else {	//NO EXISTE SEGMENTO
		/*
		 * se creará y se agregará la nueva Key con el Timestamp actual,
		 junto con el nombre de la tabla en el segmento. Para esto se debe generar el nuevo segmento y solicitar una nueva página
		 */

		//Crear un segmento
		segmento_t* segmentoNuevo = malloc(sizeof(segmento_t));

		//Asignar path determinado
		asignarPathASegmento(segmentoNuevo,
				comando.argumentos.INSERT.nombreTabla);

		//Crear su tabla de paginas
		segmentoNuevo->tablaPaginas = malloc(sizeof(tabla_de_paginas_t));
		segmentoNuevo->tablaPaginas->registrosPag = list_create();

		insertarPaginaDeSegmento(comando.argumentos.INSERT.value, keyBuscada,
				segmentoNuevo);

		//Agregar segmento Nuevo a tabla de segmentos
		list_add(tablaSegmentos.listaSegmentos, (segmento_t*) segmentoNuevo);

		resultadoInsert.TipoDeMensaje = TEXTO_PLANO;
		resultadoInsert.Argumentos.TEXTO_PLANO.texto = string_from_format(
				"INSERT REALIZADO CON EXITO");
		return resultadoInsert;
	}

}

int hayPaginaDisponible(void) {
	return queue_is_empty(memoriaPrincipal.marcosLibres) != true;
}

void insertarPaginaDeSegmento(char* value, uint16_t key, segmento_t * segmento) {
	if (hayPaginaDisponible()) {

		crearRegistroEnTabla(segmento->tablaPaginas,
				colocarPaginaEnMemoria(getCurrentTime(), key, value));

		printf("Se realizo el INSERT\n");

	} else {//aplicar el algoritmo de reemplazo (LRU) y en caso de que la memoria se encuentre full iniciar el proceso Journal.
		printf("HACER LRU Y SINO JOURNAL\n");
	}
}

/*
 CREATE [TABLA] [TIPO_CONSISTENCIA] [NUMERO_PARTICIONES] [COMPACTION_TIME]
 Ej:
 CREATE TABLA1 SC 4 60000
 Esta operación incluye los siguientes pasos:
 1.Se envía al FileSystem la operación para crear la tabla.
 2.Tanto si el FileSystem indica que la operación se realizó de forma exitosa o en caso de falla
 por tabla ya existente, continúa su ejecución normalmente.
 */


Operacion createAPI(char* input, Comando comando) {
	Operacion resultadoCreate;

	//Enviar al FS la operacion
	enviarRequestFS(input);

	//Lo que recibo del FS lo retorno

	resultadoCreate=recibirRequestFS();

	return resultadoCreate;

}

/*
 La operación Describe permite obtener la Metadata de una tabla en particular o de todas las tablas que el file system tenga.
 Para esto, se utiliza la siguiente nomenclatura:
 DESCRIBE [NOMBRE_TABLA]

 Esta operación consulta al FileSystem por la metadata de las tablas. Sirve principalmente para poder responder las solicitudes del Kernel.
 */
Operacion describeAPI() {
	//enviarRequestFS(input);

	//Lo que recibo del FS lo retorno

	//recibirRequestFS();
}

/*
 * 		La operación Drop permite la eliminación de una tabla del file system. Para esto, se utiliza la siguiente nomenclatura:
 DROP [NOMBRE_TABLA]

 Esta operación incluye los siguientes pasos:
 1. Verifica si existe un segmento de dicha tabla en la memoria principal y de haberlo libera dicho espacio.
 2. Informa al FileSystem dicha operación para que este último realice la operación adecuada.
 *
 * */
void removerSegmentoDeTabla(segmento_t* segmentoSeleccionado) {
	bool segmentoCoincidePath(void* comparado) {
		//printf(RED"pathSegmentoBuscado: %s\npathComparado: %s"STD"\n",pathSegmentoBuscado,obtenerPath((segmento_t*)comparado));
		if (strcmp(obtenerPath(segmentoSeleccionado),
				obtenerPath((segmento_t*) comparado))) {
			return false;
		}
		return true;
	}
	list_remove_by_condition(tablaSegmentos.listaSegmentos,
			segmentoCoincidePath);

}

Operacion dropAPI(char* input, Comando comando) {
	Operacion resultadoDrop;
	resultadoDrop.TipoDeMensaje = ERROR;

	segmento_t *segmentoSeleccionado = NULL;

	if (verificarExistenciaSegmento(comando.argumentos.DROP.nombreTabla,
			&segmentoSeleccionado)) {
		//EXISTE EL SEGMENTO
		//PROCEDE A LIBERAR EL ESPACIO

		//   SACO EL SEGMENTO DE LA TABLA DE SEGMENTOS
		removerSegmentoDeTabla(segmentoSeleccionado);

		//1. Tomo la tabla de paginas del segmento
		//2. Por cada registro de la tabla:
		//	 2.1. Busco el MCB correspondiente en la tabla administrativa de marcos
		//   y lo pusheo en la cola de marcosLibres

		//	 2.2 Elimino el registro de la tabla de paginas del segmento
		//3. Elimino la tabla de paginas
		//4. Elimino el segmento
		//5. Enviar request al FS

		liberarSegmento(segmentoSeleccionado);

		printf("Drop realizado\n");

		resultadoDrop.TipoDeMensaje = TEXTO_PLANO;
		resultadoDrop.Argumentos.TEXTO_PLANO.texto = string_from_format(
				"Drop realizado con exito");
	} else {
		resultadoDrop.TipoDeMensaje = ERROR;
		resultadoDrop.Argumentos.ERROR.mensajeError = string_from_format(
				"La tabla buscada no existe en memoria");
	}
	return resultadoDrop;

}
Operacion journalAPI(){
	Operacion resultadoJournal;
	char * input;
	//char*   string_from_format(const char* format, ...);
	//1. Por cada MCB en la listaAdminMarcos ver si tiene el flag de modificado
	//(si en vez de tener en esa lista, lo tengo en la tabla de paginas de cada segmento, ya tengo la tabla PENSAR )
	//	1.1. Si tiene modificado, armo un insert con todos sus datos (viendo de que tabla es) y lo mando al FS

	//  1.2. Si no esta modificado avanzo
	for(int i=0; i< JournalTestNumber; i++){


		//Enviar al FS la operacion

		resultadoJournal.TipoDeMensaje = COMANDO;
		//resultadoJournal.Argumentos.COMANDO.comandoParseable= string_from_format(input);
		input=string_from_format("INSERT tabla%d %d \"JOHN WILLY\" %llu",i,i*100, getCurrentTime());

		enviarRequestFS(input);



		resultadoJournal=recibirRequestFS();


	}
	return resultadoJournal;
}

bool verificarExistenciaSegmento(char* nombreTabla,
		segmento_t ** segmentoAVerificar) {
	char* pathSegmentoBuscado = malloc(
			sizeof(char) * (strlen(nombreTabla) + strlen(pathLFS)) + 1);

	strcpy(pathSegmentoBuscado, pathLFS);
	strcat(pathSegmentoBuscado, nombreTabla);

	//Recorro tabla de segmentos buscando pathSegmentoBuscado

	bool segmentoCoincidePath(void* comparado) {
		//printf(RED"pathSegmentoBuscado: %s\npathComparado: %s"STD"\n",pathSegmentoBuscado,obtenerPath((segmento_t*)comparado));
		if (strcmp(pathSegmentoBuscado, obtenerPath((segmento_t*) comparado))) {
			return false;
		}
		return true;
	}

	t_list* listaConSegmentoBuscado = list_filter(tablaSegmentos.listaSegmentos,
			segmentoCoincidePath);

	//Para ver que el path es el correcto

	//printf("El path del segmento buscado es: %s\n", pathSegmentoBuscado);
	free(pathSegmentoBuscado);

	if (list_is_empty(listaConSegmentoBuscado)) {
		list_destroy(listaConSegmentoBuscado);
		printf(GRN"APIMemoria.c: NO SE ENCONTRO EL SEGMENTO"STD"\n");
		return false;
	}
	//Tomo de la lista filtrada el segmento con el path coincidente

	*segmentoAVerificar = (segmento_t*) list_get(listaConSegmentoBuscado, 0);

	//Elimino la lista filtrada
	list_destroy(listaConSegmentoBuscado);

	return true;
}

//Busca en cada registro de la tabla de paginas el indice de marco y me fijo si coincide la key

//En vez de pasarle value le paso la operación que quiero hacer con value (modificarlo con insert o tomarlo con select)

bool contieneKey(segmento_t* segmentoElegido, uint16_t keyBuscada,
		registroTablaPag_t ** registroResultado) {
	//1. Tomo la tabla de paginas del segmento

	t_list * regsDelSegmentoElegido =
			segmentoElegido->tablaPaginas->registrosPag;

	//2. Por cada registro de la tabla, me meto en la memoria y tomo la key
	//3. En cada marco me fijo si la key que tiene == keyBuscada
	//3.1 En caso de que la key sea la keyBuscada tomo el valor del value, y countUso++
	//3.2 Si la key NO es la buscada sigo con el siguiente marco

	bool compararConPagina(void* registroAComparar) {
		uint16_t *keyPagina = malloc(sizeof(uint16_t));

		void* direccionPagina = memoriaPrincipal.memoria
				+ memoriaPrincipal.tamanioMarco
						* (((registroTablaPag_t*) registroAComparar)->nroMarco);
		memcpy(keyPagina, direccionPagina + sizeof(timestamp_t),
				sizeof(uint16_t));

		if (*keyPagina == keyBuscada) {
			//Aumento el uso de la pagina
			free(keyPagina);
			return true;
		}
		free(keyPagina);
		return false;
	}

	t_list* listaConRegistroBuscado = list_filter(regsDelSegmentoElegido,
			compararConPagina);

	if (list_is_empty(listaConRegistroBuscado)) {
		list_destroy(listaConRegistroBuscado);
		return false;
	}

	*registroResultado = (registroTablaPag_t*) list_remove(
			listaConRegistroBuscado, 0);
	list_destroy(listaConRegistroBuscado);

	return true;

}

Operacion tomarContenidoPagina(registroTablaPag_t registro) {

	Operacion resultadoRetorno;

	void * direccionMarco = memoriaPrincipal.memoria
			+ memoriaPrincipal.tamanioMarco * registro.nroMarco;
	/*timestamp_t timestamp;
	 uint16_t key;
	 */
	resultadoRetorno.Argumentos.REGISTRO.value = malloc(
			sizeof(char) * tamanioValue);

	memcpy(&resultadoRetorno.Argumentos.REGISTRO.timestamp, direccionMarco,
			sizeof(timestamp_t));

	memcpy(&resultadoRetorno.Argumentos.REGISTRO.key,
			direccionMarco + sizeof(timestamp_t), sizeof(uint16_t));

	strcpy(resultadoRetorno.Argumentos.REGISTRO.value,
			direccionMarco + sizeof(timestamp_t) + sizeof(uint16_t));

	//printf("Timestamp: %llu\nKey:%d\nValue: %s\n",timestamp,key,value);

	return resultadoRetorno;

}


void enviarRequestFS(char* input) {
	lfsSocket = conectarLFS(); //TODO: NO DEJARLA COMO GLOBAL

	Operacion request;
	request.TipoDeMensaje = COMANDO;

	request.Argumentos.COMANDO.comandoParseable = string_from_format(input);

	send_msg(lfsSocket, request);

	destruir_operacion(request);
}

Operacion recibirRequestFS(void) {
	Operacion resultado;
	resultado = recv_msg(lfsSocket);
	return resultado;
}
