#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // sleep

#define PASSAGEIRO  30  // compram passagens no atendimento, passam no scanner, esperam seu avião no patio
#define ATENDIMENTO 3
#define SCANNER     2
#define AVIAO       3

// tempo de voo
#define AVIAO_TIME_BASE 3
#define AVIAO_TIME_OFFSET 2     // offset must be less than base

// carga maxima
#define AVIAO_CARGA_BASE 2
#define AVIAO_CARGA_OFFSET 1

/**
 * @brief Estrutura do passageiro, contém sua id, a id do avião que deseja
 * voar e se está carregando itens impróprios ou não.
 * 
 */
typedef struct Passageiros {
    int id;
    int aviao_id;
    int improprio;  /**< 0 - false, 1 - true */
} Passageiro;

typedef struct Avioes {
    int id;
    int max_carga;  /**< quantas pessoas pode carregar */
    int carga;      /**< carga atual */
    int tempo;      /**< quanto tempo falta para retornar ao aeroporto */
} Aviao;

Aviao aviao[AVIAO];     // array de avioes
pthread_mutex_t aviao_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Thread responsável por vender passagens aos passageiros.
 * Inicialmente, todos os passageiros se apresentam nessa fila.
 * 
 */
Passageiro atendimento_queue[PASSAGEIRO];
int atendimento_len = 0;
int atendimento_consulta_aviao = 0;     // 0 - false, 1 - true (usado para saber se atendimento está com o lock dos avioes)
pthread_mutex_t atendimento_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t atendimento_cond = PTHREAD_COND_INITIALIZER;

/**
 * @brief Thread responsável por averiguar se tem algum passageiro 
 * com itens impróprios. Se isso acontecer o passageiro deve retornar
 * para o atendimento.
 * 
 */
Passageiro scanner_queue[PASSAGEIRO];
int scanner_len = 0;
pthread_mutex_t scanner_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t scanner_atendimento_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t scanner_patio_cond = PTHREAD_COND_INITIALIZER;

/**
 * @brief Thread responsável por transportar o passageiro para o
 * local de destino. Se o passageiro perder o avião ele deve voltar
 * para o atendimento.
 * 
 */
Passageiro patio_queue[PASSAGEIRO];
int patio_len = 0;
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

    for (int i = 0; i < PASSAGEIRO; i++) {      // criando passageiros e colocando na fila de atendimento
        Passageiro psg;
        psg.id = i;
        psg.aviao_id = rand() % AVIAO;
        psg.improprio = rand() % 10 < 9 ? 0 : 1;
        
        atendimento_queue[i] = psg;
        atendimento_len++;

        // sem_init(&atendimento_ativo[i], 0, 0);
        // sem_init(&scanner_ativo[i], 0, 0);
        // sem_init(&patio_ativo[i], 0, 0);
    }

    for (int i = 0; i < AVIAO; i++) {    // inicializando avioes
        aviao[i].id = i;
        aviao[i].max_carga = AVIAO_CARGA_BASE + AVIAO_CARGA_OFFSET - 2 * (1 + rand() % AVIAO_CARGA_OFFSET);
        aviao[i].carga = 0;
        aviao[i].tempo = 0;
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
        // sem_wait(&atendimento_ativo[id]);
        pthread_mutex_lock(&atendimento_lock);
        
        if (atendimento_len) {
            Passageiro pas = atendimento_queue[--atendimento_len];
            pthread_mutex_unlock(&atendimento_lock);    // libera acesso a fila

            pthread_cond_broadcast(&scanner_atendimento_cond);      // avisar outras threads que estavam esperando espaço na fila
            pthread_cond_broadcast(&patio_atendimento_cond);

            printf("Atendimento %d: Cliente %d sendo atendido.\n", id, pas.id);
            sleep(rand_r(arg) % 5);
            pas.aviao_id = rand() % AVIAO;
            
            // conferir tb a fila do patio
            pthread_mutex_lock(&scanner_lock);
            while (scanner_len >= PASSAGEIRO) { // colocar na fila do scanner
                printf("Atendimento %d: Fila do scanner cheia. Passageiro %d aguardando no atendimento.\n", id, pas.id);
                pthread_cond_wait(&atendimento_cond, &scanner_lock);   // dormir e esperar a fila esvaziar
            }
            printf("Atendimento %d: Passageiro %d prosseguindo para o scanner.\n", id, pas.id);
            scanner_queue[scanner_len++] = pas;

            pthread_mutex_unlock(&scanner_lock);
        
        } else {
            printf("Atendimento %d: Não tem ninguém para ser atendido.\n", id);
            pthread_mutex_unlock(&atendimento_lock);
        }

        // sem_post(&atendimento_ativo[id]);
        sleep(rand_r(arg) % 2);
    }
}

