/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include "ff.h"
#include "fatfs.h"
#include <string.h>
#include "usb_device.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_START_ADDRESS   	  0x8020000
#define APPUPDATE_FLAG_ADDRESS    0x80E0000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
uint8_t FileSysInit = false;
uint8_t FileRecvFinished = false;
uint8_t FirewareHasUpdated = false;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId myTask02Handle;
osThreadId myTask03Handle;
osMessageQId myQueue01Handle;
extern USBD_HandleTypeDef hUsbDeviceFS;
extern TIM_HandleTypeDef htim1;
/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartTask02(void const * argument);
void StartTask03(void const * argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of myQueue01 */
  osMessageQDef(myQueue01, 1, uint32_t);
  myQueue01Handle = osMessageCreate(osMessageQ(myQueue01), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of myTask02 */
  osThreadDef(myTask02, StartTask02, osPriorityIdle, 0, 1024);
  myTask02Handle = osThreadCreate(osThread(myTask02), NULL);

  /* definition and creation of myTask03 */
  osThreadDef(myTask03, StartTask03, osPriorityIdle, 0, 1024);
  myTask03Handle = osThreadCreate(osThread(myTask03), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */

void run_to_app(void)
{
	typedef void(*app_entry_t)(void);

	HAL_TIM_Base_Stop_IT(&htim1);
	HAL_NVIC_DisableIRQ(TIM1_UP_TIM10_IRQn);
	htim1.Instance->CNT = 0;

	USBD_Stop(&hUsbDeviceFS);

	__disable_irq();

	__set_MSP(*(__IO uint32_t*)APP_START_ADDRESS);
	__set_CONTROL(0);
	__set_PSP(0);

	app_entry_t app_entry = (app_entry_t)*(__IO uint32_t*)(APP_START_ADDRESS + 4);
	app_entry();

}
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
	  //no app firmware or have need to update flag, all use the flag FirmWareNeedUpdateFlag
	if(((*(uint32_t*)(APP_START_ADDRESS)) == 0xffffffff) || ((*(uint32_t*)(APP_START_ADDRESS)) == 0x0))
	{
		FirmWareNeedUpdateFlag = 0x55AA;
	}

	if(FirewareHasUpdated || 0 == FirmWareNeedUpdateFlag)
	{
		run_to_app();
	}
	else
	{
		HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
		osDelay(1000);
	}
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the myTask02 thread.
* @param argument: Not used
* @retval None
*/
const uint32_t crc32_table[256] =
{
	 0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
	 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
	 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	 0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
	 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
	 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	 0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
	 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	 0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
	 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
	 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
	 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
	 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
	 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
	 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
	 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	 0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
	 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	 0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
	 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	 0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
	 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
	 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
	 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	 0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
	 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
	 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
	 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	 0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
	 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t calculate_crc32(uint8_t *data, uint32_t len, uint32_t crc)
{
	uint32_t i;
	for (i=0; i < len; i ++)
	{
		crc = ( crc >> 8) ^crc32_table[(crc ^ data[i] ) & 0xff];
	}
	return crc ;
}
/* USER CODE END Header_StartTask02 */
void StartTask02(void const * argument)
{
  /* USER CODE BEGIN StartTask02 */

	FRESULT ret = FR_INT_ERR;
	uint32_t count = 0;
	static uint32_t totalcnt = 0;
	static uint8_t hearderRcv = false;
	FILINFO file_info;
	static uint32_t crc_value = 0xffffffff;
//	uint32_t filesize = 0;
//	uint32_t i;

  /* Infinite loop */
  for(;;)
  {
	if(0x55AA == FirmWareNeedUpdateFlag)
	{
		if(false == FileSysInit)
		{
			ret = f_mount (&SDFatFS, SDPath, 1);
			if(ret == FR_OK)
			{
				f_stat("ff.bin",&file_info);
				if(file_info.fsize>0)
				{
					f_unlink ("ff.bin");
				}
				ret = f_open (&SDFile, "ff.bin",FA_OPEN_ALWAYS|FA_WRITE|FA_READ);
				if(ret == FR_OK)
				{
					FileSysInit = true;
					crc_value = 0xffffffff;
					//USB_Printf("SD card init success!\r\n");
				}
			}
			osDelay(2000);
		}
		else
		{
			//USB_Printf("waiting data ... \r\n");
			osEvent UserDataEvent = osMessageGet (myQueue01Handle, 1000);
			if(UserDataEvent.status == osEventMessage)
			{
				if(hearderRcv)
				{
					f_lseek(&SDFile,f_size(&SDFile));
					ret = f_write(&SDFile, (uint8_t*)UserDataEvent.value.v, 64, &count);
					if((FR_OK == ret)&&(64 == count))
					{
						f_sync (&SDFile);
						totalcnt -=count;
					}
					if(0 == totalcnt)
					{
						//f_sync (&SDFile);
						//calc crc32
	//					f_rewind(&SDFile);
	//					uint8_t* databuf = pvPortMalloc(sizeof(uint8_t)*64);
	//					if(databuf != NULL)
	//					{
	//						filesize = f_size(&SDFile);
	//						for(i = 0; i < filesize/64;i++)
	//						{
	//							ret = f_read(&SDFile, databuf, 64, &count);
	//							crc_value = calculate_crc32(databuf, 64, crc_value);
	//							f_lseek (&SDFile, (i+1)*64);
	//							osDelay(100);
	//						}
	//					}
	//					vPortFree(databuf);
						f_close(&SDFile);

						USB_Printf("Receive Data Finished, the crc32 is %x.\r\n",crc_value);
						FileRecvFinished = true;
					}
				}
				else if(*((uint16_t*)UserDataEvent.value.v) == 0x55AA)
				{
					hearderRcv = true;
					totalcnt = *((uint32_t*)(((uint8_t*)UserDataEvent.value.v)+2));
					//USB_Printf("Receive header, the data length to receive is %d\r\n",totalcnt);
				}
			}
			//osDelay(100);
		}
	}
	else
	{
		osDelay(1000);
	}
  }
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_StartTask03 */
/**
* @brief Function implementing the myTask03 thread.
* @param argument: Not used
* @retval None
*/
#define FLASHACCESS_VALUE_EMPTY					0xFFFFFFFF						//!< Value for empty flash

#define	FLASHACCESS_SIZE_FLASH					((1024 - 128 - 128) * 1024)			//!< Total size of flash
#define FLASHACCESS_SECTOR_SIZE					(128 * 1024)					//!< Size of one sector [byte]
#define FLASHACCESS_SECTOR_MIN					5								//!< First sector to use
#define FLASHACCESS_SECTOR_MAX					10								//!< Last sector to use

#define FLASHACCESS_ADDRESS_MIN					0x08020000 						//!< First available address in flash
#define FLASHACCESS_ADDRESS_MAX					0x080FFFFF - 0x20000 			//!< Last available address in flash

#define FLASHACCESS_SECTOR_OF(address)			((((address) - 0x08020000) / FLASHACCESS_SECTOR_SIZE) + FLASHACCESS_SECTOR_MIN)				//!< Get the sector for an address
#define FLASHACCESS_IS_ADDRESS_VALID(address)	((address) >= FLASHACCESS_ADDRESS_MIN && (address) <= FLASHACCESS_ADDRESS_MAX - 3)			//!< Check if an address is valid
#define FLASHACCESS_ADDRESS_OF_SECTOR(sector)	(FLASHACCESS_ADDRESS_MIN + ((sector - FLASHACCESS_SECTOR_MIN) * FLASHACCESS_SECTOR_SIZE))	//!< Starting address of a given sector

bool FlashAccess_Clear (const uint8_t indexSector, const uint32_t length)
{
	uint32_t Result = 0;

	// determine number of sectors (at least one)
	uint32_t NumberOfSectors = (length + FLASHACCESS_SECTOR_SIZE - 1) / FLASHACCESS_SECTOR_SIZE;

	// check that the region is ok and in-between min and max sectors
	if (indexSector >= FLASHACCESS_SECTOR_MIN &&
			indexSector + NumberOfSectors - 1 <= FLASHACCESS_SECTOR_MAX)
	{
		// unlock the flash and clear the sectors
		HAL_FLASH_Unlock();

		FLASH_EraseInitTypeDef EraseInitStruct;
		EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
		EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
		EraseInitStruct.Sector = indexSector;
		EraseInitStruct.NbSectors = NumberOfSectors;

		// erase the flash sector
		Result = HAL_FLASHEx_Erase(&EraseInitStruct, &Result) == HAL_OK;
	}

	return (bool)Result;
}

bool FlashAccess_Write (const uint32_t address, const uint32_t value)
{
	bool Result = false;

	// make sure that it is a valid flash range
	if (FLASHACCESS_IS_ADDRESS_VALID(address))
	{
		// unlock the flash and write the value
		HAL_FLASH_Unlock();
		Result = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, value) == HAL_OK;
	}

	return Result;
}
void FlashClearFlag(void)
{
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t Result;
	HAL_FLASH_Unlock();

	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	EraseInitStruct.Sector = 11;
	EraseInitStruct.NbSectors = 1;

	// erase the flash sector
	HAL_FLASHEx_Erase(&EraseInitStruct, &Result);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, APPUPDATE_FLAG_ADDRESS, 0);

}
/* USER CODE END Header_StartTask03 */
void StartTask03(void const * argument)
{
  /* USER CODE BEGIN StartTask03 */
	FRESULT ret = FR_INT_ERR;
	uint32_t filesize;
	uint32_t address_end;
	uint8_t data[4];
	uint32_t count;
  /* Infinite loop */
  for(;;)
  {
	 if(FileRecvFinished)
	 {
		ret = f_open (&SDFile, "ff.bin", FA_READ);
		filesize = f_size(&SDFile);
		address_end = FLASHACCESS_ADDRESS_MIN + filesize;
		if(ret == FR_OK)
		{
			FlashAccess_Clear(FLASHACCESS_SECTOR_OF(FLASHACCESS_ADDRESS_MIN), FLASHACCESS_SIZE_FLASH);

			for (uint32_t Add = FLASHACCESS_ADDRESS_MIN; Add < address_end; )
			{
				ret = f_read(&SDFile, data, 4, &count);
				FlashAccess_Write(Add, *((uint32_t*)data));
				Add += sizeof(uint32_t);
				f_lseek(&SDFile,(Add-FLASHACCESS_ADDRESS_MIN));
			}
		}
		f_close(&SDFile);
		FileRecvFinished = false;
		FlashClearFlag();
		FirewareHasUpdated = true;
	 }
	 osDelay(1000);
   }

  /* USER CODE END StartTask03 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
