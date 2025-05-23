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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "cmox_crypto.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LENGTH_OF_SEED_KEY 16
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* Definitions for serialTask */
osThreadId_t serialTaskHandle;
const osThreadAttr_t serialTask_attributes = {
  .name = "serialTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for requestSeedTask */
osThreadId_t requestSeedTaskHandle;
const osThreadAttr_t requestSeedTask_attributes = {
  .name = "requestSeedTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for compareKeyTask */
osThreadId_t compareKeyTaskHandle;
const osThreadAttr_t compareKeyTask_attributes = {
  .name = "compareKeyTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for outputQueue */
osMessageQueueId_t outputQueueHandle;
const osMessageQueueAttr_t outputQueue_attributes = {
  .name = "outputQueue"
};
/* Definitions for requestSeedBinarySem */
osSemaphoreId_t requestSeedBinarySemHandle;
const osSemaphoreAttr_t requestSeedBinarySem_attributes = {
  .name = "requestSeedBinarySem"
};
/* Definitions for sendKeyBinarySem */
osSemaphoreId_t sendKeyBinarySemHandle;
const osSemaphoreAttr_t sendKeyBinarySem_attributes = {
  .name = "sendKeyBinarySem"
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void SerialTask(void *argument);
void RequestSeedTask(void *argument);
void CompareKeyTask(void *argument);

/* USER CODE BEGIN PFP */
void HandleInputKey(char* inputKey, uint8_t len);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
char inputMsg[40];
char requestSeed[] = "seed";
char sendKey[] = "key";
//const char testKey[LENGTH_OF_SEED_KEY*2+3] = "key77384fd183cb1916e7ac4c0fe022efcf";
const uint8_t SecretKey[] =
{
  0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};
const uint8_t IV[] =
{
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};
uint8_t InputCiphertext[] =
{
  0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46, 0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d
};
/* Computed data buffer */
uint8_t ComputedCiphertext[LENGTH_OF_SEED_KEY];

/* DRBG context handle */
cmox_ctr_drbg_handle_t Drbg_Ctx;

uint8_t Entropy[] =
{
  0x4c, 0xfb, 0x21, 0x86, 0x73, 0x34, 0x6d, 0x9d, 0x50, 0xc9, 0x22, 0xe4, 0x9b, 0x0d, 0xfc, 0xd0,
  0x90, 0xad, 0xf0, 0x4f, 0x5c, 0x3b, 0xa4, 0x73, 0x27, 0xdf, 0xcd, 0x6f, 0xa6, 0x3a, 0x78, 0x5c
};
const uint8_t Nonce[] =
{
  0x01, 0x69, 0x62, 0xa7, 0xfd, 0x27, 0x87, 0xa2, 0x4b, 0xf6, 0xbe, 0x47, 0xef, 0x37, 0x83, 0xf1
};
const uint8_t Personalization[] =
{
  0x88, 0xee, 0xb8, 0xe0, 0xe8, 0x3b, 0xf3, 0x29, 0x4b, 0xda, 0xcd, 0x60, 0x99, 0xeb, 0xe4, 0xbf,
  0x55, 0xec, 0xd9, 0x11, 0x3f, 0x71, 0xe5, 0xeb, 0xcb, 0x45, 0x75, 0xf3, 0xd6, 0xa6, 0x8a, 0x6b
};
uint8_t EntropyInputReseed[] =
{
  0xb7, 0xec, 0x46, 0x07, 0x23, 0x63, 0x83, 0x4a, 0x1b, 0x01, 0x33, 0xf2, 0xc2, 0x38, 0x91, 0xdb,
  0x4f, 0x11, 0xa6, 0x86, 0x51, 0xf2, 0x3e, 0x3a, 0x8b, 0x1f, 0xdc, 0x03, 0xb1, 0x92, 0xc7, 0xe7
};
const uint8_t Known_Random[] =
{
  0xa5, 0x51, 0x80, 0xa1, 0x90, 0xbe, 0xf3, 0xad, 0xaf, 0x28, 0xf6, 0xb7, 0x95, 0xe9, 0xf1, 0xf3
};

/* Computed data buffer */
uint8_t ComputedRandom[LENGTH_OF_SEED_KEY];
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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  HAL_UART_Transmit(&huart2, (uint8_t*)"Startttttttttt\n", (uint16_t)15, 100);

  cmox_init_arg_t init_target = {CMOX_INIT_TARGET_AUTO, NULL};

  /* Initialize cryptographic library */
  if (cmox_initialize(&init_target) != CMOX_INIT_SUCCESS)
  {
    Error_Handler();
  }

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of requestSeedBinarySem */
  requestSeedBinarySemHandle = osSemaphoreNew(1, 0, &requestSeedBinarySem_attributes);

  /* creation of sendKeyBinarySem */
  sendKeyBinarySemHandle = osSemaphoreNew(1, 0, &sendKeyBinarySem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of outputQueue */
  outputQueueHandle = osMessageQueueNew (2, 33, &outputQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of serialTask */
  serialTaskHandle = osThreadNew(SerialTask, NULL, &serialTask_attributes);

  /* creation of requestSeedTask */
  requestSeedTaskHandle = osThreadNew(RequestSeedTask, NULL, &requestSeedTask_attributes);

  /* creation of compareKeyTask */
  compareKeyTaskHandle = osThreadNew(CompareKeyTask, NULL, &compareKeyTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HandleInputKey(char* inputKey, uint8_t len) {
	uint8_t i = 0;
	uint8_t convertedValue;
	char hex[3];
	hex[2] = '\0'; // add null character...

	memset(InputCiphertext, 0, sizeof(uint8_t)*LENGTH_OF_SEED_KEY);

	for (i=0; i<len; i+=2) {
		(void)strncpy(hex, &inputKey[i], 2);
		convertedValue = (uint8_t)strtoul(hex, NULL, 16);
		InputCiphertext[i/2] = convertedValue;
	}
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_SerialTask */
/**
  * @brief  Function implementing the serialTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_SerialTask */
void SerialTask(void *argument)
{
  /* USER CODE BEGIN 5 */
	char c;
	size_t idx = 0;
	memset(inputMsg, 0, sizeof(char)*40); //reset buffer
	char msg[33];
	osStatus_t resultGetFromQ;

  /* Infinite loop */
  for(;;)
  {
	  if (HAL_UART_Receive(&huart2, (uint8_t*)&c, (uint16_t)1, 10) == HAL_OK) { //handle for input
		  if ((c == '\n') || (c == '\r')) {
			  if (strcmp(inputMsg, requestSeed) == 0) {
				  osSemaphoreRelease(requestSeedBinarySemHandle);
			  }
			  else if (strncmp(inputMsg, sendKey, 3) == 0) {
				  HandleInputKey(&inputMsg[3], (uint8_t)strlen(&inputMsg[3]));
				  osSemaphoreRelease(sendKeyBinarySemHandle);
			  }
			  else {
				  inputMsg[idx++] = '\n';
				  inputMsg[idx] = '\r';
				  HAL_UART_Transmit(&huart2, (uint8_t*)inputMsg, (uint16_t)strlen(inputMsg), 10);
			  }
			  idx = 0; //reset idx
			  memset(inputMsg, 0, sizeof(char)*40); //reset buffer
		  }
		  else {
			  inputMsg[idx++] = c;
		  }
	  }
	  //handle for output
	  if (osMessageQueueGetCount(outputQueueHandle) > 0)
	  {
		  resultGetFromQ = osMessageQueueGet(outputQueueHandle, &msg[0], NULL, 0);
		  if (resultGetFromQ == osOK) {
			  HAL_UART_Transmit(&huart2, (uint8_t*)&msg[0], (uint16_t)strlen(msg), 10);
			  HAL_UART_Transmit(&huart2, (uint8_t*)"\n", 1, 10);
		  } else {
			  ITM_SendChar(resultGetFromQ+6+48);
			  ITM_SendChar('\n');
		  }
	  }
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_RequestSeedTask */
/**
* @brief Function implementing the requestSeedTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_RequestSeedTask */
void RequestSeedTask(void *argument)
{
  /* USER CODE BEGIN RequestSeedTask */

	cmox_drbg_retval_t retval;
	/* General DRBG context */
	cmox_drbg_handle_t *drgb_ctx;
	size_t i = 0;
	char msg[33];

	/* Infinite loop */
	for(;;)
	{
		if (osSemaphoreAcquire(requestSeedBinarySemHandle, osWaitForever) == osOK) {
		  /* Construct a drbg context that is configured to perform ctrDRBG with AES256 operations */
		  drgb_ctx = cmox_ctr_drbg_construct(&Drbg_Ctx, CMOX_CTR_DRBG_AES256);
		  if (drgb_ctx == NULL)
		  {
			  Error_Handler();
		  }

		  /* Initialize the DRBG context with entropy, nonce and personalization string parameters */
		  Entropy[0] = (uint8_t)HAL_GetTick();
		  retval = cmox_drbg_init(drgb_ctx,                                     /* DRBG context */
								Entropy, sizeof(Entropy),                     /* Entropy data */
								Personalization, sizeof(Personalization),     /* Personalization string */
								Nonce, sizeof(Nonce));                        /* Nonce data */

		  if (retval != CMOX_DRBG_SUCCESS)
		  {
			  Error_Handler();
		  }

		  /* Reseed the DRBG with reseed parameters */
		  EntropyInputReseed[0] = (uint8_t)HAL_GetTick();
		  retval = cmox_drbg_reseed(drgb_ctx,                                           /* DRBG context */
									EntropyInputReseed, sizeof(EntropyInputReseed),     /* Entropy reseed data */
									NULL, 0);

		  /* Generate 1st random data */
		  memset(ComputedRandom, 0, sizeof(uint8_t)*LENGTH_OF_SEED_KEY);
		  retval = cmox_drbg_generate(drgb_ctx,                             /* DRBG context */
									NULL, 0,                                /* No additional data */
									ComputedRandom, LENGTH_OF_SEED_KEY-1);  /* Data buffer to receive generated random */

		  /* Verify API returned value */
		  if (retval != CMOX_DRBG_SUCCESS)
		  {
			  Error_Handler();
		  }

		  /* Cleanup the context */
		  retval = cmox_drbg_cleanup(drgb_ctx);
		  if (retval != CMOX_DRBG_SUCCESS)
		  {
			  Error_Handler();
		  }

		  for (i=0; i<LENGTH_OF_SEED_KEY-1; i++) {
			  (void)sprintf(&msg[2*i], "%02X", ComputedRandom[i]);
		  }
		  (void)osMessageQueuePut(outputQueueHandle, &msg[0], 0, 0);
		  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
	  }
	}
  /* USER CODE END RequestSeedTask */
}

/* USER CODE BEGIN Header_CompareKeyTask */
/**
* @brief Function implementing the compareKeyTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_CompareKeyTask */
void CompareKeyTask(void *argument)
{
  /* USER CODE BEGIN CompareKeyTask */

  cmox_cipher_retval_t retval;
  size_t computedSize;
  char msg[33];

  /* Infinite loop */
  for(;;)
  {
	  /* Compute directly the ciphertext passing all the needed parameters */
	  /* Note: CMOX_AES_CBC_ENC_ALGO refer to the default AES implementation
	   * selected in cmox_default_config.h. To use a specific implementation, user can
	   * directly choose:
	   * - CMOX_AESFAST_CBC_ENC_ALGO to select the AES fast implementation
	   * - CMOX_AESSMALL_CBC_ENC_ALGO to select the AES small implementation
	   */
	  if (osSemaphoreAcquire(sendKeyBinarySemHandle, osWaitForever) == osOK) {
		  memset(ComputedCiphertext, 0, sizeof(uint8_t)*LENGTH_OF_SEED_KEY);
		  ComputedRandom[15] = 0x01; //perform PKCS7 padding
		  retval = cmox_cipher_encrypt(CMOX_AES_CBC_ENC_ALGO,                  			/* Use AES ECB algorithm */
				  	  	  	  	  	   ComputedRandom, LENGTH_OF_SEED_KEY,           	/* Plaintext to encrypt */
									   SecretKey, LENGTH_OF_SEED_KEY,                   /* AES key to use */
									   IV, LENGTH_OF_SEED_KEY,                         	/* Initialization vector */
									   ComputedCiphertext, &computedSize);   			/* Data buffer to receive generated ciphertext */

		  /* Verify API returned value */
		  if (retval != CMOX_CIPHER_SUCCESS)
		  {
			  Error_Handler();
		  }

		  /* Verify generated data size is the expected one */
		  if (computedSize != LENGTH_OF_SEED_KEY)
		  {
			  Error_Handler();
		  }

		  /* Verify generated data are the expected ones */
		  if (memcmp(InputCiphertext, ComputedCiphertext, computedSize) != 0) {
			  (void)sprintf(msg, "Please don't hack me bro");
			  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
		  }
		  else {
			  (void)sprintf(msg, "We are good to go bro");
			  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
		  }

		  (void)osMessageQueuePut(outputQueueHandle, &msg[0], 0, 0);
	  }
  }
  /* USER CODE END CompareKeyTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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

#ifdef  USE_FULL_ASSERT
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
