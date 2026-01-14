/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <string.h>


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define RX_SZ   (512u)
#define TX_SZ   (512u)   /* RX_SZ 이상 권장 */

static uint8_t pc_rx_buf[2][RX_SZ];     /* USART3 RX ping-pong */
static uint8_t esp_rx_buf[2][RX_SZ];    /* UART4  RX ping-pong */
static volatile uint8_t pc_rx_sel  = 0u;
static volatile uint8_t esp_rx_sel = 0u;

/* PC->ESP (UART4 TX) : tx buffer 2개 + pending 1개 */
static uint8_t pc2esp_tx_buf[2][TX_SZ];
static volatile uint8_t  pc2esp_busy = 0u;
static volatile uint8_t  pc2esp_active_id = 0u;
static volatile uint8_t  pc2esp_pending = 0u;
static volatile uint8_t  pc2esp_pending_id = 0u;
static volatile uint16_t pc2esp_pending_len = 0u;

/* ESP->PC (USART3 TX) : tx buffer 2개 + pending 1개 */
static uint8_t esp2pc_tx_buf[2][TX_SZ];
static volatile uint8_t  esp2pc_busy = 0u;
static volatile uint8_t  esp2pc_active_id = 0u;
static volatile uint8_t  esp2pc_pending = 0u;
static volatile uint8_t  esp2pc_pending_id = 0u;
static volatile uint16_t esp2pc_pending_len = 0u;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */
static void Bridge_Start(void);
static void ESP_EnterBootloader(void);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void start_rx_usart3(void)
{
    (void)HAL_UARTEx_ReceiveToIdle_DMA(&huart3, pc_rx_buf[pc_rx_sel], RX_SZ);
    if (huart3.hdmarx) __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
}

static void start_rx_uart4(void)
{
    (void)HAL_UARTEx_ReceiveToIdle_DMA(&huart4, esp_rx_buf[esp_rx_sel], RX_SZ);
    if (huart4.hdmarx) __HAL_DMA_DISABLE_IT(huart4.hdmarx, DMA_IT_HT);
}

/* ESP8266 Download Mode: GPIO0=LOW, RST 펄스 */
static void ESP_EnterBootloader(void)
{

	if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0) == GPIO_PIN_RESET) {
		/* GPIO0(BOOT)=LOW */
		HAL_GPIO_WritePin(ESP12F_Boot_GPIO_Port, ESP12F_Boot_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LD0_GPIO_Port, LD0_Pin, GPIO_PIN_SET);
	}
	else {
		HAL_GPIO_WritePin(ESP12F_Boot_GPIO_Port, ESP12F_Boot_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LD0_GPIO_Port, LD0_Pin, GPIO_PIN_RESET);
	}

    /* RST: LOW->HIGH */
    HAL_GPIO_WritePin(ESP12F_RST_GPIO_Port, ESP12F_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(ESP12F_RST_GPIO_Port, ESP12F_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
}

static void Bridge_Start(void)
{
    pc_rx_sel  = 0u;
    esp_rx_sel = 0u;

    pc2esp_busy = 0u; pc2esp_pending = 0u;
    esp2pc_busy = 0u; esp2pc_pending = 0u;

    start_rx_usart3();
    start_rx_uart4();
}

/* RX 이벤트(Idle 또는 버퍼 full) */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (Size == 0u) return;

    if (huart->Instance == USART3) {
        /* --- PC(USART3) -> ESP(UART4) --- */
        uint8_t used_rx = pc_rx_sel;
        pc_rx_sel ^= 1u;
        start_rx_usart3(); /* 먼저 다음 RX 걸기 (다른 버퍼로) */

        if (Size > TX_SZ) Size = TX_SZ;

        if (!pc2esp_busy) {
            /* TX idle: active로 시작 */
            uint8_t txid = 0u;
            (void)memcpy(pc2esp_tx_buf[txid], pc_rx_buf[used_rx], Size);
            pc2esp_busy = 1u;
            pc2esp_active_id = txid;
            (void)HAL_UART_Transmit_DMA(&huart4, pc2esp_tx_buf[txid], Size);
        } else if (!pc2esp_pending) {
            /* TX busy: 반대 버퍼에 1개만 대기 */
            uint8_t txid = (uint8_t)(pc2esp_active_id ^ 1u);
            (void)memcpy(pc2esp_tx_buf[txid], pc_rx_buf[used_rx], Size);
            pc2esp_pending = 1u;
            pc2esp_pending_id = txid;
            pc2esp_pending_len = Size;
        } else {
            /* pending까지 차면 드롭(여기 오면 TX_SZ/RX_SZ 키우거나 baud 낮추면 됨) */
        }
    }
    else if (huart->Instance == UART4) {
        /* --- ESP(UART4) -> PC(USART3) --- */
        uint8_t used_rx = esp_rx_sel;
        esp_rx_sel ^= 1u;
        start_rx_uart4(); /* 다음 RX */

        if (Size > TX_SZ) Size = TX_SZ;

        if (!esp2pc_busy) {
            uint8_t txid = 0u;
            (void)memcpy(esp2pc_tx_buf[txid], esp_rx_buf[used_rx], Size);
            esp2pc_busy = 1u;
            esp2pc_active_id = txid;
            (void)HAL_UART_Transmit_DMA(&huart3, esp2pc_tx_buf[txid], Size);
        } else if (!esp2pc_pending) {
            uint8_t txid = (uint8_t)(esp2pc_active_id ^ 1u);
            (void)memcpy(esp2pc_tx_buf[txid], esp_rx_buf[used_rx], Size);
            esp2pc_pending = 1u;
            esp2pc_pending_id = txid;
            esp2pc_pending_len = Size;
        } else {
            /* drop */
        }
    }
}

/* TX 완료 콜백: pending 있으면 이어서 전송 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART4) {
        /* PC->ESP */
        if (pc2esp_pending) {
            pc2esp_pending = 0u;
            pc2esp_active_id = pc2esp_pending_id;
            (void)HAL_UART_Transmit_DMA(&huart4,
                                       pc2esp_tx_buf[pc2esp_active_id],
                                       pc2esp_pending_len);
        } else {
            pc2esp_busy = 0u;
        }
    }
    else if (huart->Instance == USART3) {
        /* ESP->PC */
        if (esp2pc_pending) {
            esp2pc_pending = 0u;
            esp2pc_active_id = esp2pc_pending_id;
            (void)HAL_UART_Transmit_DMA(&huart3,
                                       esp2pc_tx_buf[esp2pc_active_id],
                                       esp2pc_pending_len);
        } else {
            esp2pc_busy = 0u;
        }
    }
}

/* UART 에러 시 RX 재시작 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        start_rx_usart3();
    } else if (huart->Instance == UART4) {
        start_rx_uart4();
    }
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
  MX_UART4_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */
  ESP_EnterBootloader();  /* ESP를 항상 Download Mode로 넣고 시작 */
  Bridge_Start();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

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
  /* EXTI3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
}

/* USER CODE BEGIN 4 */

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
