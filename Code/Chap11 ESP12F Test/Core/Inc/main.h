/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ESP12F_Rx_Pin GPIO_PIN_0
#define ESP12F_Rx_GPIO_Port GPIOA
#define ESP12F_Tx_Pin GPIO_PIN_1
#define ESP12F_Tx_GPIO_Port GPIOA
#define ESP12F_Boot_Pin GPIO_PIN_0
#define ESP12F_Boot_GPIO_Port GPIOB
#define ESP12F_RST_Pin GPIO_PIN_2
#define ESP12F_RST_GPIO_Port GPIOB
#define Buzzer_Pin GPIO_PIN_9
#define Buzzer_GPIO_Port GPIOE
#define VS1003_CS_Pin GPIO_PIN_10
#define VS1003_CS_GPIO_Port GPIOE
#define VS1003_DREQ_Pin GPIO_PIN_11
#define VS1003_DREQ_GPIO_Port GPIOE
#define VS1003_xRESET_Pin GPIO_PIN_12
#define VS1003_xRESET_GPIO_Port GPIOE
#define VS1003_xDCS_Pin GPIO_PIN_13
#define VS1003_xDCS_GPIO_Port GPIOE
#define D7_Pin GPIO_PIN_10
#define D7_GPIO_Port GPIOD
#define D6_Pin GPIO_PIN_11
#define D6_GPIO_Port GPIOD
#define D5_Pin GPIO_PIN_12
#define D5_GPIO_Port GPIOD
#define D4_Pin GPIO_PIN_13
#define D4_GPIO_Port GPIOD
#define EN_Pin GPIO_PIN_14
#define EN_GPIO_Port GPIOD
#define RS_Pin GPIO_PIN_15
#define RS_GPIO_Port GPIOD
#define SDIO_WP_Pin GPIO_PIN_6
#define SDIO_WP_GPIO_Port GPIOC
#define SDIO_INT_Pin GPIO_PIN_7
#define SDIO_INT_GPIO_Port GPIOC
#define USB_OTG_FS_PSO_Pin GPIO_PIN_8
#define USB_OTG_FS_PSO_GPIO_Port GPIOA
#define Servo_Pin GPIO_PIN_3
#define Servo_GPIO_Port GPIOB
#define LD1_Pin GPIO_PIN_4
#define LD1_GPIO_Port GPIOB
#define LD0_Pin GPIO_PIN_5
#define LD0_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
