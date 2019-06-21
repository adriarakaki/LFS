/*
 * Unidad_de_ejecucion.c
 *
 *  Created on: 8 may. 2019
 *      Author: utnso
 */

#include "Unidad_de_ejecucion.h"

//FUNCIONES: Privadas. No van en el header.
static int exec_file_lql(PCB *pcb);
static int exec_string_comando(PCB *pcb);
static int loggear_operacion(Operacion op);
static socket_t direccionar_request(char *request);
static socket_t comunicarse_con_memoria();
static socket_t comunicarse_con_memoria_principal();

void *exec(void *null){
	pthread_detach(pthread_self());
	for(;;){
		//La cpu por default esta disponible si esta en este wait
		sem_wait(&scriptEnReady);
		sem_wait(&extraerDeReadyDeAUno);
		PCB *pcb = seleccionar_siguiente();
		sem_post(&extraerDeReadyDeAUno);
		switch(pcb->tipo){
		case STRING_COMANDO:
			exec_string_comando(pcb);
			break;
		case FILE_LQL:
			exec_file_lql(pcb);
			break;
		}
	}
	return NULL;
}





static socket_t direccionar_request(char *request){
	Comando comando = parsear_comando(request);
	Memoria *memoria;
	switch(comando.keyword){//A esta altura ya nos aseguramos de que el comando habia sido valido
	case SELECT:
		memoria = determinar_memoria_para_tabla(comando.argumentos.SELECT.nombreTabla);
		break;
	case INSERT:
		memoria = determinar_memoria_para_tabla(comando.argumentos.INSERT.nombreTabla);
		break;
	case CREATE:
		memoria = determinar_memoria_para_tabla(comando.argumentos.CREATE.nombreTabla);
		break;
	case DESCRIBE:
		memoria = determinar_memoria_para_tabla(comando.argumentos.DESCRIBE.nombreTabla);
		break;
	case DROP:
		memoria = determinar_memoria_para_tabla(comando.argumentos.DROP.nombreTabla);
		break;
	default:
		return EXIT_FAILURE;
	}
	destruir_comando(comando);

	if(memoria == NULL){
		printf(YEL"Warning: se esta trabajando sin seleccion de criterios. La request es atendida por la memoria principal\n"STD);
		log_info(logger_invisible, "Warning: se esta trabajando sin seleccion de criterios. La request es atendida por la memoria principal");
		return comunicarse_con_memoria_principal();
	}else{
		return comunicarse_con_memoria(memoria);
	}
}


static socket_t comunicarse_con_memoria(Memoria *memoria){
	int socketServer;
	if((socketServer = connect_to_server(memoria->ip, memoria->puerto)) == EXIT_FAILURE){
		log_error(logger_error, "Planificador.c: comunicarse_con_memoria: error al conectarse al servidor memoria %s:%s", memoria->ip, memoria->puerto);
		log_error(logger_invisible, "Planificador.c: comunicarse_con_memoria: error al conectarse al servidor memoria %s:%s", memoria->ip, memoria->puerto);
		return EXIT_FAILURE;
	}
	log_info(logger_invisible, "Conectado a la memoria %s:%s", memoria->ip, memoria->puerto);
	return socketServer;
}


static socket_t comunicarse_con_memoria_principal(){
	int socketServer;
	if((socketServer = connect_to_server(fconfig.ip_memoria_principal, fconfig.puerto_memoria_principal)) == EXIT_FAILURE){
		log_error(logger_error, "Planificador.c: comunicarse_con_memoria: error al conectarse al servidor memoria %s:%s", fconfig.ip_memoria_principal, fconfig.puerto_memoria_principal);
		log_error(logger_invisible, "Planificador.c: comunicarse_con_memoria: error al conectarse al servidor memoria %s:%s", fconfig.ip_memoria_principal, fconfig.puerto_memoria_principal);
		return EXIT_FAILURE;
	}
	log_info(logger_invisible, "Conectado a la memoria %s:%s", fconfig.ip_memoria_principal, fconfig.puerto_memoria_principal);
	return socketServer;
}





