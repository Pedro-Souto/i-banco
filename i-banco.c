/*
// Projeto SO - exercicio 2, version 1
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

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_TRANSFERENCIA "transferir"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_ARG_SAIR_AGORA "agora"

#define OP_LER_SALDO  0
#define OP_CREDITAR   1
#define OP_DEBITAR    2
#define OP_SAIR       3
#define OP_TRANSFERIR 4

#define MAXARGS 4
#define BUFFER_SIZE 100
#define MAXFILHOS 20

typedef struct
{
  int operacao;
  int idConta;
  int valor;
  int idConta_Destino;
} comando_t;


#define NUM_TRABALHADORAS  3
#define CMD_BUFFER_DIM     (NUM_TRABALHADORAS * 2)

comando_t cmd_buffer[CMD_BUFFER_DIM];

int buff_write_idx = 0, buff_read_idx = 0, contador = 0;

////////////////////////////////////////////////////////////////
pthread_cond_t pode_simular;
pthread_mutex_t entrar_buffer;
////////////////////////////////////////////////////////////////

pthread_mutex_t buffer_ctrl;
sem_t sem_read_ctrl, sem_write_ctrl;

pthread_t thread_id[NUM_TRABALHADORAS];

int t_num[NUM_TRABALHADORAS]; /*##aux*/

void wait_on_simular(void) {
  while (pthread_cond_wait(&pode_simular, &entrar_buffer) != 0) {
    if (errno == EINTR)
      continue;

    perror("Error waiting at semaphore \"sem_read_ctrl\"");
    exit(EXIT_FAILURE);
  }
}

void signal_on_simular(void) {
  while (pthread_cond_signal(&pode_simular) != 0) {
    if (errno == EINTR)
      continue;

    perror("Error waiting at semaphore \"sem_read_ctrl\"");
    exit(EXIT_FAILURE);
  }
}

void lock_cmd_buffer_cond(void) {
  int rc;

  if ((rc = pthread_mutex_lock(&entrar_buffer)) != 0) {
    errno = rc;
    perror("Error in pthread_mutex_lock");
    exit(EXIT_FAILURE);
  }
}

void unlock_cmd_buffer_cond(void) {
  int rc;

  if ((rc = pthread_mutex_unlock(&entrar_buffer)) != 0) {
    errno = rc;
    perror("Error in pthread_mutex_unlock");
    exit(EXIT_FAILURE);
  }
}

void wait_on_read_sem(void) {
  while (sem_wait(&sem_read_ctrl) != 0) {
    if (errno == EINTR)
      continue;

    perror("Error waiting at semaphore \"sem_read_ctrl\"");
    exit(EXIT_FAILURE);
  }
}

void post_to_read_sem(void) {
  if(sem_post(&sem_read_ctrl) != 0) {
    perror("Error posting at semaphore \"sem_read_ctrl\"");
    exit(EXIT_FAILURE);
  }
}

void wait_on_write_sem(void) {
  while (sem_wait(&sem_write_ctrl) != 0) {
    if (errno == EINTR)
      continue;

    perror("Error waiting at semaphore \"sem_write_ctrl\"");
    exit(EXIT_FAILURE);
  }
}

void post_to_write_sem(void) {
  if(sem_post(&sem_write_ctrl) != 0) {
    perror("Error posting at semaphore \"sem_write_ctrl\"");
    exit(EXIT_FAILURE);
  }
}

void lock_cmd_buffer(void) {
  int rc;

  if ((rc = pthread_mutex_lock(&buffer_ctrl)) != 0) {
    errno = rc;
    perror("Error in pthread_mutex_lock");
    exit(EXIT_FAILURE);
  }
}

