/**
 * @file queue.h
 * @author Eduardo F. Assis (eduardoffassis@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-11-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "aeroporto.h"

typedef struct Elements {
    Passageiro *content;
    struct Elements *previous;
    struct Elements *next;
} Element;

typedef struct Queues {
    Element *first_element;
    Element *last_element;
    int size;
} Queue;

Queue *queue_init();
void queue_insert(Queue *queue, Passageiro *pas);
Passageiro *queue_pop(Queue *queue);
void queue_clear(Queue *queue);
void queue_destruct(Queue *queue);

#endif