static int exec_string_comando(PCB *pcb){
	int socketTarget = direccionar_request((char *)pcb->data);
	Operacion request;

	request.opCode = getNumber();
	request.TipoDeMensaje = COMANDO;
	request.Argumentos.COMANDO.comandoParseable = (char*)pcb->data;
	send_msg(socketTarget, request);

	request = recv_msg(socketTarget);
	loggear_operacion(request);

	destruir_operacion(request);
	free(pcb->data);
	free(pcb);
	close(socketTarget);
	return FINALIZO;
}





static int exec_file_lql(PCB *pcb){
	Operacion request;
	char buffer[MAX_BUFFER_SIZE_FOR_LQL_LINE];
	char *line = NULL;
	FILE *lql = (FILE *)pcb->data; //Como el FILE nunca se cerro, cada vez que entre, va a continuar donde se habia quedado
	int quantumBuffer = vconfig.quantum(); //Para hacer la llamada una sola vez por cada exec. No se actualiza el quantum en tiempo real, pero se actualiza cuando entra un nuevo script por que ya tiene el valor actualizado

	for(int i=1; i<=quantumBuffer; ++i){
		line = fgets(buffer, MAX_BUFFER_SIZE_FOR_LQL_LINE, lql);
		if(line == NULL || feof(lql)){
			printf("\n");
			fclose(lql);
			free(pcb);
			return FINALIZO;
		}
		int socketTarget = direccionar_request(line);
		request.opCode = getNumber();
		request.TipoDeMensaje = COMANDO;
		request.Argumentos.COMANDO.comandoParseable = line;
		send_msg(socketTarget, request);
		request = recv_msg(socketTarget);
		if(loggear_operacion(request) == INSTRUCCION_ERROR){
			fclose(lql);
			free(pcb);
			close(socketTarget);
			return FINALIZO;
		}
		close(socketTarget);
	}
	printf("\n");
	sem_wait(&meterEnReadyDeAUno);
	desalojar(pcb);
	sem_post(&meterEnReadyDeAUno);
	sem_post(&scriptEnReady); //Ya que se metio a la lista de vuelta
	simular_retardo();
	return DESALOJO;
}





static int loggear_operacion(Operacion op){
	switch(op.TipoDeMensaje){
	case TEXTO_PLANO:
		log_info(logger_visible,"CPU: %d | ID Operacion: %d | %s", process_get_thread_id(), op.opCode, op.Argumentos.TEXTO_PLANO.texto);
		log_info(logger_invisible,"CPU: %d | ID Operacion: %d | %s", process_get_thread_id(), op.opCode, op.Argumentos.TEXTO_PLANO.texto);
		return CONTINUAR;
	case COMANDO:
		log_info(logger_visible,"CPU: %d | ID Operacion: %d | %s", process_get_thread_id(), op.opCode, op.Argumentos.COMANDO.comandoParseable);
		log_info(logger_invisible,"CPU: %d | ID Operacion: %d | %s", process_get_thread_id(), op.opCode, op.Argumentos.COMANDO.comandoParseable);
		return CONTINUAR;
	case REGISTRO:
		log_info(logger_visible,"CPU: %d | ID Operacion: %d | Timestamp: %llu, Key: %d, Value: %s", process_get_thread_id(), op.opCode, op.Argumentos.REGISTRO.timestamp, op.Argumentos.REGISTRO.key, op.Argumentos.REGISTRO.value);
		log_info(logger_invisible,"CPU: %d | ID Operacion: %d | Timestamp: %llu, Key: %d, Value: %s", process_get_thread_id(), op.opCode, op.Argumentos.REGISTRO.timestamp, op.Argumentos.REGISTRO.key, op.Argumentos.REGISTRO.value);
		return CONTINUAR;
	case ERROR:
		log_error(logger_error,"CPU: %d | ID Operacion: %d | Abortando: %s", process_get_thread_id(), op.opCode, op.Argumentos.ERROR.mensajeError);
		log_error(logger_invisible,"CPU: %d | ID Operacion: %d | Abortando: %s", process_get_thread_id(), op.opCode, op.Argumentos.ERROR.mensajeError);
		return INSTRUCCION_ERROR;
	default:
		return 1;//TODO
	}
	return INSTRUCCION_ERROR;
}

