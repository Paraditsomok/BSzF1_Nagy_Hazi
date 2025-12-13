/***************************************************************************//**
 * @file
 * @brief Top level application functions
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


/***************************************************************************//**
 * @file     app.c
 * @brief    EFM32 Segment LCD Display driver extension demonstration
 *
 * @details  This application demonstrates the usage of the Segment LCD driver
 *           extension by individually turning on and off the segments of the
 *           lower (7 alphanumeric characters) and the upper (4 seven-segment
 *           digits) displays.
 *
 * Created on:  2023. may. 06.
 *
 * Updated on: 2024. Sept. 20. (Orosz Gyorgy <orosz@mit.bme.hu>)
 *                - touch slider added
 *
 * Author:      NASZALY Gabor <naszaly@mit.bme.hu>
 ******************************************************************************/


/***************************************************************************//**
 * Includes
 ******************************************************************************/

/*
 * Header for the SegmentLCD driver extension
 */
#include "segmentlcd_individual.h"

/*
 * "segmentlcd.h" is also required, as the SegmentLCD driver extension does not
 * provide any initialization functions. We use the base SegmentLCD driver to
 * to initialize the display.
 */
#include "segmentlcd.h"

/*
 * "sl_udelay.h" is used only by the demo functions to slow things down.
 * Otherwise it is not required to use the SegmentLCD driver extension.
 */
#include <sl_udelay.h>

/*
 * Source of UART0
 */
#include "initializer.h"

/*
 * Necessary for random number generation
 */
#include <stdlib.h>
#include <stdio.h>

#include "app.h"

/***************************************************************************//**
 * Globals
 ******************************************************************************/

#define INVERTED_VELOCITY 2
#define MAX_BULLETS 3
#define SHOOTING_COOLDOWN 5
#define SCREEN_WIDTH 4 //It's 0,1,2,3 so 4
#define SPAWNRATE 2400 //After every 4000 ticks a duck tries to spawn
/*
 * SegmentLCD_UpperSegments() and SegmentLCD_LowerSegments() are used to update
 * the display with new data. Display data is expected by these functions to be
 * located in arrays with SegmentLCD_UpperCharSegments_TypeDef and
 * SegmentLCD_LowerCharSegments_TypeDef elements, respectively.
 */
SegmentLCD_UpperCharSegments_TypeDef upperCharSegments[SEGMENT_LCD_NUM_OF_UPPER_CHARS];
SegmentLCD_LowerCharSegments_TypeDef lowerCharSegments[SEGMENT_LCD_NUM_OF_LOWER_CHARS];



/*
 * Global variables
 */
 /* counts 1ms timeTicks */
volatile uint8_t MaxDucks=25;
volatile uint8_t DucksDecayed=25;
volatile uint8_t score=0;
//Comes from main/set in settings
extern uint8_t difficulty;
extern volatile uint32_t msTicks;

//Player
struct Hunter{
  int x;
  int cooldown;
  //The y of hunter is always 0
};

//Bullet
struct Bullet{
  int x;
  int y;
  int ticksBeforeMovement; //Amount of ticks needed, to move 1 digit forward
};

//Ducks
struct Duck{
  int x;
  int lifeTime;
  int hit;
  int y; //for bullet reference it's here but not necessary
};


//Actual player initialization
struct Hunter Player={.x=0, .cooldown=1};

//Circular buffer
struct Bullet Bullets[MAX_BULLETS];
uint8_t lastBullet=0;

//Circular duck buffer
struct Duck Ducks[SCREEN_WIDTH];
extern int32_t lastSpawn; //wait a bit before first spawn
/***************************************************************************//**
 * Function definitions
 ******************************************************************************/


/***************************************************************************//**
 * @brief SysTick_Handler
 *   Interrupt Service Routine for system tick counter
 * @note
 *   No wrap around protection
 ******************************************************************************/
void SysTick_Handler(void)
{
  msTicks++;       /* increment counter necessary in Delay()*/
}

/***************************************************************************//**
 * @brief Delays number of msTick Systicks (typically 1 ms)
 * @param dlyTicks Number of ticks to delay
 ******************************************************************************/
void Delay(uint32_t dlyTicks)
{
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks) ;
}


/***************************************************************************//**
 * non blocking UART input taking function
 ******************************************************************************/
//Simple non-blocking UART
int USART_RxNonblocking(USART_TypeDef *usart)
{
  if (usart->STATUS & USART_STATUS_RXDATAV) {
      return (int)(usart->RXDATA);
  }
  else {
      return -1;
  }
}

/***************************************************************************//**
 * Movement, and update functions
 ******************************************************************************/

/***************************************************************************//**
 * Initiating bullet for player, see call function later
 ******************************************************************************/


/***************************************************************************//**
 * Handles all movement, and shooting for player
 ******************************************************************************/
