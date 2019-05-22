/*
 ============================================================================
 Name        : Memoria.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Memoria
 ============================================================================
 */

#include "Memoria.h"

void mostrarPathSegmentos(){
	segmento_t * segmentoAux;

	for(int i=0;i<list_size(tablaSegmentos.listaSegmentos);++i){
		segmentoAux= (segmento_t *)list_get(tablaSegmentos.listaSegmentos, i);
		printf("EL PATH DEL SEGMENTO ES: %s\n",obtenerPath(segmentoAux));

	}

}

//TODO: cada printf en rojo que indique el error se debe poner en el log

int main(void) {
	//Se hacen las configuraciones iniciales para log y config
	if(configuracion_inicial() == EXIT_FAILURE){
			printf(RED"Memoria.c: main: no se pudo generar la configuracion inicial"STD"\n");
			return EXIT_FAILURE;
		}
	mostrar_por_pantalla_config();
/*
	if(realizarHandshake()==EXIT_FAILURE){
		printf(RED"Memoria.c: main: no se pudo inicializar la memoria principal"STD"\n");
				return EXIT_FAILURE;
	}
*/
	tamanioValue=4;
	
	pathLFS= malloc(strlen("/puntoDeMontajeQueMeDaJuanEnElHandshake/")*sizeof(char)+1);
	strcpy(pathLFS,"/puntoDeMontajeQueMeDaJuanEnElHandshake/");


	//Habilita el server y queda en modo en listen / * Inicializar la memoria principal
	if(inicializar_memoriaPrincipal()==EXIT_FAILURE){
		printf(RED"Memoria.c: main: no se pudo inicializar la memoria principal"STD"\n");
		return EXIT_FAILURE;
	}
	printf("Memoria Inicializada correctamente\n");


	//TODO:GOSSIPING

	//FUNCIONES PARA TEST DE SELECT
	memoriaConUnSegmentoYUnaPagina();

	//mostrarContenidoMemoria();
	mostrarPathSegmentos();

	//Inicio consola

	if(iniciar_consola() == EXIT_FAILURE){
			printf(RED"Memoria.c: main: no se pudo levantar la consola"STD"\n");
			return EXIT_FAILURE;
	}

	pthread_join(idConsola,NULL);


/*
	int miSocket = enable_server(fconfig.ip, fconfig.puerto);
	log_info(logger_invisible, "Servidor encendido, esperando conexiones");
	threadConnection(miSocket, connection_handler);
*/

	//TODO: liberar todo
	if(memoriaPrincipal.memoria!=NULL)
		free(memoriaPrincipal.memoria);
	config_destroy(configFile);
	log_destroy(logger_invisible);
	log_destroy(logger_visible);
}


int realizarHandshake(void){
	int lfsSocket = conectarLFS();
	tamanioValue = handshakeLFS(lfsSocket);
	printf("TAMAÑO_VALUE= %d\n", tamanioValue);
	return EXIT_SUCCESS;
}


int handshakeLFS(int socketLFS){
	send_msg(socketLFS, TEXTO_PLANO, "handshake");

    TipoDeMensaje tipo;
    char *tamanio = recv_msg(socketLFS, &tipo);

	if(tipo == COMANDO)
		printf("Handshake falló. No se recibió el tamaño del value.\n");
	if(tipo == TEXTO_PLANO)
		printf("Handshake exitoso. Se recibió el tamaño del value, es: %d\n", *tamanio);

	return *tamanio;
}


int conectarLFS(){
	//Obtiene el socket por el cual se va a conectar al LFS como cliente / * Conectarse al proceso File System
	int socket = connect_to_server(fconfig.ip_fileSystem, fconfig.puerto_fileSystem);
	if(socket == EXIT_FAILURE){
		log_error(logger_invisible, "El LFS no está levantado. Cerrar la Memoria, levantar el LFS y volver a levantar la Memoria");
		return EXIT_FAILURE;
	}
	log_error(logger_invisible, "Conectado al LFS. Iniciando Handshake.");

	return socket;
}

char* obtenerPath(segmento_t* segmento){
		return segmento->pathTabla;
	}

void mostrarContenidoMemoria(){

}




void asignarPathASegmento(segmento_t * segmentoANombrar,char* nombreTabla){
	segmentoANombrar->pathTabla=malloc(sizeof(char)*(strlen(pathLFS)+strlen(nombreTabla))+1);
	strcpy(segmentoANombrar->pathTabla,pathLFS);
	strcat(segmentoANombrar->pathTabla,nombreTabla);
}

pagina_t* crearPaginaEnTabla(tabla_de_paginas_t *tablaDondeSeEncuentra){
	pagina_t *paginaACrear= malloc(sizeof(pagina_t));
	paginaACrear->countUso=0;
	paginaACrear->flagModificado=0;
	paginaACrear->numeroPagina=(list_size(tablaDondeSeEncuentra->paginas))+1;
	list_add(tablaDondeSeEncuentra->paginas, (pagina_t*) paginaACrear);
	return paginaACrear;
}

