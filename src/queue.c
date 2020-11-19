/**
 * @file queue.c
 * @author Eduardo F. Assis (eduardoffassis@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-11-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../header/aeroporto.h"
#include "../header/queue.h"

Queue *queue_init() {
    Queue *queue = (Queue *) malloc(sizeof(Queue));
    
    queue->first_element = NULL;
    queue->last_element = NULL;
    queue->size = 0;

    return queue;
}

void queue_insert(Queue *queue, Passageiro *pas) {
    Element *ele = (Element *) malloc(sizeof(Element));
    
    ele->content = pas;
    ele->next = NULL;
    ele->previous = queue->last_element;
    
    if (queue->size) {
        queue->last_element->next = ele;
    }

    if (!queue->size) queue->first_element = ele;
    queue->last_element = ele;
    queue->size++;
}

Passageiro *queue_pop(Queue *queue) {
    if (!queue->size) return NULL;

    Passageiro *pas = queue->first_element->content;

    Element *ele = queue->first_element;

    queue->first_element = queue->first_element->next;
    if (queue->first_element)
        queue->first_element->previous = NULL;
    else 
        queue->last_element = NULL;
    
    queue->size--;

    free(ele);

    return pas;
}

void queue_clear(Queue *queue) {
    while (queue->size) {
        Passageiro *pas = queue_pop(queue);
        free(pas);
    }
}

void queue_destruct(Queue *queue) {
    queue_clear(queue);
    free(queue);
}

int teste() {
    int n = 2;
    Queue *q = queue_init();

    srand(time(NULL));

    for (int i = 0; i < n; i++) {
        Passageiro *pas = (Passageiro *) malloc(sizeof(Passageiro));
        pas->id = rand() % 101;
        pas->aviao_id = rand() % 11;
        pas->improprio = rand() % 2;
        
        queue_insert(q, pas);
    }

    Passageiro *pas1, *pas2;
    pas1 = queue_pop(q);
    pas2 = queue_pop(q);
    printf("Passageiro %d deseja voar para %d.\n", pas1->id, pas1->aviao_id);

    Passageiro *pas3 = (Passageiro *) malloc(sizeof(Passageiro));
    pas3->id = pas1->id;
    pas3->aviao_id = pas1->aviao_id;
    pas3->improprio = pas1->improprio;
    
    queue_insert(q, pas3);
    pas3 = queue_pop(q);

    Passageiro *pas_null = queue_pop(q);

    printf("Passageiro %d deseja voar para %d.\n", pas2->id, pas2->aviao_id);
    printf("Passageiro %d deseja voar para %d.\n", pas3->id, pas3->aviao_id);
    printf("Passageiro %d deseja voar para %d.\n", pas_null->id, pas_null->aviao_id);

    free(pas1);
    free(pas2);
    free(pas3);

    queue_destruct(q);

    return 0;
}