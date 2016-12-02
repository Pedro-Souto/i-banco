
/*
// Projeto SO - exercicio 3, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2016-17
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

#define OP_LER_SALDO 	0
#define OP_CREDITAR  	1
#define OP_DEBITAR   	2
#define OP_SAIR      	3
#define OP_TRANSFERIR   4
#define OP_SIMULAR		5
#define OP_SAIR_AGORA   6

#define MAXARGS 4
#define BUFFER_SIZE 1000

#define MAXFILHOS 20

typedef struct{
  int operacao;
  int idConta;
  int idContaDestino;
  int valor;
  int pid;
} comando_t;

#define NUM_TRABALHADORAS  3
#define CMD_BUFFER_DIM     (NUM_TRABALHADORAS * 2)

comando_t cmd_buffer[CMD_BUFFER_DIM];

int buff_write_idx = 0, buff_read_idx = 0;

pthread_mutex_t buffer_ctrl, pedidos_pendentes_ctrl;
sem_t sem_read_ctrl, sem_write_ctrl;

pthread_t thread_id[NUM_TRABALHADORAS];

pthread_cond_t condSimular;
int pedidos_pendentes = 0;

int t_num[NUM_TRABALHADORAS];

char * myfifo = "i-banco-pipe";

void termHandler();

void wait_on_read_sem(void) {
  while (sem_wait(&sem_read_ctrl) != 0) {
    if (errno == EINTR)
      continue;

    perror("Error waiting at semaphore \"sem_read_ctrl\"");
    exit(EXIT_FAILURE);
  }
}
void post_to_read_sem(void) {
  if (sem_post(&sem_read_ctrl) != 0) {
    perror("Error posting at semaphore \"sem_read_ctrl\"");
    exit(EXIT_FAILURE);
  }
}

void wait_on_write_sem(void) {
  if (sem_wait(&sem_write_ctrl) != 0) {
    perror("Error waiting at semaphore \"sem_write_ctrl\"");
    exit(EXIT_FAILURE);
  }
}
void post_to_write_sem(void) {
  if (sem_post(&sem_write_ctrl) != 0) {
    perror("Error posting at semaphore \"sem_write_ctrl\"");
    exit(EXIT_FAILURE);
  }
}

void lock_cmd_buffer(void) {
  if ((errno = pthread_mutex_lock(&buffer_ctrl)) != 0) {
    perror("Error in pthread_mutex_lock()");
    exit(EXIT_FAILURE);
  }
}

void unlock_cmd_buffer(void) {
  if ((errno = pthread_mutex_unlock(&buffer_ctrl)) != 0) {
    perror("Error in pthread_mutex_unlock()");
    exit(EXIT_FAILURE);
  }
}


void lock_pedidos_pendentes(void) {
  if ((errno = pthread_mutex_lock(&pedidos_pendentes_ctrl)) != 0) {
    perror("Error in pthread_mutex_lock()");
    exit(EXIT_FAILURE);
  }
}

void unlock_pedidos_pendentes(void) {
  if ((errno = pthread_mutex_unlock(&pedidos_pendentes_ctrl)) != 0) {
    perror("Error in pthread_mutex_unlock()");
    exit(EXIT_FAILURE);
  }
}

void wait_cond_simular() {
  if ((errno = pthread_cond_wait(&condSimular, &pedidos_pendentes_ctrl)) != 0) {
    perror("Error in pthread_cond_wait()");
    exit(EXIT_FAILURE);
  }
}

void signal_cond_simular() {
  if ((errno = pthread_cond_signal(&condSimular)) != 0) {
    perror("Error in pthread_cond_signal()");
    exit(EXIT_FAILURE);
  }
}

void put_cmd(comando_t *cmd) {
  cmd_buffer[buff_write_idx] = *cmd;
  buff_write_idx = (buff_write_idx+1) % CMD_BUFFER_DIM;
}

void get_cmd(comando_t *cmd) {
  *cmd = cmd_buffer[buff_read_idx];
  buff_read_idx = (buff_read_idx+1) % CMD_BUFFER_DIM;
}

void *thread_main(void *arg_ptr) {
  int t_num;
  comando_t cmd;
  char outputFile[256];
  int pipeWrite;
  char outputTxt[256];

  t_num = *((int *)arg_ptr);

  while(1) {
	  
    wait_on_read_sem();
    lock_cmd_buffer();
    get_cmd(&cmd);
    unlock_cmd_buffer();
    post_to_write_sem();


    if(cmd.operacao == OP_LER_SALDO) {
      	int saldo;
      	saldo = lerSaldo (cmd.idConta, t_num);
      	sprintf(outputFile,"i-banco-pid%d", cmd.pid);
		pipeWrite=open(outputFile,O_RDWR,S_IRUSR | S_IWUSR);
      	if (saldo < 0){
        	sprintf(outputTxt, "Erro ao ler saldo da conta %d.\n", cmd.idConta);
     	}else{
	        sprintf(outputTxt, "%s(%d): O saldo da conta é %d.\n\n", COMANDO_LER_SALDO, cmd.idConta, saldo);
       	}
       	write(pipeWrite,&outputTxt,sizeof(outputTxt));
    }else if(cmd.operacao == OP_CREDITAR) {
    	sprintf(outputFile,"i-banco-pid%d", cmd.pid);
      	pipeWrite=open(outputFile,O_RDWR,S_IRUSR | S_IWUSR);
      	if (creditar (cmd.idConta, cmd.valor, t_num) < 0){
        	sprintf(outputTxt, "Erro ao creditar %d na conta %d.\n", cmd.valor, cmd.idConta);
      	} else{
        	sprintf(outputTxt, "%s(%d, %d): OK\n\n", COMANDO_CREDITAR, cmd.idConta, cmd.valor);
    	}
   		write(pipeWrite,&outputTxt,sizeof(outputTxt));
    } else if(cmd.operacao == OP_DEBITAR) {
    	sprintf(outputFile,"i-banco-pid%d", cmd.pid);
      	pipeWrite=open(outputFile,O_RDWR,S_IRUSR | S_IWUSR);
     	if (debitar (cmd.idConta, cmd.valor, t_num) < 0){
        	sprintf(outputTxt, "Erro ao debitar %d na conta %d.\n", cmd.valor, cmd.idConta);
      	}else{
	    	sprintf(outputTxt, "%s(%d, %d): OK\n\n", COMANDO_DEBITAR, cmd.idConta, cmd.valor);
    	}
  		write(pipeWrite,&outputTxt,sizeof(outputTxt));
    }else if(cmd.operacao == OP_TRANSFERIR) {
    	sprintf(outputFile,"i-banco-pid%d", cmd.pid);
      	pipeWrite=open(outputFile,O_RDWR,S_IRUSR | S_IWUSR);
      	if (transferir (cmd.idConta, cmd.idContaDestino, cmd.valor, t_num) < 0){
        	sprintf(outputTxt, "Erro ao transferir %d da conta %d para a conta %d.\n", cmd.valor, cmd.idConta, cmd.idContaDestino);
      	}else{
        	sprintf(outputTxt,"%s(%d, %d, %d): OK\n\n", COMANDO_TRANSFERIR, cmd.idConta, cmd.idContaDestino, cmd.valor);
    	}
    	write(pipeWrite,&outputTxt,sizeof(outputTxt));
    }else if(cmd.operacao == OP_SAIR){
      
      return NULL;
    } else /* unknown command; ignore */{
      printf("Thread %d: Unknown command: %d\n", t_num, cmd.operacao);
      continue;
    }
    
    lock_pedidos_pendentes();
    pedidos_pendentes--;
    if( !(pedidos_pendentes > 0))
      signal_cond_simular();
    unlock_pedidos_pendentes();
    
  }
}

