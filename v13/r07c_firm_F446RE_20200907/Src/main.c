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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <string.h>
#include "spi_max10.h"
#include "log.h"

#include "sdcard/sdcard.h"

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
SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* File entries, used when listing SD card root folder contents. */
#define FILES_MAXCOUNT 100
scdf_file_entry_t _file_entries[FILES_MAXCOUNT];
unsigned char _sdcard_read_test[32*1024];


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI2_Init(void);
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
  /* USER CODE BEGIN 2 */

  /* Output a message on Hyperterminal using printf function */
  termout(WL_LOG_DEBUGNORMAL, "UART Printf Example: retarget the C library printf function to the UART");
  termout(WL_LOG_DEBUGNORMAL, "Build Date : %s, time : %s, for Nucleo F446RE.", __DATE__, __TIME__);


#if 0
while(1) // TEST : see how good or bad SPI synchronization status is read from MAX10
{
//#define SPI_MAX10_RCV_HDR_READY() HAL_GPIO_WritePin(GPIOA, SPI_SYNC_Pin, GPIO_PIN_RESET)
//#define SPI_MAX10_SND_RSP_READY() HAL_GPIO_WritePin(GPIOA, SPI_SYNC_Pin, GPIO_PIN_SET  )
    int stateOfPushButton = HAL_GPIO_ReadPin(GPIOC, B1_Pin);
    if(stateOfPushButton == GPIO_PIN_RESET)
    {
        HAL_GPIO_WritePin(GPIOA, SPI_SYNC_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOA, SPI_SYNC_Pin, GPIO_PIN_SET  );
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    }
}
#endif

  logout(WL_LOG_DEBUGNORMAL, "---");
  logout(WL_LOG_DEBUGNORMAL, "UART Printf Example: retarget the C library printf function to the UART");
  logout(WL_LOG_DEBUGNORMAL, "Build Date : %s, time : %s, for Nucleo F446RE.", __DATE__, __TIME__);

  /* Initialize SPI internals. */
  spi_init();

  /* Initialize SD card related stuff. */
  scf_init();

  /* ----------------------------------------------------- */
  /* ----------------------------------------------------- */
  /* ----------------------------------------------------- */
  termout(WL_LOG_DEBUGNORMAL, "------------------------------------------");
  termout(WL_LOG_DEBUGNORMAL, "------------------------------------------");
  termout(WL_LOG_DEBUGNORMAL, "------------------------------------------");

    /* SD card test related. */
    // char* filename;
    // unsigned long tmp;
    sdcard_t* sd = sc_get_sdcard_buff();
    sdc_ret_t ret;
    unsigned long items_count;
