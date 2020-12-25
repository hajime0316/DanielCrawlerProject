/*
 * cppmain.cpp
 *
 *  Created on: Dec 25, 2020
 *      Author: hajime
 */

#include <stdio.h>

#include "cppmain.hpp"

#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "can.h"

static const int MCU_ID = 0X0A;

static int global_divided_encoder_count = 0;

// モジュールのインクルード
#include "stm32_easy_can/stm32_easy_can.h"

void setup(void) {
  // stm32_easy_canモジュールの初期化
  stm32_easy_can_init(&hcan, MCU_ID, 0X7FF);
  // エンコーダスタート
  HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);
  // タイマスタート
  HAL_TIM_Base_Start_IT(&htim1);
}

void loop(void) {
  printf("%d\r\n", global_divided_encoder_count);
}

//**************************
//    タイマ割り込み関数
//**************************
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  // 5msecタイマ
  if(htim->Instance == TIM1) {
    // エンコーダの値の計算
    static int last_encoder_count = TIM4->CNT;
    int encoder_count = TIM4->CNT;
    int divided_encoder_count = encoder_count - last_encoder_count;
    if(divided_encoder_count > MAX_ENCODER_COUNT / 2) {
      divided_encoder_count -= (MAX_ENCODER_COUNT + 1);
    }
    else if(divided_encoder_count < -(MAX_ENCODER_COUNT / 2)) {
      divided_encoder_count += (MAX_ENCODER_COUNT + 1);
    }
    last_encoder_count = encoder_count;

    global_divided_encoder_count = divided_encoder_count; // デバッグ用

    // 送信データ生成->送信
    int id = (1 << 10) | MCU_ID;
    unsigned char message[2];
    message[0] = divided_encoder_count >> 8 & 0XFF;
    message[1] = divided_encoder_count & 0XFF;
    stm32_easy_can_transmit_message(id, sizeof(message), message);

    // デバッグ用に緑LEDを点滅
    static int i = 0;
    if(i >= 20) {
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_14);
      i = 0;
    }
    else {
      i++;
    }
  }
}

//**************************
//    CAN通信受信割り込み
//**************************
void stm32_easy_can_interrupt_handler(void)
{
}