void *scanner(void *arg) {
    int id = *(int *) arg;

    while(1){
        pthread_mutex_lock(&scanner_lock);

        if (scanner_len) {
            Passageiro pas = scanner_queue[--scanner_len];
            pthread_mutex_unlock(&scanner_lock);

            pthread_cond_broadcast(&atendimento_cond);

            printf("\tScanner %d: Passageiro %d está sendo averiguado.\n", id, pas.id);
            sleep(rand_r(arg) % 5);

            if (pas.improprio) {
                printf("\tScanner %d: Passageiro %d com itens impróprios.\n", id, pas.id);
                pas.improprio = 0;  // remove itens improprios do passageiro e permite que ele retorne para o atendimento
                pthread_mutex_lock(&atendimento_lock);
                while (atendimento_len >= PASSAGEIRO) {     // checando se a fila do atendimento nao está cheia
                    printf("\tScanner %d: Fila de atendimento cheia, passageiro %d aguardando.\n", id, pas.id);
                    pthread_cond_wait(&scanner_atendimento_cond, &atendimento_lock);
                }
                printf("\tScanner %d: Passageiro %d voltou para o atendimento.\n", id, pas.id);
                atendimento_queue[atendimento_len++] = pas;
                pthread_mutex_unlock(&atendimento_lock);
            
            } else {    // prossegue para o patio
                printf("\tScanner %d: Passageiro %d liberado.\n", id, pas.id); 
                pthread_mutex_lock(&patio_lock);
                while (patio_len >= PASSAGEIRO) {     // checando a fila do patio
                    printf("\tScanner %d: Patio cheio, passageiro %d aguardando.\n", id, pas.id);
                    pthread_cond_wait(&scanner_patio_cond, &patio_lock);
                }
                printf("\tScanner %d: Passageiro %d prosseguiu para o patio.\n", id, pas.id);
                patio_queue[patio_len++] = pas;
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

        if (patio_len) {
            Passageiro pas = patio_queue[--patio_len];
            pthread_mutex_unlock(&patio_lock);

            pthread_cond_broadcast(&scanner_patio_cond);

            printf("\t\tPatio: Passageiro %d procurando seu aviao.\n", pas.id);
            sleep(rand() % 2);
            
            int encontrou = 0;
            pthread_mutex_lock(&aviao_lock);
            for (int i = 0; i < AVIAO; i++) {
                if (aviao[i].id == pas.aviao_id) {
                    while (aviao[i].tempo > 0) {    // aviao fora do patio, passageiro deve esperar
                        printf("\t\tPatio: Passageiro %d aguardando o aviao %d.\n", pas.id, aviao[i].id);
                        pthread_cond_wait(&patio_aviao_cond, &aviao_lock);          
                    }
                    aviao[i].carga++;
                    printf("\t\tPatio: Passageiro %d embarcou.\n", pas.id);

                    if (aviao[i].carga == aviao[i].max_carga) {  // aviao cheio
                        aviao[i].tempo = AVIAO_TIME_BASE + AVIAO_TIME_OFFSET - 2 * (1 + rand() % AVIAO_TIME_OFFSET);  // tempo de voo
                        printf("\t\tPatio: Aviao %d decolou!\n", aviao[i].id);
                    }
                    
                    encontrou = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&aviao_lock);
            
            if (!encontrou) {   // caso o passageiro não encontre seu avião
                printf("\t\tPatio: Passageiro %d nao encontrou seu aviao.\n", pas.id);
                pthread_mutex_lock(&atendimento_lock);
                while (atendimento_len >= PASSAGEIRO) {
                    pthread_cond_wait(&patio_atendimento_cond, &atendimento_lock);
                }
                printf("\t\tPatio: Passageiro %d retornou para o atendimento.\n", pas.id);
                atendimento_queue[atendimento_len++] = pas;
                pthread_mutex_unlock(&atendimento_lock);
            }
        } else {    // patio vazio
            printf("\t\tPatio: Nenhum passageiro para embarcar.\n");
            pthread_mutex_unlock(&patio_lock);
        }

        sleep(rand() % 5);
    }
}

void *torre_controle(void *arg) {
    while(1) {
        sleep(1);
        
        pthread_mutex_lock(&aviao_lock);
        for (int i = 0; i < AVIAO; i++) {
            if (aviao[i].tempo) aviao[i].tempo--;
            
            if (aviao[i].tempo == 0 && aviao[i].carga == aviao[i].max_carga) {   // se caso algum aviao tiver voltado, avisar o patio.
                aviao[i].carga = 0;
                printf("\t\t\tTorre: Aviao %d aterrizou.\n", aviao[i].id);
                pthread_cond_broadcast(&patio_aviao_cond);
            }
        }
        pthread_mutex_unlock(&aviao_lock);
    }
}

// colocar tempo maximo de espera pelo aviao, depois desse tempo o aviao decola mesmo sem estar na capacidade maxima.