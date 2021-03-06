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
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <fatfs.h>

#include <string.h>
#include "spi_max10.h"
#include "log.h"
#include "misc_tests.h"

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
CRC_HandleTypeDef hcrc;

SD_HandleTypeDef hsd;

SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* File system object for SD card logical drive */
FATFS SDFatFs;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI2_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Redirect printf to UART
// Related information : http://www.openstm32.org/forumthread1346
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);

    return ch;
}
int _write(int file, char *ptr, int len)
{
    int DataIdx;

    for (DataIdx = 0; DataIdx < len; DataIdx++)
    {
        __io_putchar(*ptr++);
    }
    return len;
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
  log_init();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_SPI2_Init();
  MX_SDIO_SD_Init();
  MX_FATFS_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */

  /* Output a message on Hyperterminal using printf function */
  termout(WL_LOG_DEBUGNORMAL, "UART Printf Example: retarget the C library printf function to the UART");
  termout(WL_LOG_DEBUGNORMAL, "Build Date : %s, time : %s, for Nucleo F446RE.", __DATE__, __TIME__);


  /* Initialize SD card and FatFs. */
  termout(WL_LOG_DEBUGNORMAL, "retSD:%d SDPath:%c%c%c%c", retSD, SDPath[0], SDPath[1], SDPath[2], SDPath[3]);
  termout(WL_LOG_DEBUGNORMAL, "BSP_SD_IsDetected:%d", BSP_SD_IsDetected());
  if(retSD == 0)
  {
    /* success: set the orange LED on */
    // HAL_GPIO_WritePin(GPIOG, GPIO_PIN_7, GPIO_PIN_RESET);

    /*##-2- Register the file system object to the FatFs module ###*/
    termout(WL_LOG_DEBUGNORMAL, "FatFs Initialization STT ...");
    if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
    {
        /* FatFs Initialization Error : set the red LED on */
        termout(WL_LOG_DEBUGNORMAL, "FatFs Initialization Error !");
        //HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_RESET);
        //while(1);
    }
  }


#if 0
while(1) // TEST : see how good or bad SPI synchronization status is read from MAX10
{
//#define SPI_MAX10_RCV_HDR_READY() HAL_GPIO_WritePin(GPIOA, SPI_SYNC_Pin, GPIO_PIN_RESET)
//#define SPI_MAX10_SND_RSP_READY() HAL_GPIO_WritePin(GPIOA, SPI_SYNC_Pin, GPIO_PIN_SET  )
    int stateOfPushButton = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
    if(stateOfPushButton == GPIO_PIN_RESET)
    {
        HAL_GPIO_WritePin(SPI_SYNC_Port, SPI_SYNC_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(SPI_SYNC_Port, SPI_SYNC_Pin, GPIO_PIN_SET  );
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    }
}
#endif

  logout(WL_LOG_DEBUGNORMAL, "---");
  logout(WL_LOG_DEBUGNORMAL, "UART Printf Example: retarget the C library printf function to the UART");
  logout(WL_LOG_DEBUGNORMAL, "Build Date : %s, time : %s, for Nucleo F446RE.", __DATE__, __TIME__);

  /* Initialize SPI internals. */
  spi_init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  unsigned char logmsg_id = 0;

  /* Count the number of packets received between two time intervals. */
  unsigned long spi_init_cnt = 0;
  unsigned long prev_spi_init_cnt = spi_init_cnt;
  unsigned long prev_spi_init_tickcnt = HAL_GetTick();

  unsigned long prev_tick = HAL_GetTick();
  while (1)
  {
    wl_spi_pkt_t* trans_hdr = &_spi_trans_hdr;

    /* Display SPI communication status. */
    log_infos_t* log_info = &_log_inf_data;

    /* Compute the number of packets exchanged in one second. */
    unsigned long spi_init_tickcnt = HAL_GetTick();
    unsigned long spi_tick_delta = spi_init_tickcnt - prev_spi_init_tickcnt;
    if(spi_tick_delta == 0)
    {
        spi_tick_delta = 1;
    }

    unsigned long spi_packet_rate = spi_init_cnt - prev_spi_init_cnt;
    spi_packet_rate = (1000 * spi_packet_rate) / spi_tick_delta;

    /* Also indicate transfer speed in KB/s unit.
     * Note : as same data size is sent in both directions between
     *        MAX 10 and STM32, speed is indicated as "theoretical"
     *        maximum speed that would be reached with proper transmission
     *        protocol which (for example) would feature larger packet
     *        size from the device transferring the larger amount of data.
     */
    unsigned long spi_speed_kbps = spi_packet_rate * (WL_SPI_PARAMS_LEN * 2);
    spi_speed_kbps = spi_speed_kbps / 1024;

    prev_spi_init_cnt = spi_init_cnt;
    prev_spi_init_tickcnt = spi_init_tickcnt;

    trans_hdr->data[WL_SPI_DATA_LEN-1] = '\0';
    logout(WL_LOG_DEBUGNORMAL, "[%08X:%d] ID[%3u] PKT/s[%4u][%4u KB/s] [%s] [cbu:%5u] R:%04X W:%04X"
        , (unsigned int)HAL_GetTick()
        , _spi_state
        , (unsigned int)logmsg_id
        , (unsigned int)spi_packet_rate
        , (unsigned int)spi_speed_kbps
        //, trans_hdr->cmn.magic_ca , trans_hdr->cmn.magic_fe // PKT[%02X %02X]
        , trans_hdr->data
        , (unsigned int)log_cbmem_use()
        , log_info->readptr, log_info->writeptr
        );

    logmsg_id++;

    //int i;
    while(1)
    {
        // // STM32 SPI synchronization test (I/O test only) vvv
        // int stateOfPushButton = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
        // logout(WL_LOG_DEBUGNORMAL, "    [0x%08X][%s::%d] SYNC:%d", (unsigned int)HAL_GetTick(), __FILE__, __LINE__, stateOfPushButton);
        // HAL_GPIO_WritePin(SPI_SYNC_Port, SPI_SYNC_Pin, (stateOfPushButton ? GPIO_PIN_SET : GPIO_PIN_RESET));
        // // STM32 SPI synchronization test (I/O test only) ^^^

        //HAL_Delay(50);

        /* Send log message at a reasonable rate when blue button is released,
         * and like crazy when it is pushed.
         */
        unsigned long log_period = (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET ? 10 : 500);
        if((HAL_GetTick() - prev_tick) > log_period)
        {
            break;
        }


        /* If packet received from MAX 10,
         * then process its answer.
         */
        if(spi_periodic_check() == 1)
        {
            spi_init_cnt++;
        }

        /* Do miscellaneous tests. */
        do_tests();
    }

    prev_tick = HAL_GetTick();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }

  /* Terminate FatFs (never called). */
  termout(WL_LOG_DEBUGNORMAL, "FATFS_UnLinkDriver ...");
  FATFS_UnLinkDriver(SDPath);

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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SDIO|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLSAI.PLLSAIM = 16;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV4;
  PeriphClkInitStruct.PLLSAIDivQ = 1;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48CLKSOURCE_PLLSAIP;
  PeriphClkInitStruct.SdioClockSelection = RCC_SDIOCLKSOURCE_CLK48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief SDIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDIO_SD_Init(void)
{

  /* USER CODE BEGIN SDIO_Init 0 */

  /* USER CODE END SDIO_Init 0 */

  /* USER CODE BEGIN SDIO_Init 1 */

  /* USER CODE END SDIO_Init 1 */
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 0;
  /* USER CODE BEGIN SDIO_Init 2 */

  /* USER CODE END SDIO_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_SLAVE;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_HARD_INPUT;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI_SYNC_GPIO_Port, SPI_SYNC_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI_SYNC_Pin */
  GPIO_InitStruct.Pin = SPI_SYNC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(SPI_SYNC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_DETECT_Pin */
  GPIO_InitStruct.Pin = SD_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SD_DETECT_GPIO_Port, &GPIO_InitStruct);

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

    logout(WL_LOG_DEBUGNORMAL, "***-***-***-***-***-***");
    // logout(WL_LOG_DEBUGNORMAL, "Tick[0x%08X] Error_Handler at \"%s\"::%d"
    //     , (unsigned int)HAL_GetTick()
    //     , file
    //     , line);

    /* Make LD2 (green LED) blinking for a small while when error happens. */
    int i;
    for(i=0; i<3; i++)
    {
        HAL_GPIO_WritePin(GPIOA, LD2_Pin, GPIO_PIN_SET);
        HAL_Delay(200);
        HAL_GPIO_WritePin(GPIOA, LD2_Pin, GPIO_PIN_RESET);
        HAL_Delay(200);
    }

  //while(1)
  //{
  //}
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
