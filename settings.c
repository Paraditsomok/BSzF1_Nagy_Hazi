/*
 * settings.c
 *
 *  Created on: 16 Nov 2025
 *      Author: galnandor
 */




#include "app.h"
#include "initializer.h"

#include <stdio.h>

extern uint8_t difficulty;
extern uint32_t lastSpawn;
extern volatile uint32_t msTicks;
extern volatile uint8_t score;
extern volatile uint8_t DucksDecayed;
extern volatile uint8_t MaxDucks;





/***************************************************************************//**
 * Taking inputs and handling characters
 ******************************************************************************/


bool difficulty_set(void){
  int ch=-1;
  ch = USART_RxNonblocking(UART0);
  if(ch=='+'&&difficulty<3) difficulty++;
  else if(ch=='-'&&difficulty>1) difficulty--;
  else if(ch=='s') return true;
  return false;
}

/***************************************************************************//**
 * Writing the difficulty on screen and sending back the difficulty on UART
 ******************************************************************************/


int difficulty_selection(void){
  int lastDifficulty=difficulty;
  while(!difficulty_set()){
      if(lastDifficulty!=difficulty){
          lastDifficulty=difficulty;
          printf("B%d\r\n",difficulty);
      }
  }
  Delay(1000);
  lastSpawn=msTicks;
  return 1;
}


int end_message(void){
  //printf("Game Over! Your score: %d \r\nPress 's' to play again! \r\n", score);
  printf("A%d\r\n",score);
  score=0;
  DucksDecayed=25;
  MaxDucks=25;
  return 2;
}
