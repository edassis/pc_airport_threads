/**
 * @file aeroporto.h
 * @author Eduardo F. Assis (eduardoffassis@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-11-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef _AEROPORTO_H_
#define _AEROPORTO_H_

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

/**
 * @brief Estrutura dos aviões.
 * 
 */
typedef struct Avioes {
    int id;
    int max_carga;      /**< quantas pessoas pode carregar */
    int carga;          /**< carga atual */
    int tempo_voo;      /**< quanto tempo falta para retornar ao aeroporto */
    int tempo_espera;   /**< quanto tempo até decolar */
} Aviao;

#endif