void unlock_cmd_buffer(void) {
  int rc;

  if ((rc = pthread_mutex_unlock(&buffer_ctrl)) != 0) {
    errno = rc;
    perror("Error in pthread_mutex_unlock");
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

  t_num = *((int *)arg_ptr);

  while(1) { 

  	wait_on_read_sem();
    lock_cmd_buffer_cond();
    lock_cmd_buffer();

    contador++;

    unlock_cmd_buffer_cond();

    get_cmd(&cmd);

    unlock_cmd_buffer();
    post_to_write_sem();


    if(cmd.operacao == OP_LER_SALDO)
    {
    	int saldo;

      	saldo = lerSaldo (cmd.idConta);
      	if (saldo < 0)
        	printf("Erro ao ler saldo da conta %d.\n", cmd.idConta);
      	else
        	printf("%s(%d): O saldo da conta é %d.\n\n", COMANDO_LER_SALDO, cmd.idConta, saldo);

    	lock_cmd_buffer_cond();
    	contador--;
    	unlock_cmd_buffer_cond();
    	signal_on_simular();
    }

    else if(cmd.operacao == OP_CREDITAR)
    {
      	if (creditar (cmd.idConta, cmd.valor) < 0)
       	 	printf("Erro ao creditar %d na conta %d.\n", cmd.valor, cmd.idConta);
      	else
       		printf("%s(%d, %d): OK\n\n", COMANDO_CREDITAR, cmd.idConta, cmd.valor);

       	lock_cmd_buffer_cond();
    	contador--;
    	unlock_cmd_buffer_cond();
    	signal_on_simular();
    }

    else if(cmd.operacao == OP_DEBITAR)
    {
      	if (debitar (cmd.idConta, cmd.valor) < 0)
        	printf("Erro ao debitar %d na conta %d.\n", cmd.valor, cmd.idConta);
      	else
        	printf("%s(%d, %d): OK\n\n", COMANDO_DEBITAR, cmd.idConta, cmd.valor);

        lock_cmd_buffer_cond();
    	contador--;
    	unlock_cmd_buffer_cond();
    	signal_on_simular();
    }

    else if(cmd.operacao == OP_TRANSFERIR)
    {
      	if (transferir (cmd.idConta, cmd.idConta_Destino, cmd.valor) < 0)
       		printf("Erro ao transferir %d da conta %d para a conta %d.\n", cmd.valor, cmd.idConta, cmd.idConta_Destino);
      	else
        	printf("%s(%d, %d, %d): OK\n\n", COMANDO_TRANSFERENCIA, cmd.idConta, cmd.idConta_Destino, cmd.valor);

        lock_cmd_buffer_cond();
    	contador--;
    	unlock_cmd_buffer_cond();
    	signal_on_simular();
    }

    else if(cmd.operacao == OP_SAIR)
    {
      return NULL;
    }

    else /* unknown command; ignore */
    {
      printf("Thread %d: Unknown command: %d\n", t_num, cmd.operacao);
      continue;
    }
  }
}

void start_threads(void) {
  int i, rc;

  for(i=0; i<NUM_TRABALHADORAS; ++i) {
    t_num[i] = i;
    if ((rc = pthread_create(&thread_id[i],
                             NULL,
                             thread_main,
                             (void *)&t_num[i])) != 0) {
      errno = rc;
      perror("Error in pthread_create");
      exit(EXIT_FAILURE);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
int main (int argc, char** argv) {
  char *args[MAXARGS + 1];
  char buffer[BUFFER_SIZE];
  comando_t cmd;
  int send_cmd_to_thread;
  
  int numFilhos = 0;
  pid_t pidFilhos[MAXFILHOS];

  (void)argc, (void)argv;
  
  inicializarContas();
    /* Associa o signal SIGUSR1 'a funcao trataSignal.
     Esta associacao e' herdada pelos processos filho que venham a ser criados.
     Alternativa: cada processo filho fazer esta associacao no inicio da
     funcao simular; o que se perderia com essa solucao?
     Nota: este aspeto e' de nivel muito avancado, logo
     so' foi exigido a projetos com nota maxima  */
  if (signal(SIGUSR1, trataSignal) == SIG_ERR) {
    perror("Erro ao definir signal.");
    exit(EXIT_FAILURE);
  }

  pthread_cond_init(&pode_simular, NULL);
  pthread_mutex_init(&entrar_buffer, NULL);

  pthread_mutex_init(&buffer_ctrl, NULL); /* this func allways return 0 (ok) */

  if(sem_init(&sem_read_ctrl, 0, 0) != 0) {
    perror("Could not initialize semaphore \"sem_read_ctrl\"");
    exit(EXIT_FAILURE);
  }
  if(sem_init(&sem_write_ctrl, 0, CMD_BUFFER_DIM) != 0) {
    perror("Could not initialize semaphore \"sem_read_ctrl\"");
    exit(EXIT_FAILURE);
  }

  start_threads();

  printf("Bem-vinda/o ao i-banco\n\n");
  
  while (1) {
    int numargs;
    
    numargs = readLineArguments(args, MAXARGS+1, buffer, BUFFER_SIZE);

    send_cmd_to_thread = 0; /* default is NO (do not send) */

    /* EOF (end of file) do stdin ou comando "sair" */
    if (numargs < 0 ||
        (numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0))) {
      int i, rc;

      printf("i-banco vai terminar.\n--\n");
      
      if (numargs > 1 && (strcmp(args[1], COMANDO_ARG_SAIR_AGORA) == 0)) {
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
        if ((rc = pthread_join(thread_id[i], NULL)) != 0) {
          errno = rc;
          perror("Error in pthread_join");
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
      
      printf("--\ni-banco terminou.\n");
      exit(EXIT_SUCCESS);
    }


    else if (numargs == 0)
      /* Nenhum argumento; ignora e volta a pedir */
      continue;


    /* Debitar */
    else if (strcmp(args[0], COMANDO_DEBITAR) == 0) {
      if (numargs < 3) {
        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_DEBITAR);
        continue;
      }
      cmd.operacao = OP_DEBITAR;
      cmd.idConta = atoi(args[1]);
      cmd.valor = atoi(args[2]);

      send_cmd_to_thread = 1;
    }
    
    /* Creditar */
    else if (strcmp(args[0], COMANDO_CREDITAR) == 0) {
      if (numargs < 3) {
        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_CREDITAR);
        continue;
      }
      cmd.operacao = OP_CREDITAR;
      cmd.idConta = atoi(args[1]);
      cmd.valor = atoi(args[2]);

      send_cmd_to_thread = 1;
    }
    
    /* Ler Saldo */
    else if (strcmp(args[0], COMANDO_LER_SALDO) == 0) {
      if (numargs < 2) {
        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_LER_SALDO);
        continue;
      }
      cmd.operacao = OP_LER_SALDO;
      cmd.idConta = atoi(args[1]);

      send_cmd_to_thread = 1;
    }

    /* Transferir */
    else if (strcmp(args[0], COMANDO_TRANSFERENCIA) == 0) {
      if (numargs < 3) {
        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_TRANSFERENCIA);
        continue;
      }
      cmd.operacao = OP_TRANSFERIR;
      cmd.idConta = atoi(args[1]);
      cmd.idConta_Destino = atoi(args[2]);
      cmd.valor = atoi(args[3]);

      send_cmd_to_thread = 1;
    }
    
    /* Simular */
    else if (strcmp(args[0], COMANDO_SIMULAR) == 0) {

      	int numAnos, pid;
      
      	if (numFilhos >= MAXFILHOS) {
        	printf("%s: Atingido o numero maximo de processos filho a criar.\n", COMANDO_SIMULAR);
        	continue;
      	}
      	if (numargs < 2) {
        	printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_SIMULAR);
        	continue;
      	}
      
      	numAnos = atoi(args[1]);

      	lock_cmd_buffer_cond();
    	while (contador != 0){
    		wait_on_simular();
    	}unlock_cmd_buffer_cond();


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
      	}
      	else {
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
    }

  } 
}
