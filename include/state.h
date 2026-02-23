/**
 * @file    states.h
 * @brief   Interface para definição dos estados
 */

#ifndef STATE_H
#define STATE_H

/**
 * @brief Enum para definir os possiveis estados principais da máquina
 */
typedef enum
{
    STATE_MENU,
    STATE_SETTINGS,
    STATE_TEST_RUNNING,
    STATE_TEST_FAIL,
    STATE_TEST_PASS,
} system_state_t;

#endif