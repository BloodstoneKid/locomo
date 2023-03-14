#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <signal.h>
#include "lomo.h"

int semaforo,buzon;
char *LOGIN1 = "i6687129";
char *LOGIN2 = "i1080834";

bool esNumero(char number[]){
    		int i = 0;

    		for (; number[i] != 0; i++){
        		if (!isdigit(number[i]))
            			return false;
    		}
    		return true;
	}
	
void errorHandler(){
		printf("Modo de uso: 1) --mapa para mostrar el HTML del mapa.\n 2) Un numero de retardo y un numero de trenes\n");
		exit(1);
	}
	
void cierre(int signum){
		semctl(semaforo,0,IPC_RMID);
		msgctl(buzon,IPC_RMID,NULL);
		printf("Cerrados");
	}	

int main(int argc, char *argv[]){
	struct sigaction nuevo;
	nuevo.sa_handler = cierre;
	sigaction(SIGINT,&nuevo,NULL);
	if (argc==2 || argc==3){
		if(!strcmp(argv[1],"--mapa")){
			LOMO_generar_mapa(LOGIN1,LOGIN2);
		}else if(esNumero(argv[1]) && esNumero(argv[2])){
			key_t clave = ftok(argv[0],'G');
			semaforo = semget(clave,1,IPC_CREAT | 0600);
			buzon = msgget(clave,IPC_CREAT);
			
			//LOMO_inicio(*argv[1],semaforo,buzon,LOGIN1,LOGIN2);
			semctl(semaforo,0,IPC_RMID);
			msgctl(buzon,IPC_RMID,NULL);
		}else{
			errorHandler();
		}
	}else{
		errorHandler();
	}
}

