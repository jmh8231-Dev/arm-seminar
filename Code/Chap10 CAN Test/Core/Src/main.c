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
#include "can.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "sdio.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_host.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "CLCD.h"
#include "VS1003.h"
#include "AS6221.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define AS6221_ADDR1        (0x48 << 1)
#define AS6221_ADDR2        (0x49 << 1)

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

volatile uint8_t can1_rx0_flag = 0;

uint32_t bytesWritten, bytesRead;
extern ApplicationTypeDef Appli_state;

uint8_t NEXT = 0;

uint8_t str[21];

AS6221_t as6221[2];

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
  MX_ADC2_Init();
  MX_TIM1_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  MX_TIM2_Init();
  MX_FATFS_Init();
  MX_USB_HOST_Init();
  MX_SDIO_SD_Init();
  MX_SPI1_Init();
  MX_I2C1_Init();
  MX_CAN1_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim6);
//  HAL_TIM_Base_Start_IT(&htim7);

  lcd_Init(20, 4);
  lcd_clear();

  as6221[0].address = AS6221_ADDR1;
  as6221[0].CR = ConvPer125ms;
  as6221[0].CF = Quadruple;
  as6221[0].SM = false;
  as6221[0].IM = false;
  as6221[0].POL = false;
  as6221[0].SS = false;

  as6221[1] = as6221[0];
  as6221[1].address = AS6221_ADDR2;

  AS6221_Init(&as6221[0]);
  AS6221_Init(&as6221[1]);

  // FilterScale을 16bit로 할 경우 상위 하위 데이터는 의미 없고 필터가 2쌍이 되는것임(32bit일 경우 유효해짐)
  canFilter1.FilterMaskIdHigh = 0x538 << 5;
  canFilter1.FilterMaskIdLow = 0x538 << 5;
  canFilter1.FilterIdHigh = 0x3AA << 5;
  canFilter1.FilterIdLow = 0x3AA << 5;
  canFilter1.FilterMode = CAN_FILTERMODE_IDMASK; // ID List, ID Mask 모드중 Mask 모드로 설정
  canFilter1.FilterScale = CAN_FILTERSCALE_16BIT; // 필터 스케일을 16bit로 설정
  canFilter1.FilterFIFOAssignment = CAN_FILTER_FIFO0; // FIFO 0번을 사용
  canFilter1.FilterBank = 0; // 0번 필터 규칙으로 설정
  canFilter1.FilterActivation = ENABLE; // 설정된 필터 값 활성화 여부 설정(즉시 활성화)

  HAL_CAN_ConfigFilter(&hcan1, &canFilter1); // 설정한 필터값 전달
  HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING); // CAN 수신 인터럽트 활성화
  HAL_CAN_Start(&hcan1); // CAN 버스 활성화

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */
    if(lcd_flag) {
    	lcd_flag = false;
		AS6221_ReadTemperature(&as6221[0]);
		AS6221_ReadTemperature(&as6221[1]);

		int16_t temp1_int = (int16_t)(as6221[0].Temp * 100);
		int16_t temp2_int = (int16_t)(as6221[1].Temp * 100);

		can1Tx0Data[0] = temp1_int & 0xFF; // Temp 1 하위 바이트
		can1Tx0Data[1] = (temp1_int >> 8) & 0xFF; // Temp 1 위 바이트

		can1Tx0Data[2] = temp2_int & 0xFF; // Temp 2 하위 바이트
		can1Tx0Data[3] = (temp2_int >> 8) & 0xFF; // Temp 2 하위 바이트
    }

    if(can1_rx0_flag) {
   		  can1_rx0_flag = false;

		  int16_t raw_temp1 = (can1Rx0Data[1] << 8) | can1Rx0Data[0];
		  int16_t raw_temp2 = (can1Rx0Data[3] << 8) | can1Rx0Data[2];


		  double f_temp1 = raw_temp1 / 100.0f;
		  double f_temp2 = raw_temp2 / 100.0f;


		  lcd_clear();

   		  sprintf(str, "Rx ID:0x%3X", canRxHeader.StdId);
   		  lcd_setCurStr(0, 0, str);

		  sprintf(str, "Temp1: %.2f C", f_temp1);
		  lcd_setCurStr(0, 1, str);

		  sprintf(str, "Temp2: %.2f C", f_temp2);
		  lcd_setCurStr(0, 2, str);
    }

    if(sw0_flag) {
    	sw0_flag = false;

    	lcd_clear();
    	canTxHeader.StdId = 0x1EB; // 수신 가능한 ID로 설정
    	canTxHeader.RTR = CAN_RTR_DATA; // 데이터 송신 혹은 요청 모드로 설정 가능
    	canTxHeader.IDE = CAN_ID_STD; // ID를 11bit Mode로 사용
    	canTxHeader.DLC = 4; // 데이터 길이 설정

    	// 실제로 보낼 데이터를 보내는 MailBox는 3개가 있는데 그중 Free한 메일박스를 찾는 함수
    	TxMailBox = HAL_CAN_GetTxMailboxesFreeLevel(&hcan1);

    	//앞서 설정한 구조체와 데이터 주소를 받아 실제로 데이터를 전송하는 함수
    	HAL_CAN_AddTxMessage(&hcan1, &canTxHeader, &can1Tx0Data[0], &TxMailBox);
    }
    else if(sw1_flag) {
    	sw1_flag = false;

    	lcd_clear();
    	canTxHeader.StdId = 0x6AD; // 수신 불가능한 ID로 설정
    	canTxHeader.RTR = CAN_RTR_DATA; // 데이터 송신 혹은 요청 모드로 설정 가능
    	canTxHeader.IDE = CAN_ID_STD; // ID를 11bit Mode로 사용
    	canTxHeader.DLC = 4; // 데이터 길이 설정

    	// 실제로 보낼 데이터를 보내는 MailBox는 3개가 있는데 그중 Free한 메일박스를 찾는 함수
    	TxMailBox = HAL_CAN_GetTxMailboxesFreeLevel(&hcan1);

    	//앞서 설정한 구조체와 데이터 주소를 받아 실제로 데이터를 전송하는 함수
    	HAL_CAN_AddTxMessage(&hcan1, &canTxHeader, &can1Tx0Data[0], &TxMailBox);
    }
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
  /* CAN1_RX0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
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

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if(hcan->Instance == CAN1)
	{
		HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &canRxHeader, &can1Rx0Data[0]);
		can1_rx0_flag = 1;
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
