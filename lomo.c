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
#include <sys/mman.h>
#include "lomo.h"

#define TIPO_GUARDAPID 400
#define TIPO_SALIENDO 500

#define LOGIN1 "i6687129"
#define LOGIN2 "i1080834"


int semaforo[2],buzon;
int nTrenes, pids[100], idTren;
struct sembuf sops;
union semun{
    int val;
    struct semid_ds *buf;
    __u_short *array;
};

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
    LOMO_fin();
	for(int i=0; i<2; i++){
		semctl(semaforo[i],0,IPC_RMID);
	}
	msgctl(buzon,IPC_RMID,NULL);
    for(int i=0; i<nTrenes; i++){
        kill(pids[i],SIGTERM);
    }
    printf("Cerrados");
}	

void mata(){
    for(int i=0; i<nTrenes; i++){
        kill(pids[i],SIGTERM);
    }
    for(int i=0; i<2; i++){
        semctl(semaforo[i],0,IPC_RMID);
	}
    msgctl(buzon,IPC_RMID,NULL);
}

int localiza(int x, int y){
	if(x==0){
        if(y==9) return 11;
        else if(y==12) return 13;
        else return -1;
    }
	else if(x==16){
        if(y==0) return 1;
        else if(y==4) return 5;
        else if(y==7) return 6;
        else if(y==9) return 12;
        else if(y==12) return 14;
        else if(y==16) return 17;
        else return -1;
    }
	else if(x==36){
        if(y==0) return 2;
        else if(y==7) return 7;
        else if(y==12) return 15;
        else if(y==16) return 18;
        else return -1;
    }
	else if(x==54){
        if(y==0) return 3;
        else if(y==7) return 8;
        else if(y==12) return 16;
        else if(y==16) return 19;
        else return -1;
    }
	else if(x==68){
        if(y==0) return 4;
        else if(y==7) return 9;
        else if(y==16) return 20;
        else return -1;
    }
	else if(x==74){
        if(y==7) return 10;
        else return -1;
    }
	else{ return -1; }
}


int w(int semid, int semnum){
    sops.sem_op=-1;
    sops.sem_flg=0;
    sops.sem_num=semnum;
    int waiting = semop(semid,&sops,1); 
    if(waiting==-1){
        perror("Operacion wait al semaforo error");
        return -1;
    }

    return waiting;
}

int s(int semid, int semnum){
    sops.sem_op=1;
    sops.sem_flg=0;
    sops.sem_num=semnum;
    int signaled = semop(semid,&sops,1); 
    if(signaled==-1){
        perror("Operacion signal al semaforo error");
        return -1;
    }

    return signaled;
}

int main(int argc, char *argv[]){
	struct sigaction nuevo;
	nuevo.sa_handler = cierre;
	if(sigaction(SIGINT,&nuevo,NULL)==-1) perror("sigaction");

	if (argc==2 || argc==3){
		if(!strcmp(argv[1],"--mapa")){
			LOMO_generar_mapa(LOGIN1,LOGIN2);
		}else if(esNumero(argv[1]) && esNumero(argv[2])){
            nTrenes=atoi(argv[2]);
            if(nTrenes>100 || nTrenes<=0){
                errorHandler();
            }

            semaforo[0] = semget(IPC_PRIVATE,1,IPC_CREAT | 0600);
            semaforo[1] = semget(IPC_PRIVATE,23,IPC_CREAT | 0600);
            if(semaforo[0]==-1||semaforo[1]==-1) perror("crear semaforo");
            for(int i=0; i<23; i++){
                if(semctl(semaforo[1],i,SETVAL,1)==-1) perror("setval");
            }

            buzon = msgget(IPC_PRIVATE,IPC_CREAT | 0600);
            if(buzon==-1) perror("crear buzon");
                
                
            LOMO_inicio(*argv[1],semaforo[0],buzon,LOGIN1,LOGIN2);

            struct mensaje msg;
            for(int i=0; i<nTrenes; i++){
                switch(fork()){
                    case 0:
                        msg.tipo=TIPO_TRENNUEVO;
                        if(msgsnd(buzon,&msg,sizeof(msg)-sizeof(long),0)==-1) perror("buzon snd 1");
                        if(msgrcv(buzon,&msg,sizeof(msg),TIPO_RESPTRENNUEVO,1)==-1) perror("buzon rcv 1");
                        
                        idTren=msg.tren;
                        msg.tipo=TIPO_GUARDAPID;
                        msg.tren=getpid();
                        if(msgsnd(buzon,&msg,sizeof(msg)-sizeof(long),0)==-1) perror("buzon snd 2");
                        break;

                    case -1:
                        perror("fork");

                    default:
                        if(msgrcv(buzon,&msg,sizeof(msg),TIPO_GUARDAPID,1)==-1) perror("buzon rcv 2");
                        pids[i]=msg.tren;
                        break;
                }
            }

            int nuevoX, nuevoY, libreX, libreY,numsem,numsem2;
            bool iniciado = false;
            while(1){
                msg.tipo=TIPO_PETAVANCE;
                msg.tren = idTren;
                if(msgsnd(buzon,&msg,sizeof(msg)-sizeof(long),0)==-1) perror("buzon snd 3");
                if(msgrcv(buzon,&msg,sizeof(msg),TIPO_RESPPETAVANCETREN0+msg.tren,1)==-1) perror("buzon rcv 3");
                nuevoX = msg.x; nuevoY = msg.y;

		if (!iniciado){
                	w(semaforo[1], 22);
                	iniciado = true;
                }
                numsem = localiza(nuevoX,nuevoY);
                if(numsem!=-1) w(semaforo[1], numsem);

                msg.tipo=TIPO_AVANCE;
                if(msgsnd(buzon,&msg,sizeof(msg)-sizeof(long),IPC_NOWAIT)!=0) perror("buzon snd 4");
                if(msgrcv(buzon,&msg,sizeof(msg),TIPO_RESPAVANCETREN0+msg.tren,1)==0) perror("buzon rcv 4");
                libreX = msg.x; libreY = msg.y;
                numsem2 = localiza(libreX,libreY);
                if(numsem!=-1) s(semaforo[1],numsem);
                if(numsem2==1) s(semaforo[1],22);
                
                LOMO_espera(nuevoY,libreY);
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

