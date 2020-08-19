/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SHT2x_ADDR              0x40<<1
#define SHT2x_HOLD_MASTER_T     0xE3
#define SHT2x_HOLD_MASTER_RH    0xE5
#define SHT2x_NOHOLD_MASTER_T   0xF3
#define SHT2x_NOHOLD_MASTER_RH  0xF5
#define SHT2x_WRITE_USER_REG    0xE6
#define SHT2x_READ_USER_REG     0xE7
#define SHT2x_SOFT_RESET        0xFE
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t rxData; // USART 입력값
unsigned char menu = 0;
unsigned char pianoN = 0;
uint8_t txBuf[] = "+++ Menu +++\n\r L : LED \n\r C : CDS \n\r 0~7 : PIANO \n\r T : temperature \n\r push button : stop \n\r";
uint8_t rxBuf[104];

// 온습도
uint8_t i2cData[2];
uint16_t val = 0;
uint8_t mode;
float TEMP, HUMI;
char str[16];

// CDS
uint16_t adcData = 0;
char cds[10];

unsigned char Time_STOP = 0;

unsigned int DoReMi[8] = {523, 587, 659, 698, 783, 880, 987, 1046};
unsigned long pwmfreq;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
int fputc(int ch, FILE *stream)
{
  HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, 10);
  return ch;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint16_t CCR1_val = 0;
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
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1, &rxData, 1); // main에서 인터럽트를 한번 실행(open) 해줘야 동작함
  
  HAL_UART_Receive_DMA(&huart1, rxBuf, sizeof(rxBuf));
  HAL_UART_Transmit_DMA(&huart1, txBuf, sizeof(txBuf));
  
  // CDS
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adcData, sizeof(adcData));
  
  lcdInit();
  lcdGotoXY(0, 0);
  
  // TIM2
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); // 스타트를 시켜줘야함
  
  // TIM9
  HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3); // 스타트를 시켜줘야함

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    
    if(menu == 1)
    {
      TIM2->CCR1 = CCR1_val;
      CCR1_val++;
      if(CCR1_val > 100) CCR1_val = 0;
      HAL_Delay(10);
    }
     else if(menu == 2)
    {
      lcdClear();
      lcdPrintData("ADC POLLTEST", 12);
      sprintf(cds, "ADC : %d", adcData);
      lcdGotoXY(0, 1);
      lcdPrint(cds);
      HAL_Delay(100);
    }     
    else if (menu == 3)
    {
      lcdClear();
      mode = SHT2x_NOHOLD_MASTER_T;
      HAL_I2C_Master_Transmit(&hi2c1, SHT2x_ADDR, &mode, 1, 10);
      HAL_Delay(100);
      HAL_I2C_Master_Receive(&hi2c1, SHT2x_ADDR, i2cData, 2, 10);
      val = i2cData[0] << 8| i2cData[1];
      TEMP = -46.85 + 175.72 * ((float)val / 65536);
      lcdGotoXY(0,0);
      sprintf(str, "TEMP : %5.2lf", TEMP);
      lcdPrint(str);
    
      mode = SHT2x_NOHOLD_MASTER_RH;
      HAL_I2C_Master_Transmit(&hi2c1, SHT2x_ADDR, &mode, 1, 10);
      HAL_Delay(100);
      HAL_I2C_Master_Receive(&hi2c1, SHT2x_ADDR, i2cData, 2, 10);
      val = i2cData[0] << 8| i2cData[1];
      HUMI = -6 + 125 * ((float)val / 65536);
      lcdGotoXY(0,1);
      sprintf(str, "HUMI : %5.2lf", HUMI);
      lcdPrint(str);
    }
    
    if((pianoN >= '0') && (pianoN <= '7')) { 
      printf("pianoN : %d ", pianoN);
      TIM5->EGR = TIM5->EGR & 0xFE;
      pwmfreq = 84000000/DoReMi[pianoN-'0'];
      TIM5->ARR = pwmfreq-1;
      TIM5->CCR3 = pwmfreq/2;
      HAL_Delay(100);
      TIM5->CCR3 = 0;
      TIM5->EGR = TIM9->EGR | 0x01;
      pianoN = 9;
    }
    
    if(Time_STOP == 1)
    {
      printf("stop");
      menu = 0;
      lcdClear();
      TIM2->CCR1 = 0;
      TIM5->CCR3 = 0;
      Time_STOP = 0;
    }
    
    HAL_Delay(250);
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
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
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

/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance == USART1)
  {
    HAL_UART_Receive_IT(&huart1, &rxData, 1);
    HAL_UART_Transmit(&huart1, &rxData, 1, 10);
    
    if(rxData == 'L') menu = 1;
    else if(rxData == 'C') menu = 2;
    else if((rxData >= '0') && (rxData <= '7')) pianoN = rxData;
    else if(rxData == 'T') menu = 3;
  }
}

// 인터럽트
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12)){ // LED제어, 스위치 1
    Time_STOP = 1;
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
