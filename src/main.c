/**
 * @file main.c
 * @author Eduardo F. Assis (eduardoffassis@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-11-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // sleep

#include "../header/aeroporto.h"
#include "../header/queue.h"

#define PASSAGEIRO  30  // compram passagens no atendimento, passam no scanner, esperam seu avião no patio
#define ATENDIMENTO 3
#define SCANNER     2
#define AVIAO       5

// tempo de voo
#define AVIAO_TIME_VOO_BASE 8
#define AVIAO_TIME_VOO_OFFSET 1     // offset must be less than base
// tempo de espera no patio
#define AVIAO_ESPERA_BASE   20
#define AVIAO_ESPERA_OFFSET 8
// carga maxima (passageiros)
#define AVIAO_CARGA_BASE 5
#define AVIAO_CARGA_OFFSET 1

Aviao aviao[AVIAO];     // array de avioes
pthread_mutex_t aviao_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Thread responsável por vender passagens aos passageiros.
 * Inicialmente, todos os passageiros se apresentam nessa fila.
 * 
 */
Queue *atendimento_queue;
pthread_mutex_t atendimento_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t atendimento_cond = PTHREAD_COND_INITIALIZER;

/**
 * @brief Thread responsável por averiguar se tem algum passageiro 
 * com itens impróprios. Se isso acontecer o passageiro deve retornar
 * para o atendimento.
 * 
 */
Queue *scanner_queue;
pthread_mutex_t scanner_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t scanner_atendimento_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t scanner_patio_cond = PTHREAD_COND_INITIALIZER;

/**
 * @brief Thread responsável por transportar o passageiro para o
 * local de destino. Se o passageiro perder o avião ele deve voltar
 * para o atendimento.
 * 
 */
Queue *patio_queue;
pthread_mutex_t patio_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t patio_aviao_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t patio_atendimento_cond = PTHREAD_COND_INITIALIZER;

void *atendimento(void *arg);
void *scanner(void *arg);
void *patio(void *arg);
void *torre_controle(void *arg);

int main(int argc, char **argv) {
    int i;
    int *id;

    srand((unsigned) time(NULL));

    // inicializando filas
    atendimento_queue = queue_init();
    scanner_queue = queue_init();
    patio_queue = queue_init();
    
    for (int i = 0; i < PASSAGEIRO; i++) {      // criando passageiros e colocando na fila de atendimento
        Passageiro *psg = (Passageiro *) malloc(sizeof(Passageiro));
        psg->id = i;
        psg->aviao_id = rand() % AVIAO;
        psg->improprio = rand() % 10 < 9 ? 0 : 1;   // 10% de chance de estar com item improprio
        
        queue_insert(atendimento_queue, psg);
    }

    for (int i = 0; i < AVIAO; i++) {    // inicializando avioes
        aviao[i].id = i;
        aviao[i].max_carga = AVIAO_CARGA_BASE + AVIAO_CARGA_OFFSET - 2 * (1 + rand() % AVIAO_CARGA_OFFSET);
        aviao[i].carga = 0;
        aviao[i].tempo_espera = AVIAO_ESPERA_BASE + AVIAO_ESPERA_OFFSET - 2 * (1 + rand() % AVIAO_ESPERA_OFFSET);
        aviao[i].tempo_voo = 0;
    }
    
    pthread_t t_atendimento[ATENDIMENTO];
    pthread_t t_scanner[SCANNER];
    pthread_t t_patio;
    pthread_t t_torre_controle;

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

    pthread_create(&t_patio, NULL, patio, NULL);
    pthread_create(&t_torre_controle, NULL, torre_controle, NULL);

    pthread_join(t_atendimento[0], NULL);   // always running

    return 0;
}

void *atendimento(void *arg) {
    int id = *(int *) arg;
    
    while(1){
        pthread_mutex_lock(&atendimento_lock);
        
        if (atendimento_queue->size) {
            Passageiro *pas = queue_pop(atendimento_queue);
            pthread_mutex_unlock(&atendimento_lock);    // libera acesso a fila

            pthread_cond_broadcast(&scanner_atendimento_cond);      // avisar outras threads que estavam esperando espaço na fila
            pthread_cond_broadcast(&patio_atendimento_cond);

            printf("Atendimento %d: Cliente %d sendo atendido.\n", id, pas->id);
            sleep(rand_r(arg) % 5);
            pas->aviao_id = rand() % AVIAO;
            
            pthread_mutex_lock(&scanner_lock);
            while (scanner_queue->size >= PASSAGEIRO) { // colocar na fila do scanner
                printf("Atendimento %d: Fila do scanner cheia. Passageiro %d aguardando no atendimento.\n", id, pas->id);
                pthread_cond_wait(&atendimento_cond, &scanner_lock);   // dormir e esperar a fila esvaziar
            }
            printf("Atendimento %d: Passageiro %d prosseguindo para o scanner.\n", id, pas->id);
            queue_insert(scanner_queue, pas);

            pthread_mutex_unlock(&scanner_lock);
        
        } else {
            printf("Atendimento %d: Nao tem ninguem para ser atendido.\n", id);
            pthread_mutex_unlock(&atendimento_lock);
        }

        sleep(rand_r(arg) % 2);
    }
}