unsigned char char2pchar(unsigned char c);


    /* Re-init SD card library.
     * Note : if error is detected here, then SD card won't be
     *        tested after that. This is in order to support
     *        test on Action Replay carts without flooding
     *        screen with errors.
     */
    sdc_fat_reset();
    sdc_fat_init();
    ret = sd->init_status;

    if(ret == SDC_OK)
    {
        termout(WL_LOG_DEBUGNORMAL, "SD card init success");
    }
    else
    {
        termout(WL_LOG_DEBUGNORMAL, "SD card init failure ! ret = %d", ret);

        HAL_GPIO_WritePin(SD_CS_GPIO_Port , SD_CS_Pin , GPIO_PIN_SET);
        HAL_GPIO_WritePin(SD_DIN_GPIO_Port, SD_DIN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(SD_CLK_GPIO_Port, SD_CLK_Pin, GPIO_PIN_SET);
        int val = HAL_GPIO_ReadPin(SD_DOUT_GPIO_Port, SD_DOUT_Pin);

        while(1)
        {
            int set = ((HAL_GetTick() % 4000) > 2000 ? 1 : 0);
            //HAL_GPIO_WritePin(SD_CS_GPIO_Port , SD_CS_Pin , (set ? GPIO_PIN_SET : GPIO_PIN_RESET));
            //HAL_GPIO_WritePin(SD_DIN_GPIO_Port, SD_DIN_Pin, (set ? GPIO_PIN_SET : GPIO_PIN_RESET));
            HAL_GPIO_WritePin(SD_CLK_GPIO_Port, SD_CLK_Pin, (set ? GPIO_PIN_SET : GPIO_PIN_RESET));

            int val2 = HAL_GPIO_ReadPin(SD_DOUT_GPIO_Port, SD_DOUT_Pin);
            //termout(WL_LOG_DEBUGNORMAL, "DOUT : %d", val2);
            if(val2 != val)
            {
                termout(WL_LOG_DEBUGNORMAL, "DOUT activity ! %d", val2);
                val = val2;
            }
        }
    }

    termout(WL_LOG_DEBUGNORMAL, "CID register details:");
    termout(WL_LOG_DEBUGNORMAL, " | Mfg ID         MID=%d"                 , sd->stats.cid[ 0]);
    termout(WL_LOG_DEBUGNORMAL, " | OEM/App ID     OID=%c%c"               , char2pchar(sd->stats.cid[ 1]), char2pchar(sd->stats.cid[ 2]));
    termout(WL_LOG_DEBUGNORMAL, " | Product name   PNM=%c%c%c%c%c"         , char2pchar(sd->stats.cid[ 3]), char2pchar(sd->stats.cid[ 4]), char2pchar(sd->stats.cid[ 5]), char2pchar(sd->stats.cid[ 6]), char2pchar(sd->stats.cid[ 7])); 
    termout(WL_LOG_DEBUGNORMAL, " | Product rev    PRV=0x%02X (v%d.%d)"    , sd->stats.cid[ 8], sd->stats.cid[ 8]>>4, sd->stats.cid[ 8] & 0x0F);
    termout(WL_LOG_DEBUGNORMAL, " | Product serial PSN=0x%02X%02X%02X%02X" , sd->stats.cid[ 9], sd->stats.cid[10], sd->stats.cid[11], sd->stats.cid[12]);
    termout(WL_LOG_DEBUGNORMAL, " | Mfg date       MDT=0x%04X %4d/%2d"     , ((sd->stats.cid[13]&0x0F) << 8) | sd->stats.cid[14], 2000+(((sd->stats.cid[13]&0x0F) << 4) | sd->stats.cid[14]>>4), sd->stats.cid[14]&0x0F);
    termout(WL_LOG_DEBUGNORMAL, " | checksum*2+1   CRC=0x%02X"             , sd->stats.cid[15]);
// #define CHAR_PER_LINE 8
//     for(j=0; j<(sizeof(sd->stats.cid) / CHAR_PER_LINE); j++)
//     {
//         /* Display start address. */
//         conio_printf(2, row, COLOR_YELLOW, "%03X:", j*CHAR_PER_LINE);
// 
//         for(i=0; i<CHAR_PER_LINE; i++)
//         {
//             unsigned char c = sd->stats.cid[j*CHAR_PER_LINE + i];
//             conio_printf(2+4+i*3  , row, COLOR_WHITE, "%02X", c);
//             conio_printf(2+4+8*3+i, row, COLOR_WHITE, "%c"  , char2pchar(c));
//         }
//         row++;
//     }
// #undef CHAR_PER_LINE
    termout(WL_LOG_DEBUGNORMAL, "");

    termout(WL_LOG_DEBUGNORMAL, "CSD register details:");
    termout(WL_LOG_DEBUGNORMAL, " | sdhc        = %d (CCS=%d)", sd->stats.sdhc, sd->ccs);
    termout(WL_LOG_DEBUGNORMAL, " | file_format = %d", sd->stats.file_format);
    termout(WL_LOG_DEBUGNORMAL, " | user_size   = %d MB", sd->stats.user_size);
// #define CHAR_PER_LINE 8
//     for(j=0; j<(sizeof(sd->stats.csd) / CHAR_PER_LINE); j++)
//     {
//         /* Display start address. */
//         conio_printf(2, row, COLOR_YELLOW, "%03X:", j*CHAR_PER_LINE);
// 
//         for(i=0; i<CHAR_PER_LINE; i++)
//         {
//             unsigned char c = sd->stats.csd[j*CHAR_PER_LINE + i];
//             conio_printf(2+4+i*3  , row, COLOR_WHITE, "%02X", c);
//             conio_printf(2+4+8*3+i, row, COLOR_WHITE, "%c"  , char2pchar(c));
//         }
//         row++;
//     }
// #undef CHAR_PER_LINE
    termout(WL_LOG_DEBUGNORMAL, "");

    termout(WL_LOG_DEBUGNORMAL, "Sector #0 contents:");

    /* Display sector#0 contents. */
    unsigned char sector_data[512];
    memset(sector_data, 0xFE, sizeof(sector_data));
    sdc_read_multiple_blocks(0/*sector*/, sector_data, 1/*blocks_count*/);

    int offset_list[20] =
    {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 
        55, 56, 57, 58, 59, 60, 61,             /* First partition data.    */
        63                                      /* Signature (55 AA).       */
    };
#define CHAR_PER_LINE 8
    for(int j=0; j<sizeof(offset_list) / sizeof(int); j++)
    {
        char tmp[CHAR_PER_LINE+1] = { '\0' };

        for(int i=0; i<CHAR_PER_LINE; i++)
        {
            tmp[i] = char2pchar(sector_data[offset_list[j]*CHAR_PER_LINE+i]);
        }

        termout(WL_LOG_DEBUGNORMAL, "%03X: %s", offset_list[j]*CHAR_PER_LINE, tmp);
    }
#undef CHAR_PER_LINE



    /* Try to list files in root folder. */
    sdc_fat_reset();
    ret = sdc_fat_init();

    termout(WL_LOG_DEBUGNORMAL, "Listing files ...");
    items_count = scf_filelist(_file_entries, FILES_MAXCOUNT, 0/*list_offset*/, 1/*list_sort*/, NULL/*all files*/, SC_FILE_SDCARD, "/");
    termout(WL_LOG_DEBUGNORMAL, "-> %d files in root folder", items_count);

    for(int i=0; i<items_count; i++)
    {
        unsigned short sort_index = _file_entries[i].sort_index;

        char test_path[SC_PATHNAME_MAXLEN];
        sprintf(test_path, "/%s", _file_entries[sort_index].filename);
        unsigned long test_size = 0;
        test_size = scf_size(SC_FILE_SDCARD, test_path);

        termout(WL_LOG_DEBUGNORMAL, "ITM[%2d][%2d] size:%8u, name:\"%s\"", i, sort_index, test_size, _file_entries[sort_index].filename);
    }

#if 0
    /* Retrieve size of a specific file. */
    unsigned long test_size = 0;
    test_size = scf_size(SC_FILE_SDCARD, "/0.bin");
    termout(WL_LOG_DEBUGNORMAL, "0.bin file size : %u bytes", test_size);
#endif

    /* Read a specific size. */
    unsigned long sdcard_read_stt = HAL_GetTick();
    scf_read(SC_FILE_SDCARD, "/sonicr.iso", 0, sizeof(_sdcard_read_test), _sdcard_read_test);
    unsigned long sdcard_read_end = HAL_GetTick();
    unsigned long sdcard_read_diff = sdcard_read_end - sdcard_read_stt;
    unsigned long sdcard_read_speed = ((sizeof(_sdcard_read_test)/1024) * 1000) / sdcard_read_diff;
    termout(WL_LOG_DEBUGNORMAL, "%u KB read in %u msec. speed = %u KB/s", sizeof(_sdcard_read_test) >> 10, sdcard_read_diff, sdcard_read_speed);

#define CHAR_PER_LINE 16
    for(int j=0; j<8; j++)
    {
        char tmp[CHAR_PER_LINE+1] = { '\0' };

        for(int i=0; i<CHAR_PER_LINE; i++)
        {
            tmp[i] = char2pchar(_sdcard_read_test[(j*CHAR_PER_LINE) + i]);
        }

        termout(WL_LOG_DEBUGNORMAL, "%03X: %s", j*CHAR_PER_LINE, tmp);
    }
#undef CHAR_PER_LINE



  /* ----------------------------------------------------- */
  /* ----------------------------------------------------- */
  /* ----------------------------------------------------- */




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
    wl_spi_trans_hdr_t* trans_hdr = &_spi_trans_hdr;

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

    trans_hdr->params[30] = '\0';
    logout(WL_LOG_DEBUGNORMAL, "[%08X:%d] ID[%3u] PKT/s[%4u][%4u KB/s] END[%02X%02X%02X%02X] [%s] [cbu:%5u] R:%04X W:%04X"
        , (unsigned int)HAL_GetTick()
        , _spi_state
        , (unsigned int)logmsg_id
        , (unsigned int)spi_packet_rate
        , (unsigned int)spi_speed_kbps
        //, trans_hdr->cmn.magic_ca , trans_hdr->cmn.magic_fe // PKT[%02X %02X]
        , trans_hdr->end_bytes[0], trans_hdr->end_bytes[1], trans_hdr->end_bytes[2], trans_hdr->end_bytes[3]
        , trans_hdr->params
        , (unsigned int)log_cbmem_use()
        , log_info->readptr, log_info->writeptr
        );

    logmsg_id++;

    //int i;
    while(1)
    {
        // // STM32 SPI synchronization test (I/O test only) vvv
        // int stateOfPushButton = HAL_GPIO_ReadPin(GPIOC, B1_Pin);
        // logout(WL_LOG_DEBUGNORMAL, "    [0x%08X][%s::%d] SYNC:%d", (unsigned int)HAL_GetTick(), __FILE__, __LINE__, stateOfPushButton);
        // HAL_GPIO_WritePin(GPIOA, SPI_SYNC_Pin, (stateOfPushButton ? GPIO_PIN_SET : GPIO_PIN_RESET));
        // // STM32 SPI synchronization test (I/O test only) ^^^

        //HAL_Delay(50);

        /* Send log message at a reasonable rate when blue button is released, 
         * and like crazy when it is pushed.
         */
        unsigned long log_period = (HAL_GPIO_ReadPin(GPIOC, B1_Pin) == GPIO_PIN_RESET ? 10 : 500);
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
    }

    prev_tick = HAL_GetTick();

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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, SD_CS_Pin|SD_CLK_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_DIN_GPIO_Port, SD_DIN_Pin, GPIO_PIN_RESET);

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

  /*Configure GPIO pin : SD_DOUT_Pin */
  GPIO_InitStruct.Pin = SD_DOUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SD_DOUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SD_CS_Pin SD_CLK_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin|SD_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_DIN_Pin */
  GPIO_InitStruct.Pin = SD_DIN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SD_DIN_GPIO_Port, &GPIO_InitStruct);

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
