/***************************************************************************//**
 * @file main.c
 * @brief main() function.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include "app.h"
#include "initializer.h"
#include "settings.h"
#include <stdio.h>

volatile uint32_t msTicks;
uint8_t difficulty = 1;
int32_t lastSpawn;

typedef enum {
    GAME_MENU = 2,
    GAME_PLAY = 1,
    GAME_END  = 3
} GameState;


int main(void) {
    app_init();

    static uint32_t lastTick = 0;
    GameState gamestate = GAME_MENU;

    //printf("Welcome to duck hunt! To hunt ducks, press 's'. To change difficulty press '+' or '-'\r\n");

    while (1) {
        uint32_t now = msTicks;

        if ((now - lastTick) >= GAME_TICK_MS) {
            lastTick = now;
            switch (gamestate) {
                case GAME_MENU:
                    gamestate = difficulty_selection();
                    break;
                case GAME_PLAY:
                    gamestate = app_process_action();
                    break;
                case GAME_END:
                    gamestate = end_message();
                    break;
                default:
                    gamestate = GAME_MENU;
                    break;
            }
        }
    }
}
