/*
// Operações sobre contas, versao 1
// Sistemas Operativos, DEI/IST/ULisboa 2016-17
*/

#ifndef CONTAS_H
#define CONTAS_H

#define NUM_CONTAS 10
#define TAXAJURO 0.1
#define CUSTOMANUTENCAO 1

#define ATRASO 1

void inicializarContas();
void abreLogFile();
void fechaLogFile();
void copyPasteFlush();
void destroy_mutex_contas();
int contaExiste(int idConta);
int debitar(int idConta, int valor, int id);
int debitarSemMutex(int idConta, int valor, int id);
int creditar(int idConta, int valor, int id);
int creditarSemMutex(int idConta, int valor, int id);
int transferir(int idContaOrigem, int idContaDestino, int valor, int id);
int lerSaldo(int idConta, int id);
void trataSignal(int sig);
void simular(int numAnos);


#endif
