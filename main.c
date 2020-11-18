#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#define PASSAGEIRO  100  // compram passagens no atendimento, passam no scanner, esperam seu avião
#define ATENDIMENTO 3
#define SCANNER     2
#define AVIAO       8

/**
 * @brief Estrutura do passageiro, contém sua id, a id do avião que deseja
 * voar e se está carregando itens impróprios ou não.
 * 
 */
typedef struct Passageiros {
    int id;
    int aviao_id;
    int improprio;  // 0 - false, 1 - true
} Passageiro;
// pthread_mutex_t lock_passageiro = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Thread responsável por vender passagens aos passageiros.
 * Inicialmente, todos os passageiros se apresentam nessa fila.
 * 
 */
Passageiro queue_atendimento[PASSAGEIRO];
int atendimento_len = 0;
pthread_mutex_t lock_atendimento = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Thread responsável por averiguar se tem algum passageiro 
 * com itens impróprios. Se isso acontecer o passageiro deve retornar
 * para o atendimento.
 * 
 */
Passageiro queue_scanner[PASSAGEIRO];
int scanner_len = 0;
pthread_mutex_t lock_scanner = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Thread responsável por transportar o passageiro para o
 * local de destino. Se o passageiro perder o avião ele deve voltar
 * para o atendimento.
 * 
 */
Passageiro queue_patio[PASSAGEIRO];
int patio_len = 0;
pthread_mutex_t lock_patio = PTHREAD_MUTEX_INITIALIZER;

void *atendimento(void *arg);
void *scanner(void *arg);
void *aviao(void *arg);

int main(int argc, char **argv) {
    int i;
    int *id;

    srand((unsigned) time(NULL));

    for (int i = 0; i < PASSAGEIRO; i++) {      // criando passageiros e colocando na fila de atendimento
        Passageiro psg;
        psg.id = i;
        psg.aviao_id = rand() % AVIAO;
        psg.improprio = rand() % 10 < 9 ? 0 : 1;
        
        queue_atendimento[i] = psg;
        atendimento_len++;
    }

    pthread_t t_atendimento[ATENDIMENTO];
    pthread_t t_scanner[SCANNER];
    pthread_t t_aviao[AVIAO];

    for (i = 0; i < ATENDIMENTO; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        pthread_create(&(t_atendimento[i]), NULL, atendimento, (void *)(id));
    }
    
    for (i = 0; i < SCANNER; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        pthread_create(&(t_scanner[i]), NULL, scanner, (void *)(id));
    }

    for (i = 0; i < AVIAO; i++) {
        id = (int *)malloc(sizeof(int));
        *id = i;
        pthread_create(&(t_aviao[i]), NULL, aviao, (void *)(id));
    }

    pthread_join(t_atendimento[0], NULL);   // always running

    return 0;
}

void *atendimento(void *arg) {
    // int id = *(int *) arg;
    while(1){

    }
}

void *scanner(void *arg) {
    while(1){
        
    }
}

void *aviao(void *arg) {
    while(1){
        
    }
}