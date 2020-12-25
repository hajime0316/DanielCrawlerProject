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

static int global_division_encoder_count = 0;

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
  printf("%d\r\n", global_division_encoder_count);
}

//**************************
//    タイマ割り込み関数
//**************************
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  // 10msecタイマ
  if(htim->Instance == TIM1) {
    // エンコーダの値の計算
    static int last_encoder_count = TIM4->CNT;
    int encoder_count = TIM4->CNT;
    int division_encoder_count = encoder_count - last_encoder_count;
    if(division_encoder_count > MAX_ENCODER_COUNT / 2) {
      division_encoder_count -= (MAX_ENCODER_COUNT + 1);
    }
    else if(division_encoder_count < -(MAX_ENCODER_COUNT / 2)) {
      division_encoder_count += (MAX_ENCODER_COUNT + 1);
    }
    last_encoder_count = encoder_count;

    global_division_encoder_count = division_encoder_count; // デバッグ用

    // 送信データ生成->送信
    int id = (1 << 10) | MCU_ID;
    unsigned char message[2];
    message[0] = division_encoder_count >> 8 & 0XFF;
    message[1] = division_encoder_count & 0XFF;
    stm32_easy_can_transmit_message(id, sizeof(message), message);

    // デバッグ用に緑LEDを点滅
    static int i = 0;
    if(i >= 100) {
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
//void stm32_easy_can_interrupt_handler(void)
//{
//  int receive_id;
//  int receive_dlc;
//  unsigned char receive_message[8];
//
//  // 受信データ取得
//  stm32_easy_can_get_receive_message(&receive_id, &receive_dlc, receive_message);
//
//  // 受信データ処理
//  MdDataType receive_md_data_type
//    = canmd_manager_set_can_receive_data(receive_message, receive_dlc);
//
//  // 送信データ生成
//  int transmit_id;
//  int transmit_dlc;
//  unsigned char transmit_message[8];
//
//  // CAN通信の送信ID生成
//  transmit_id = md_id   << 5 | 0b00000 ;
//  //            送信元ID(5bit)  送信先ID(5bit)
//
//  if(receive_md_data_type != MD_DATA_TYPE_MOTOR_CONTROL_DATA) {
//    // 受信メッセージをそのまま送信メッセージとする
//    transmit_dlc = receive_dlc;
//    for(int i = 0; i < receive_dlc; i++) {
//       transmit_message[i] = receive_message[i];
//    }
//  }
//  // データ送信
//  stm32_easy_can_transmit_message(transmit_id, transmit_dlc, transmit_message);
//
//  return;
//}
