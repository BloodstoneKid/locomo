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

int semaforo[21],buzon;
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
	for(int i=0; i<21; i++){
		semctl(semaforo[i],0,IPC_RMID);
	}
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
        for(int i=0; i<21; i++){
		semctl(semaforo[i],0,IPC_RMID);
	}
        msgctl(buzon,IPC_RMID,NULL);
}

int localiza(int x, int y){
	switch(x){
		case 0: if(y==0) return 0;
			else if(y==9) return 11;
			else if(y==12) return 13;
			break;
		case 16:if(y==0) return 1;
			else if(y==4) return 5;
			else if(y==7) return 6;
			else if(y==9) return 12;
			else if(y==12) return 14;
			else if(y==16) return 17;
			break;
		case 36:if(y==0) return 2;
			else if(y==7) return 7;
			else if(y==12) return 15;
			else if(y==16) return 18;
			break;
		case 54:if(y==0) return 3;
			else if(y==7) return 8;
			else if(y==12) return 16;
			else if(y==16) return 19;
			break;
		case 68:if(y==0) return 4;
			else if(y==7) return 9;
			else if(y==16) return 20;
			break;
		case 74: return 10;
			break;
		default: return -1;
	}
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
		for(int i=0; i<21; i++){
			semaforo[i] = semget(clave,1,IPC_CREAT | 0600);
            		if(semaforo[i]==-1) perror("crear semaforo");
		}
		buzon = msgget(clave,IPC_CREAT | 0600);
            if(buzon==-1) perror("crear buzon");
            
			
			LOMO_inicio(*argv[1],semaforo[0],buzon,LOGIN1,LOGIN2);
	struct mensaje msg;
            for(int i=0; i<nTrenes; i++){
            if(!fork()){
                msg.tipo=TIPO_TRENNUEVO;
                if(msgsnd(buzon,&msg,sizeof(msg)-sizeof(long),IPC_NOWAIT)!=0) perror("buzon snd 1");
                if(msgrcv(buzon,&msg,sizeof(msg)-sizeof(long),TIPO_RESPTRENNUEVO,1)==0) perror("buzon rcv 1");
                
                tren[i].ntren=msg.tren;
                msg.tipo=TIPO_GUARDAPID;
                msg.tren=getpid();
                if(msgsnd(buzon,&msg,sizeof(msg)-sizeof(long),IPC_NOWAIT)!=0) perror("buzon snd 2");


            }else{
                if(msgrcv(buzon,&msg,sizeof(msg)-sizeof(long),TIPO_GUARDAPID,1)==0) perror("buzon rcv 2");
                tren[i].pid=msg.tren;
            }
            }
                while(1){
                for(int i=0; i<nTrenes; i++){
                msg.tipo=TIPO_PETAVANCE;
                msg.tren = tren[i].ntren;
                if(msgsnd(buzon,&msg,sizeof(msg)-sizeof(long),IPC_NOWAIT)!=0) perror("buzon snd 3");
                if(msgrcv(buzon,&msg,sizeof(msg)-sizeof(long),TIPO_RESPPETAVANCETREN0+msg.tren,1)==0) perror("buzon rcv 3");
                int oldY = msg.y;

                msg.tipo=TIPO_AVANCE;
                if(msgsnd(buzon,&msg,sizeof(msg)-sizeof(long),IPC_NOWAIT)!=0) perror("buzon snd 4");
                if(msgrcv(buzon,&msg,sizeof(msg)-sizeof(long),TIPO_RESPAVANCETREN0+msg.tren,1)==0) perror("buzon rcv 4");

		/*int numsem = localiza(msg.x,msg.y);
		if(numsem!=-1) wait(&semaforo[numsem]);*/
                LOMO_espera(oldY,msg.y);
                sleep(1);
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