marco_t *crearMarcoEnMemoria(timestamp_t timestamp,uint16_t key, char* value, int * limiteMarco){

	*limiteMarco=sizeof(timestamp_t)+sizeof(uint16_t)+(sizeof(char)*strlen(value)+1);

	marco_t* baseMarco=memcpy(memoriaPrincipal.memoria+memoriaPrincipal.index,&timestamp,sizeof(timestamp_t));
	memoriaPrincipal.index+=sizeof(timestamp_t);

	memcpy(memoriaPrincipal.memoria+memoriaPrincipal.index,&key,sizeof(uint16_t));
	memoriaPrincipal.index+=sizeof(uint16_t);

	memcpy(memoriaPrincipal.memoria+memoriaPrincipal.index,value,sizeof(char)*strlen(value)+1);
	memoriaPrincipal.index+=sizeof(char)*strlen(value)+1;

	return baseMarco;
}

void memoriaConUnSegmentoYUnaPagina(void){

	pagina_t * paginaEjemplo;
	//Crear un segmento
	segmento_t* segmentoEjemplo=malloc(sizeof(segmento_t));

	//Asignar path determinado
	asignarPathASegmento(segmentoEjemplo,"tablaEjemplo");

	//Crear su tabla de paginas
	segmentoEjemplo->tablaPaginas=malloc(sizeof(tabla_de_paginas_t));
	segmentoEjemplo->tablaPaginas->paginas=list_create();
	//Crear pagina

	paginaEjemplo=crearPaginaEnTabla(segmentoEjemplo->tablaPaginas);

	//Preparo un marco de EJEMPLO
	paginaEjemplo->baseMarco=crearMarcoEnMemoria(getCurrentTime(),1, "Carlos", &(paginaEjemplo->limiteMarco));

	//Agregar a tabla de segmentos
	list_add(tablaSegmentos.listaSegmentos, (segmento_t*) segmentoEjemplo);

}





int inicializar_memoriaPrincipal(){
	int tamanioMemoria=atoi(fconfig.tamanio_memoria);
	memoriaPrincipal.index=0;
	memoriaPrincipal.cantMaxPaginas= tamanioMemoria/ sizeof(marco_t);

	memoriaPrincipal.memoria = malloc(tamanioMemoria);
	if(!memoriaPrincipal.memoria)
			return EXIT_FAILURE;

	tablaSegmentos.listaSegmentos=list_create();

	return EXIT_SUCCESS;
}