void updatePlayer(void){
  int ch;
  ch = USART_RxNonblocking(UART0); //Non blocking UART input gets copied into ch
  if (ch == -1) return; //invalid input
  if (ch == 'b' && Player.x>0) { //b stands for left, and it's clamped
      Player.x -=1;

  }
  else if (ch == 'j' && Player.x<SCREEN_WIDTH-1) { //j stands for right, as per the exercise the right limit is the 4th digit
      Player.x +=1;
  }
  else if (ch=='a' && Player.cooldown==0){ //a stands for shoot, cooldown is needed, so people can't just spam right/left+shoot to win
      Player.cooldown=SHOOTING_COOLDOWN;
      Bullets[lastBullet]=(struct Bullet){.x=Player.x, .y=1, .ticksBeforeMovement=INVERTED_VELOCITY}; //Bullets get created in a circular buffer (The hunter cd + bullet cap is great enough, to never make these overlap)
      lastBullet+=1;
      if(lastBullet>=3)lastBullet=0;
  }
}

/***************************************************************************//**
 * Moves bullets, after predefined ticks forward
 ******************************************************************************/
void updateBullets(void){
  for(int i=0;i<MAX_BULLETS; i++){
      if(Bullets[i].ticksBeforeMovement>0)
        Bullets[i].ticksBeforeMovement--; //Ugly name but quite descriptive, at every tick, we check whether a bullet is allowed to move
      else if(Bullets[i].y>0) { //No conditions, because y running out of bounds means nothing, bullets don't get destroyed on hit anyways.
          Bullets[i].y++;
          Bullets[i].ticksBeforeMovement=INVERTED_VELOCITY;
      }
  }
}



/***************************************************************************//**
 * Creating a duck after a given time at a random spot
 ******************************************************************************/

void createDucks(){
  if((msTicks-lastSpawn)>=SPAWNRATE){ //This runs on Real ticks
      uint8_t spawnspot=(rand()&0x03);
      if(Ducks[spawnspot].lifeTime==0 && MaxDucks!=0){
          MaxDucks-=1;
          Ducks[spawnspot]= (struct Duck) {.x=spawnspot, .lifeTime=60/difficulty, .hit=false, .y=3}; //This runs on gameTick, so duck life is actually 50*30/diff
          lastSpawn=msTicks;
      };
  }
}




/***************************************************************************//**
 * Clears and redraws the display at the end of ticks
 ******************************************************************************/
void updateDisplay(void){
  //delete segments
  for (uint8_t p = 0; p < SEGMENT_LCD_NUM_OF_LOWER_CHARS; p++) {
     for (uint8_t s = 0; s < 15; s++) {
        lowerCharSegments[p].raw = 1 << s;
     }
  }

  //hunter location
  lowerCharSegments[Player.x].d = 1;

  //bullet location
  for(int i=0;i<MAX_BULLETS;i++){
      if(Bullets[i].y==1)lowerCharSegments[Bullets[i].x].p = 1;
      if(Bullets[i].y==2)lowerCharSegments[Bullets[i].x].j = 1;
  }

  //duck location
  for(int i=0;i<SCREEN_WIDTH;i++){
      if(Ducks[i].lifeTime>0 && (Ducks[i].hit==false || (Ducks[i].hit==true && (Ducks[i].lifeTime&0x02))))lowerCharSegments[Ducks[i].x].a = 1;
  }
  //draw LCD

  SegmentLCD_Number((MaxDucks*100)+score); //correct score display
  SegmentLCD_LowerSegments(lowerCharSegments);


}


/***************************************************************************//**
 * Checks duck and bullet collision in y=3
 ******************************************************************************/
void checkCollision(void){
  for(int i=0; i<SCREEN_WIDTH;i++){
      for(int j=0;j<MAX_BULLETS;j++){
          if(Bullets[j].y==Ducks[i].y && Bullets[j].x==Ducks[i].x && Ducks[i].hit==false && Ducks[i].lifeTime > 0){
              Ducks[i].hit=true;
              Ducks[i].lifeTime=9; //The vanishing animation makes it so every 2 game ticks its on and for 2 it's off. See above
              score+=1;
          }
      }
  }
}


/***************************************************************************//**
 * Calculates game ticks, and initiates updates. This is the "main" loop
 ******************************************************************************/

int app_process_action(void)
{
  if(DucksDecayed!=0){
          //Run game update
          updatePlayer();
          updateBullets();
          createDucks();
          checkCollision();
          if(Player.cooldown>0)Player.cooldown--;
          for(int i=0;i<SCREEN_WIDTH;i++){
              if(Ducks[i].lifeTime>0){
                  Ducks[i].lifeTime-=1;
                  if(Ducks[i].lifeTime==0){
                      Ducks[i].hit=false;
                      DucksDecayed-=1;
                  }
              }

          }
          updateDisplay(); //Ha egy kacsát eltávolítunk, akkor az meg is jelenjen a képernyőn!
          return 1;
  }
  else return 3; //to indicate the game is finished
}


