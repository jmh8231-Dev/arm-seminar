/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "fatfs.h"
#include "sdio.h"
#include "tim.h"
#include "usart.h"
#include "usb_host.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "CLCD.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
int _write(int file, char* p, int len){
	HAL_UART_Transmit(&huart3, p, len, 10);
	return len;
}

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t rx_data = 0;
uint8_t lcd_flag = 0;

uint16_t adc1_val[3] = {0,};
uint16_t adc2_val = 0;

volatile uint8_t sw0_flag = 0;
volatile uint8_t sw1_flag = 0;
volatile uint8_t sw2_flag = 0;
volatile uint8_t sw3_flag = 0;

uint32_t bytesWritten, bytesRead;
extern ApplicationTypeDef Appli_state;

uint8_t str[21];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_NVIC_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART3_UART_Init();
  MX_TIM4_Init();
  MX_ADC1_Init();
  MX_DAC_Init();
  MX_ADC2_Init();
  MX_TIM1_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  MX_TIM2_Init();
  MX_FATFS_Init();
  MX_USB_HOST_Init();
  MX_SDIO_SD_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim6);
//  HAL_TIM_Base_Start_IT(&htim7);

  lcd_Init(4, 20);
  lcd_clear();

  BYTE buf[32] = "SDcard Write Test";
  uint32_t bw, br;

  // 1. SD "0" 드라이브 할당 시도
  if((retSD = f_mount(&SDFatFS, &SDPath[0], 1)) == FR_OK)
  {
	  sprintf(str, "f_mount OK %d", retSD);
	  lcd_setCurStr(0, 0, str);
  }
  else
  {
	  sprintf(str, "f_mount failed %d", retSD);
		  lcd_setCurStr(0, 0, str);
  }
  HAL_Delay(2000);
  lcd_clear();

  // 2 파일 생성 및 쓰기
    if((retSD = f_open(&SDFile, "0:/jmh8231.txt", FA_CREATE_NEW | FA_WRITE)) == FR_OK)
    {
//  	  sprintf(buf, "Hello world");
  	  f_write(&SDFile, buf, sizeof(buf), &bw);

  	  // bw로 써진 파일 사이즈가 반환됨
  	  sprintf(str, "%d bytes", bw);
  	  lcd_setCurStr(0, 0, str);

  	  f_close(&SDFile);
    }
    else
    {
  	  sprintf(str, "error %d\n", retSD);
  	  lcd_setCurStr(0, 0, str);
    }
    HAL_Delay(2000);
    lcd_clear();

    // 3 파일 읽기
    if((retSD = f_open(&SDFile, "0:/jmh8231.txt", FA_OPEN_EXISTING | FA_READ)) == FR_OK)
    {
  	  f_read(&SDFile, buf, 32, &br); // 32바이트를 읽어온다
  	  sprintf(str, "%s", buf);
  	  lcd_setCurStr(0, 0, str);

  	  sprintf(str, "%d bytes", br); // 실제로 몇 바이트가 읽혔는지 출력
  	  lcd_setCurStr(0, 1, str);
    }
    else
    {
  	  sprintf(str, "ERROR Code : %d", retSD);
  	  lcd_setCurStr(0, 0, str);
    }


//  lcd_clear();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* EXTI0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
  /* EXTI1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
  /* EXTI2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
  /* USART3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* TIM6_DAC_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
  /* TIM7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM7_IRQn);
  /* EXTI3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == GPIO_PIN_0) {
		HAL_GPIO_TogglePin(LD0_GPIO_Port, LD0_Pin);
		sw0_flag = 1;
	}
	else if(GPIO_Pin == GPIO_PIN_1) {
		sw1_flag = 1;
		HAL_GPIO_TogglePin(LD0_GPIO_Port, LD0_Pin);

	}
	else if(GPIO_Pin == GPIO_PIN_2) {
		sw2_flag = 1;
		HAL_GPIO_TogglePin(LD0_GPIO_Port, LD0_Pin);
	}
	else if(GPIO_Pin == GPIO_PIN_3) {
		sw3_flag = 1;
		HAL_GPIO_TogglePin(LD0_GPIO_Port, LD0_Pin);
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART3) {
		HAL_UART_Receive_IT(&huart3, &rx_data, 1);
		HAL_UART_Transmit(&huart3, &rx_data, 1, 1000);
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim -> Instance == TIM6) {
		lcd_flag = 1;
	}
	if(htim -> Instance == TIM7) {

	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
	  HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
	  HAL_Delay(1000);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
