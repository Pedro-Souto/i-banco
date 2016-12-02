/*
Projeto SO - exercicio 3, version 1
Sistemas Operativos, DEI/IST/ULisboa 2016-17
*/
#include "commandlinereader.h"
#include "contas.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_TRANSFERIR "transferir"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_ARG_SAIR_AGORA "agora"
#define COMANDO_SAIRTERMINAL "sair-terminal"

#define OP_LER_SALDO 	0
#define OP_CREDITAR  	1
#define OP_DEBITAR   	2
#define OP_SAIR      	3
#define OP_TRANSFERIR   4
#define OP_SIMULAR		5
#define OP_SAIR_AGORA   6
#define OP_SAIRTerminal 7

#define MAXARGS 4
#define BUFFER_SIZE 100


typedef struct
{
  int operacao;
  int idConta;
  int idContaDestino;
  int valor;
  int pid;
} comando_t;

void escreverPipe(comando_t cmd, char* filename, char* out_pipe);

int main (int argc, char** argv) {

char *args[MAXARGS + 1];
char buffer[BUFFER_SIZE];
comando_t cmd;

  /* Criar Pipe */
char out_pipe[256];
sprintf(out_pipe,"i-banco-pid%d",getpid());
if(mkfifo(out_pipe, S_IRUSR | S_IWUSR)==-1){
	printf("Error creating i-banco-pipe");
}
//signal(SIGUSR1,usrHandler);
printf("Bem-vinda/o ao terminal i-banco\n\n");

	while(1) {
    	int numargs;
    
    	numargs = readLineArguments(args, MAXARGS+1, buffer, BUFFER_SIZE);

	    if (numargs < 0 ||(numargs > 0 && (strcmp(args[0], COMANDO_SAIRTERMINAL) == 0))) {
	      printf("i-banco terminal vai terminar...\n");
	      printf("--\n");
	      
	      unlink(out_pipe);
	      unlink(argv[1]);
	      printf("(pipes fechados com sucesso)\n");
	      printf("--\n");
	      printf("i-banco terminou.\n");

	      exit(EXIT_SUCCESS);
    	} if (numargs < 0 ||(numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0))) {
	            /*Sair Agora*/
	        if (numargs>1 && strcmp(args[1], COMANDO_ARG_SAIR_AGORA) == 0){
	          cmd.operacao=OP_SAIR_AGORA;
	        }else{
	          cmd.operacao=OP_SAIR;
	        }
	        	cmd.pid=getpid();
	            escreverPipe(cmd,argv[1],out_pipe);
    	} else if (strcmp(args[0], COMANDO_LER_SALDO) == 0) {
	      if (numargs < 2) {
	        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_LER_SALDO);
	        continue;
	      }
	      cmd.operacao = OP_LER_SALDO;
	      cmd.idConta = atoi(args[1]);
	      cmd.pid = getpid();

	      escreverPipe(cmd,argv[1],out_pipe);
	    } else if (strcmp(args[0], COMANDO_CREDITAR) == 0) {
	      if (numargs < 3) {
	        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_CREDITAR);
	        continue;
	      }
	      cmd.operacao = OP_CREDITAR;
	      cmd.idConta = atoi(args[1]);
	      cmd.valor = atoi(args[2]);
	      cmd.pid = getpid();

	      escreverPipe(cmd,argv[1],out_pipe);
	    } else if (strcmp(args[0], COMANDO_TRANSFERIR) == 0) {
	     if (numargs < 4) {
	        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_TRANSFERIR);
	        continue;
	      }
	      cmd.operacao = OP_TRANSFERIR;
	      cmd.idConta = atoi(args[1]);
	      cmd.idContaDestino = atoi(args[2]);
	      cmd.valor = atoi(args[3]);
	      cmd.pid = getpid();

	      escreverPipe(cmd,argv[1],out_pipe);
	    }else if (strcmp(args[0], COMANDO_DEBITAR) == 0) {
	      if (numargs < 3) {
	        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_DEBITAR);
	        continue;
	      }
	      cmd.operacao = OP_DEBITAR;
	      cmd.idConta = atoi(args[1]);
	      cmd.valor = atoi(args[2]);
	      cmd.pid = getpid();

	      escreverPipe(cmd,argv[1],out_pipe);
	    }else if (numargs == 0){
	    	/* Nenhum argumento; ignora e volta a pedir */
	    	continue;
	    }else if (strcmp(args[0], COMANDO_SIMULAR) == 0) {

	      if (numargs < 2) {
	        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_SIMULAR);
	        continue;
	      }
	      
	      cmd.operacao = OP_SIMULAR;
	      cmd.valor = atoi(args[1]);
	      cmd.pid = getpid();

	      escreverPipe(cmd,argv[1],out_pipe);
	  	}else{
     		printf("Comando desconhecido. Tente de novo.\n");
   		}
	}
}

void escreverPipe(comando_t cmd, char * filename, char * out_pipe){

	int pipeWrite=open(filename,O_WRONLY | O_APPEND,S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
	int pipeRead=open(out_pipe,O_RDWR,S_IRUSR | S_IWUSR);
	char output[256];

	if(pipeWrite>=0 && pipeRead>=0){
		write(pipeWrite, &cmd,sizeof(cmd));
		if (cmd.operacao!=OP_SIMULAR){
			read(pipeRead, &output,sizeof(output));
			printf("%s",output);
		}return;
	}else{
  		printf("Um dos pipes não foi aberto correctamente\n");
  		exit(EXIT_SUCCESS);
	}
}