int iniciar_consola(){
	if(pthread_create(&idConsola, NULL, recibir_comandos, NULL)){
		printf(RED"Memoria.c: iniciar_consola: fallo la creacion de la consola"STD"\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


int configuracion_inicial(){

	logger_visible = iniciar_logger(true);
	if(logger_visible == NULL){
		printf(RED"Memoria.c: configuracion_inicial: error en 'logger_visible = iniciar_logger(true);'"STD"\n");
		return EXIT_FAILURE;
	}

	logger_invisible = iniciar_logger(false);
	if(logger_visible == NULL){
		printf(RED"Memoria.c: configuracion_inicial: error en 'logger_invisible = iniciar_logger(false);'"STD"\n");
		return EXIT_FAILURE;
	}


	if(inicializar_configs()==EXIT_FAILURE){
		printf(RED"Memoria.c: configuracion_inicial: error en el archivo 'Memoria.config'"STD"\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


int inicializar_configs() {
	configFile = config_create(STANDARD_PATH_MEMORIA_CONFIG);

	if(configFile == NULL){
		printf("Memoria.c: extraer_data_config: no se encontro el archivo 'Memoria.config'. Deberia estar junto al ejecutable");
		return EXIT_FAILURE;
	}

	//Config_datos_fijos
	extraer_data_fija_config();

	//Config_datos_variables
	vconfig.retardoMemoria = extraer_retardo_memoria;
	vconfig.retardoFS = extraer_retardo_FS;
	vconfig.retardoJOURNAL = extraer_retardo_JOURNAL;
	vconfig.retardoGossiping=extraer_retardo_Gossiping;
/*
	if(vconfig.quantum() <= 0)
		log_error(logger_error, "Kernel.c: extraer_data_config: (Warning) el quantum con valores menores o iguales a 0 genera comportamiento indefinido");
*/
	//TODO: Si yo hago un get de un valor que en el config no existe, va a tirar core dump. Arreglar eso.
	//La inversa no pasa nada, o sea , si agrego cosas al config y no les hago get aca no pasa nada

	//TODO: hacer que algunas se ajusten en tiempo real
	return EXIT_SUCCESS;
}

t_log* iniciar_logger(bool visible) {
	return log_create("Memoria.log", "Memoria", visible, LOG_LEVEL_INFO);
}

void extraer_data_fija_config() {
	fconfig.ip = config_get_string_value(configFile, "IP");
	fconfig.puerto = config_get_string_value(configFile, "PUERTO");
	fconfig.ip_fileSystem = config_get_string_value(configFile, "IP_FS");
	fconfig.puerto_fileSystem = config_get_string_value(configFile, "PUERTO_FS");
	fconfig.ip_seeds = config_get_string_value(configFile, "IP_SEEDS");
	fconfig.puerto_seeds = config_get_string_value(configFile, "PUERTO_SEEDS");
	fconfig.tamanio_memoria = config_get_string_value(configFile, "TAM_MEM");
	fconfig.numero_memoria = config_get_string_value(configFile, "MEMORY_NUMBER");
}



int extraer_retardo_memoria(){
	t_config *tmpConfigFile = config_create(STANDARD_PATH_MEMORIA_CONFIG);
	int res = config_get_int_value(tmpConfigFile, "RETARDO_MEM");
	config_destroy(tmpConfigFile);
	return res;
}

int extraer_retardo_FS(){
	t_config *tmpConfigFile = config_create(STANDARD_PATH_MEMORIA_CONFIG);
	int res = config_get_int_value(tmpConfigFile, "RETARDO_FS");
	config_destroy(tmpConfigFile);
	return res;
}
int extraer_retardo_JOURNAL(){
	t_config *tmpConfigFile = config_create(STANDARD_PATH_MEMORIA_CONFIG);
	int res = config_get_int_value(tmpConfigFile, "RETARDO_JOURNAL");
	config_destroy(tmpConfigFile);
	return res;
}
int extraer_retardo_Gossiping(){
	t_config *tmpConfigFile = config_create(STANDARD_PATH_MEMORIA_CONFIG);
	int res = config_get_int_value(tmpConfigFile, "RETARDO_GOSSIPING");
	config_destroy(tmpConfigFile);
	return res;
}

void mostrar_por_pantalla_config() {
	log_info(logger_visible, "IP=%s", fconfig.ip);
	log_info(logger_visible, "PUERTO=%s", fconfig.puerto);
	log_info(logger_visible, "IP_FS=%s",fconfig.ip_fileSystem);
	log_info(logger_visible, "PUERTO_FS=%s", fconfig.puerto_fileSystem);
	log_info(logger_visible, "IP_SEEDS=%s", fconfig.ip_seeds);
	log_info(logger_visible, "PUERTO_SEEDS=%s", fconfig.puerto_seeds);
	log_info(logger_visible, "RETARDO_MEM=%d", vconfig.retardoMemoria());
	log_info(logger_visible, "RETARDO_FS=%d", vconfig.retardoFS());
	log_info(logger_visible, "TAM_MEM=%s", fconfig.tamanio_memoria);
	log_info(logger_visible, "RETARDO_JOURNAL=%d", vconfig.retardoJOURNAL());
	log_info(logger_visible, "RETARDO_GOSSIPING=%d", vconfig.retardoGossiping());
	log_info(logger_visible, "MEMORY_NUMBER=%s", fconfig.numero_memoria);
}

void *connection_handler(void *nSocket){
    int socket = *(int*)nSocket;
    TipoDeMensaje tipo;
    char *resultado = recv_msg(socket, &tipo);

    //Es importante realizar este chequeo devolviendo EXIT_FAILURE
	if(resultado == NULL){
		return NULL;
	}

	printf("Hemos recibido algo!\n");

	if(tipo == COMANDO)
		comando_mostrar(parsear_comando(resultado));//ejecutarOperacion
	if(tipo == TEXTO_PLANO)
		printf("%s\n", resultado);


	//Podríamos meter un counter y que cada X mensajes recibidos corra el gossiping
	send_msg(socket, COMANDO, resultado);

	if(resultado != NULL)
		free(resultado);

	return NULL;
}

int ejecutarOperacion(char* input){ //TODO: TIPO de retorno Resultado
	Comando *parsed = malloc(sizeof(Comando));
	*parsed = parsear_comando(input);

	//TODO: funciones pasandole userInput y parsed por si necesito enviar algo o usar algun dato parseado

	if(parsed->valido){
		switch(parsed->keyword){
			case SELECT:
				selectAPI(input,*parsed);
				break;
			case INSERT:
				insertAPI(input,*parsed);
				break;
			case CREATE:
			case DESCRIBE:
			case DROP:
			case JOURNAL:
				printf("Entro un comando\n");
				break;
			default:
				fprintf(stderr, RED"No se pude interpretar el enum: %d"STD"\n", parsed->keyword);
		}

		destruir_operacion(*parsed);
	}else{
		fprintf(stderr, RED"La request no es valida"STD"\n");
	}
	return EXIT_SUCCESS; //MOMENTANEO
}
