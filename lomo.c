#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/signal.h>
#include <signal.h>
#include <sys/unistd.h>
#include "lomo.h"

#define TIPO_GUARDAPID 400

#define LOGIN1 "i6687129"
#define LOGIN2 "i1080834"

struct trenes{
    int ntren;
    int pid;
};

int semaforo,buzon;
int nTrenes;
struct trenes tren[100];

bool esNumero(char number[]){
    		int i = 0;

    		for (; number[i] != 0; i++){
        		if (!isdigit(number[i]))
            			return false;
    		}
    		return true;
	}
	
void errorHandler(){
		printf("Modo de uso: \n1) --mapa para mostrar el HTML del mapa.\n 2) Un numero de retardo y un numero de trenes (entre 0 y 100)\n");
		exit(1);
	}
	
void cierre(int signum){
		semctl(semaforo,0,IPC_RMID);
		msgctl(buzon,IPC_RMID,NULL);
        for(int i=0; i<nTrenes; i++){
            kill(tren[i].pid,SIGTERM);
        }
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
            nTrenes=atoi(argv[2]);
            if(nTrenes>100 || nTrenes<=0){
                errorHandler();
            }

			key_t clave = ftok(argv[0],'G');
			semaforo = semget(clave,1,IPC_CREAT | 0600);
			buzon = msgget(clave,IPC_CREAT);
            
			
			LOMO_inicio(*argv[1],semaforo,buzon,LOGIN1,LOGIN2);
            for(int i=0; i<nTrenes; i++){
            if(!fork()){
                struct mensaje msg;
                msg.tipo=TIPO_TRENNUEVO;
                msgsnd(buzon,&msg,sizeof(struct mensaje),IPC_NOWAIT);
                msgrcv(buzon,&msg,sizeof(struct mensaje),TIPO_RESPTRENNUEVO,1);
                tren[i].ntren=msg.tren;
                printf("\n%d",tren[i].ntren);
                msg.tipo=TIPO_GUARDAPID;
                msg.tren=getpid();
                msgsnd(buzon,&msg,sizeof(struct mensaje),IPC_NOWAIT);
            }else{
                struct mensaje msg;
                msgrcv(buzon,&msg,sizeof(struct mensaje),TIPO_GUARDAPID,1);
                tren[i].pid=msg.tren;
            }
            }

            LOMO_fin();
            for(int i=0; i<nTrenes; i++){
                kill(tren[i].pid,SIGTERM);
            }
			semctl(semaforo,0,IPC_RMID);
			msgctl(buzon,IPC_RMID,NULL);
		}else{
			errorHandler();
		}
	}else{
		errorHandler();
	}
}

