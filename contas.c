#include "contas.h"
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#define atrasar() sleep(ATRASO)

int deveTerminar = 0;
		     
int contasSaldos[NUM_CONTAS];

pthread_mutex_t account_ctrl[NUM_CONTAS];

void lock_account(int account) {
  int rc;

  assert(contaExiste(account));
  rc = pthread_mutex_lock(&account_ctrl[account-1]);

  /* confirmar que antes foi verificado que se trata de uma conta válida */
  if (rc != 0) {
    errno = rc;
    perror("Error in pthread_mutex_lock");
    exit(EXIT_FAILURE);
  }
}

void unlock_account(int account) {
  int rc;

  assert(contaExiste(account));
  rc = pthread_mutex_unlock(&account_ctrl[account-1]);

  /* confirmar que antes foi verificado que se trata de uma conta válida */
  if (rc != 0) {
    errno = rc;
    perror("Error in pthread_mutex_unlock");
    exit(EXIT_FAILURE);
  }
}

int contaExiste(int idConta) {
  return (idConta > 0 && idConta <= NUM_CONTAS);
}

void inicializarContas() {
  int i;
  for (i=0; i<NUM_CONTAS; i++)
    contasSaldos[i] = 0;
}

int debitar(int idConta, int valor) {
  atrasar();
  if (!contaExiste(idConta))
    return -1;

  lock_account(idConta);

  if (contasSaldos[idConta - 1] < valor) {
    unlock_account(idConta);
    return -1;
  }
  atrasar();
  contasSaldos[idConta - 1] -= valor;

  unlock_account(idConta);
  return 0;
}

int creditar(int idConta, int valor) {
  atrasar();
  if (!contaExiste(idConta))
    return -1;

  lock_account(idConta);
  contasSaldos[idConta - 1] += valor;
  unlock_account(idConta);
  return 0;
}

int transferir(int idContaOrigem, int idContaDestino, int valor){
  atrasar();
  if (!contaExiste(idContaOrigem) || !contaExiste(idContaDestino))
    return -1;

  lock_account(idContaOrigem);
  if (contasSaldos[idContaOrigem - 1] < valor) {
    unlock_account(idContaOrigem);
    return -1;
  }

  lock_account(idContaDestino);

  contasSaldos[idContaOrigem - 1] -= valor;
  contasSaldos[idContaDestino - 1] += valor;

  unlock_account(idContaOrigem);
  unlock_account(idContaDestino);
  
  return 0;

}

int lerSaldo(int idConta) {
  int saldo;

  atrasar();
  if (!contaExiste(idConta))
    return -1;

  lock_account(idConta);
  saldo = contasSaldos[idConta - 1];
  unlock_account(idConta);
  return saldo;
}

void trataSignal(int sig) {
  (void)sig;
  deveTerminar = 1;
}

void simular(int numAnos) {
  int id, saldo;
  int ano = 0;
  
  for (ano = 0; 
       ano <= numAnos && !deveTerminar;
       ano++) {
    
    printf("SIMULAÇÃO: Ano %d\n=================\n", ano);
    for (id = 1; id<=NUM_CONTAS; id++) {
      if (ano > 0) {
        saldo = lerSaldo(id);
        creditar(id, saldo * TAXAJURO);
        saldo = lerSaldo(id);
        debitar(id, (CUSTOMANUTENCAO > saldo) ? saldo : CUSTOMANUTENCAO);
      }
      saldo = lerSaldo(id);
      /* A funcao printf pode ser interrompida pelo SIGUSR1,
	 retornando -1 e colocando errno=EINTR.
	 Para lidar com esse caso, repetimos o printf sempre
	 que essa situacao se verifique. 
	 Nota: este aspeto e' de nivel muito avancado, logo
	 so' foi exigido a projetos com nota maxima
      */
      while (printf("Conta %5d,\t Saldo %10d\n", id, saldo) < 0) {
        if (errno == EINTR)
          continue;
        else
          break;
      }
    }
  }
  
  if (deveTerminar)
    printf("Simulacao terminada por signal\n");
    //printf("Simulacao terminada por ordem do processo pai\n");
  
}