void *scanner(void *arg) {
    int id = *(int *) arg;

    while(1){
        pthread_mutex_lock(&scanner_lock);

        if (scanner_queue->size) {
            Passageiro *pas = queue_pop(scanner_queue);
            pthread_mutex_unlock(&scanner_lock);

            pthread_cond_broadcast(&atendimento_cond);

            printf("\tScanner %d: Passageiro %d está sendo averiguado.\n", id, pas->id);
            sleep(rand_r(arg) % 5);

            if (pas->improprio) {
                printf("\tScanner %d: Passageiro %d com itens impróprios.\n", id, pas->id);
                pas->improprio = 0;  // remove itens improprios do passageiro e permite que ele retorne para o atendimento
                pthread_mutex_lock(&atendimento_lock);
                while (atendimento_queue->size >= PASSAGEIRO) {     // checando se a fila do atendimento nao está cheia
                    printf("\tScanner %d: Fila de atendimento cheia, passageiro %d aguardando.\n", id, pas->id);
                    pthread_cond_wait(&scanner_atendimento_cond, &atendimento_lock);
                }
                printf("\tScanner %d: Passageiro %d voltou para o atendimento.\n", id, pas->id);
                queue_insert(atendimento_queue, pas);
                pthread_mutex_unlock(&atendimento_lock);
            
            } else {    // prossegue para o patio
                printf("\tScanner %d: Passageiro %d liberado.\n", id, pas->id); 
                pthread_mutex_lock(&patio_lock);
                while (patio_queue->size >= PASSAGEIRO) {     // checando a fila do patio
                    printf("\tScanner %d: Patio cheio, passageiro %d aguardando.\n", id, pas->id);
                    pthread_cond_wait(&scanner_patio_cond, &patio_lock);
                }
                printf("\tScanner %d: Passageiro %d prosseguiu para o patio.\n", id, pas->id);
                queue_insert(patio_queue, pas);
                pthread_mutex_unlock(&patio_lock);
            }
        } else {
            printf("\tScanner %d: Nenhum passageiro para passar no scanner.\n", id);
            pthread_mutex_unlock(&scanner_lock);
        }

        sleep(rand_r(arg) % 2);
    }
}

void *patio(void *arg) {
    
    while(1){
        pthread_mutex_lock(&patio_lock);

        if (patio_queue->size) {
            Passageiro *pas = queue_pop(patio_queue);
            pthread_mutex_unlock(&patio_lock);

            pthread_cond_broadcast(&scanner_patio_cond);

            printf("\t\tPatio: Passageiro %d procurando seu aviao.\n", pas->id);
            sleep(rand() % 2);
            
            pthread_mutex_lock(&aviao_lock);
            for (int i = 0; i < AVIAO; i++) {   // procurando aviao do passageiro
                if (aviao[i].id == pas->aviao_id) {
                    while (aviao[i].tempo_voo > 0 || aviao[i].carga == aviao[i].max_carga) {    // aviao cheio ou fora do patio, passageiro deve esperar
                        printf("\t\tPatio: Passageiro %d aguardando o aviao %d.\n", pas->id, aviao[i].id);
                        pthread_cond_wait(&patio_aviao_cond, &aviao_lock);          
                    }
                    aviao[i].carga++;
                    printf("\t\tPatio: Passageiro %d embarcou.\n", pas->id);
                    free(pas);
                
                    break;
                }
            }
            pthread_mutex_unlock(&aviao_lock);
        } else {    // patio vazio
            printf("\t\tPatio: Nenhum passageiro para embarcar.\n");
            pthread_mutex_unlock(&patio_lock);
        }

        sleep(rand() % 5);
    }
}

void *torre_controle(void *arg) {
    int aviao_status[AVIAO] = { 0 }; // 0 - decolando, 1 - pousando;

    while(1) {
        sleep(1);
        
        pthread_mutex_lock(&aviao_lock);
        for (int i = 0; i < AVIAO; i++) {
            if (aviao[i].tempo_voo) aviao[i].tempo_voo--;
            if (aviao[i].tempo_espera) aviao[i].tempo_espera--;
            
            if (aviao[i].tempo_voo == 0) {  // aviao no aeroporto
                if (aviao_status[i] == 0) { // aviao no patio (quer decolar)
                    if (aviao[i].carga == aviao[i].max_carga) {     // capacidade maxima
                        aviao[i].tempo_voo = AVIAO_TIME_VOO_BASE + AVIAO_TIME_VOO_OFFSET - 2 * (1 + rand() % AVIAO_TIME_VOO_OFFSET);  // tempo de voo
                        printf("\t\t\tTorre: Aviao %d decolou. Boa viagem!\n", aviao[i].id); 

                        aviao_status[i] = 1;
                    } else if (aviao[i].tempo_espera == 0) {        // muito tempo esperando
                        aviao[i].tempo_voo = AVIAO_TIME_VOO_BASE + AVIAO_TIME_VOO_OFFSET - 2 * (1 + rand() % AVIAO_TIME_VOO_OFFSET);  // tempo de voo
                        printf("\t\t\tTorre: Aviao %d esperou muito tempo e esta decolando com %d de %d.\n", aviao[i].id, aviao[i].carga, aviao[i].max_carga);
                        
                        aviao_status[i] = 1;
                    }
                    
                } else {    // aviao chegando no aeroporto
                    aviao[i].carga = 0;
                    aviao[i].tempo_espera = AVIAO_ESPERA_BASE + AVIAO_ESPERA_OFFSET - 2 * (1 + rand() % AVIAO_ESPERA_OFFSET);  // tempo de voo
                    printf("\t\t\tTorre: Aviao %d aterrizou.\n", aviao[i].id);
                    pthread_cond_broadcast(&patio_aviao_cond);  // avisar o patio que o aviao pousou.
                    
                    aviao_status[i] = 0;
                }
            }
        }
        pthread_mutex_unlock(&aviao_lock);
    }
}
