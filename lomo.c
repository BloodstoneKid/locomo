#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
//#include <lomo.h>

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

int main(int argc, char *argv[]){
	if (argc==2 || argc==3){
		if(!strcmp(argv[1],"--mapa")){
			printf("Mapa\n");
		}else if(esNumero(argv[1]) && esNumero(argv[2])){
			printf("Trenes\n");
		}else{
			errorHandler();
		}
	}else{
		errorHandler();
	}
}
