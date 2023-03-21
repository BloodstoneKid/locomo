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

int semaforo[2],buzon;
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
		semctl(semaforo[0],0,IPC_RMID);
		semctl(semaforo[1],0,IPC_RMID);
		msgctl(buzon,IPC_RMID,NULL);
        for(int i=0; i<nTrenes; i++){
            kill(tren[i].pid,SIGTERM);
        }
		printf("Cerrados");
	}	

void mata(){
        for(int i=0; i<nTrenes; i++){
            kill(tren[i].pid,SIGTERM);
        }
        semctl(semaforo[0],0,IPC_RMID);
        msgctl(buzon,IPC_RMID,NULL);
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
			semaforo[0] = semget(clave,1,IPC_CREAT | 0600);
            if(semaforo[0]==-1) perror("crear semaforo");
			buzon = msgget(clave,IPC_CREAT | 0600);
            if(buzon==-1) perror("crear buzon");
            
			
			LOMO_inicio(*argv[1],semaforo[0],buzon,LOGIN1,LOGIN2);
            for(int i=0; i<nTrenes; i++){
            if(!fork()){
                struct mensaje msg;
                msg.tipo=TIPO_TRENNUEVO;
                if(msgsnd(buzon,&msg,sizeof(msg)-sizeof(long),IPC_NOWAIT)!=0) perror("buzon snd 1");
                if(msgrcv(buzon,&msg,sizeof(msg)-sizeof(long),TIPO_RESPTRENNUEVO,1)!=0) perror("buzon rcv 1");
                
                tren[i].ntren=msg.tren;
                printf("\n%d\n",tren[i].ntren);
                msg.tipo=TIPO_GUARDAPID;
                msg.tren=getpid();
                if(msgsnd(buzon,&msg,sizeof(msg)-sizeof(long),IPC_NOWAIT)!=0) perror("buzon snd 2");
                msg.tren = tren[i].ntren;


                while(1){
                msg.tipo=TIPO_PETAVANCE;
                if(msgsnd(buzon,&msg,sizeof(msg)-sizeof(long),IPC_NOWAIT)!=0) perror("buzon snd 3");
                if(msgrcv(buzon,&msg,sizeof(msg)-sizeof(long),TIPO_RESPPETAVANCETREN0,1)!=0) perror("buzon rcv 3");

                int oldY = msg.y;
                msg.tipo=TIPO_AVANCE;
                if( msgsnd(buzon,&msg,sizeof(msg)-sizeof(long),IPC_NOWAIT)!=0) perror("buzon snd 4");
                if(msgrcv(buzon,&msg,sizeof(msg)-sizeof(long),TIPO_RESPAVANCETREN0,1)!=0) perror("buzon rcv 4");

                LOMO_espera(oldY,msg.y);
                sleep(1);
                }


            }else{
                struct mensaje msg;
                if(msgrcv(buzon,&msg,sizeof(msg)-sizeof(long),TIPO_GUARDAPID,1)!=0) perror("buzon rcv 2");
                tren[i].pid=msg.tren;
            }
            }
            LOMO_fin();
		}else{
			errorHandler();
		}
	}else{
		errorHandler();
	}
    mata();
}