void start_threads(void) {
  int i;

  for(i=0; i<NUM_TRABALHADORAS; ++i) {
    t_num[i] = i;
    if ((errno = pthread_create(&thread_id[i], NULL, thread_main, (void *)&t_num[i])) != 0) {
      perror("Error creating thread");
      exit(EXIT_FAILURE);
    }
  }
}

int main (int argc, char** argv) {

  //char *args[MAXARGS + 1];
  //char buffer[BUFFER_SIZE];

  comando_t cmd;
  comando_t cmd_pipe;

  int pipeRead;
  int send_cmd_to_thread;
  int numFilhos = 0;
  pid_t pidFilhos[MAXFILHOS];

  inicializarContas();
  abreLogFile();

  /* Criar Pipe */
  unlink(myfifo);

  if(mkfifo(myfifo, S_IRUSR | S_IWUSR)==-1){
    printf("Error creating i-banco-pipe");
  }
  /* Use pipe file as stdin */
  pipeRead=open(myfifo,O_RDWR,S_IRUSR | S_IWUSR);
  dup2(pipeRead,0);

  //signal(SIGUSR1,usrHandler);

  /*para apagar pipe se encerrado abruptamente (ctrl+c)*/
  signal(SIGTERM,termHandler);
  signal(SIGKILL,termHandler);

  /* Associa o signal SIGUSR1 'a funcao trataSignal.
     Esta associacao e' herdada pelos processos filho que venham a ser criados.
     Alternativa: cada processo filho fazer esta associacao no inicio da
     funcao simular; o que se perderia com essa solucao?
     Nota: este aspeto e' de nivel muito avancado, logo
     so' foi exigido a projetos com nota maxima  
  */
  if (signal(SIGUSR1, trataSignal) == SIG_ERR) {
    perror("Erro ao definir signal.");
    exit(EXIT_FAILURE);
  }

  if ((errno = pthread_mutex_init(&buffer_ctrl, NULL)) != 0) {
    perror("Could not initialize mutex \"buffer_ctrl\"");
    exit(EXIT_FAILURE);
  }
  if ((errno = pthread_mutex_init(&pedidos_pendentes_ctrl, NULL)) != 0) {
    perror("Could not initialize mutex \"pedidos_pendentes_ctrl\"");
    exit(EXIT_FAILURE);
  }

  if ((errno = pthread_cond_init(&condSimular, NULL)) != 0) {
    perror("Could not initialize condition \"condSimular\"");
    exit(EXIT_FAILURE);
  }

  if(sem_init(&sem_read_ctrl, 0, 0) != 0) {
    perror("Could not initialize semaphore \"sem_read_ctrl\"");
    exit(EXIT_FAILURE);
  }
  if(sem_init(&sem_write_ctrl, 0, CMD_BUFFER_DIM) != 0) {
    perror("Could not initialize semaphore \"sem_read_ctrl\"");
    exit(EXIT_FAILURE);
  }

  printf("Bem-vinda/o ao i-banco\n\n");

  start_threads();
  
  while (1) {
    //t numargs;
    
    read(pipeRead,&cmd_pipe,sizeof(cmd_pipe));

    //numargs = readLineArguments(args, MAXARGS+1, buffer, BUFFER_SIZE);

    send_cmd_to_thread = 0; /* default is NO (do not send) */

    /* EOF (end of file) do stdin ou comando "sair" */
    if (cmd_pipe.operacao==OP_SAIR || cmd_pipe.operacao==OP_SAIR_AGORA) {
      int i;

      printf("i-banco vai terminar.\n--\n");
      
      if (cmd_pipe.operacao==OP_SAIR_AGORA) {
        for (i=0; i<numFilhos; i++)
          kill(pidFilhos[i], SIGUSR1);
      }
      /* Uma alternativa igualmente correta: kill(0, SIGUSR1); */


      /* informa todas as threads que devem sair */
      cmd.operacao = OP_SAIR;
      for(i=0; i < NUM_TRABALHADORAS; ++i) {
        wait_on_write_sem();
        put_cmd(&cmd);
        post_to_read_sem();
      }

      /* Espera pela terminação de cada thread */
      for(i=0; i < NUM_TRABALHADORAS; ++i) {
        if ((errno = pthread_join(thread_id[i], NULL)) != 0) {
          perror("Error joining thread.");
          exit(EXIT_FAILURE);
        }
      }

      
      /* Espera pela terminacao de processos filho */
      while (numFilhos > 0) {
        int status;
        pid_t childpid;
	
        childpid = wait(&status);
        if (childpid < 0) {
          if (errno == EINTR) {
            /* Este codigo de erro significa que chegou signal que interrompeu a espera
               pela terminacao de filho; logo voltamos a esperar */
            continue;
          }
          else if (errno == ECHILD) {
            /* Este codigo de erro significa que não há mais processos filho */
            break;
          }
          else {
            perror("Erro inesperado ao esperar por processo filho.");
            exit (EXIT_FAILURE);
          }
        }
	
        numFilhos --;
        if (WIFEXITED(status))
          printf("FILHO TERMINADO: (PID=%d; terminou normalmente)\n", childpid);
        else
          printf("FILHO TERMINADO: (PID=%d; terminou abruptamente)\n", childpid);
      }
      fechaLogFile();
      printf("--\ni-banco terminou.\n");
      
      
      sem_destroy(&sem_read_ctrl);
      sem_destroy(&sem_write_ctrl);
      pthread_mutex_destroy(&buffer_ctrl);
      pthread_mutex_destroy(&pedidos_pendentes_ctrl);
      pthread_cond_destroy(&condSimular);
      destroy_mutex_contas();
      unlink(myfifo);
      exit(EXIT_SUCCESS);
    }

    /* Debitar */
    else if (cmd_pipe.operacao==OP_DEBITAR) {

      cmd.operacao = cmd_pipe.operacao;
      cmd.idConta = cmd_pipe.idConta;
      cmd.valor = cmd_pipe.valor;
      cmd.pid = cmd_pipe.pid;

      send_cmd_to_thread = 1;
    }
    
    /* Creditar */
    else if (cmd_pipe.operacao==OP_CREDITAR) {

      cmd.operacao = cmd_pipe.operacao;
      cmd.idConta = cmd_pipe.idConta;
      cmd.valor = cmd_pipe.valor;
      cmd.pid = cmd_pipe.pid;

      send_cmd_to_thread = 1;
    }
    
    else if (cmd_pipe.operacao==OP_TRANSFERIR) {

      cmd.operacao = cmd_pipe.operacao;
      cmd.idConta = cmd_pipe.idConta;
      cmd.idContaDestino = cmd_pipe.idContaDestino;
      cmd.valor = cmd_pipe.valor;
      cmd.pid = cmd_pipe.pid;
      
      send_cmd_to_thread = 1;
    }
    
    /* Ler Saldo */
    else if (cmd_pipe.operacao==OP_LER_SALDO) {

      cmd.operacao = cmd_pipe.operacao;
      cmd.idConta = cmd_pipe.idConta;
      cmd.pid = cmd_pipe.pid;

      send_cmd_to_thread = 1;
    }
    
    /* Simular */
    else if (cmd_pipe.operacao==OP_SIMULAR) {
      
      int numAnos;
      int pid;
      
      if (numFilhos >= MAXFILHOS) {
        printf("%s: Atingido o numero maximo de processos filho a criar.\n", COMANDO_SIMULAR);
        continue;
      }
      
      numAnos = cmd_pipe.valor;
      
      lock_pedidos_pendentes();
      while(pedidos_pendentes > 0)
		wait_cond_simular();
      unlock_pedidos_pendentes();
      
      copyPasteFlush(); 
      pid = fork();
      
      if (pid < 0) {
        perror("Failed to create new process.");
        exit(EXIT_FAILURE);
      }
      
      if (pid > 0) { 
      		 
        pidFilhos[numFilhos] = pid;
        numFilhos ++;

        printf("%s(%d): Simulacao iniciada em background.\n\n", COMANDO_SIMULAR, numAnos);
        continue;
      } else {

        int simular_log;
        char filename[256];

        fechaLogFile();

        sprintf(filename,"i-banco-sim-%d.txt",getpid());
        simular_log=open(filename, O_CREAT | O_WRONLY,S_IRUSR | S_IWUSR);

        dup2(simular_log,1);
        
        simular(numAnos);
        exit(EXIT_SUCCESS);
      }
    }
    
    else
      printf("Comando desconhecido. Tente de novo.\n");

    if(send_cmd_to_thread) {

      wait_on_write_sem();
      put_cmd(&cmd);  
      post_to_read_sem();
      
      lock_pedidos_pendentes();
      pedidos_pendentes++;
      unlock_pedidos_pendentes();
    }
  } 
}

void termHandler(void) {

  unlink(myfifo);
  exit(-1);
}