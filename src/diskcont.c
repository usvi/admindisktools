#include "adt_shared.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>


#define ADT_DC_VERSION_STR "Diskcont v. 1.02 by Janne Paalijarvi\n"
#define ADT_DC_RUNNING_NUM_SIZE_BYTES ((uint64_t)(8))
#define ADT_DC_PROGRESS_UPDATE_INTERVAL ((uint32_t)(5))
#define ADT_DC_DEFAULT_BUF_SIZE (((uint32_t)(100)) * ADT_BYTES_IN_MEBIBYTE)



typedef struct
{
  uint8_t u8Silent;
  uint8_t u8Write;
  uint8_t u8Read;
  uint8_t u8ThreadError;
  uint32_t u32BufSize;
  char sDevice[ADT_GEN_BUF_SIZE];
  uint64_t u64DevSizeBytes;
  pthread_t xAllocatorThread;
  sem_t xSemThread;
  sem_t xSemBuffer0;
  sem_t xSemBuffer1;
  void* apMemBufs[2];
  uint8_t u8WantBuffer;
  uint64_t u64CurrNumber;
  
} tDcState;



static uint8_t bDC_GetParams(int argc, char* argv[], tDcState* pxState)
{
  uint8_t i;
  uint8_t u8WriteFound = 0;
  uint8_t u8ReadFound = 0;

  // Default settings
  pxState->u8Silent = 0;
  pxState->u8Write = 1;
  pxState->u8Read = 1;
  pxState->u32BufSize = ADT_DC_DEFAULT_BUF_SIZE;

  memset(pxState->sDevice, 0, ADT_GEN_BUF_SIZE);

  if (argc < 2)
  {
    // Device not given and argc generally too small
    return 0;
  }

  for (i = 1; i < (argc - 1); i++)
  {
    if (strcmp("-r", argv[i]) == 0)
    {
      u8ReadFound = 1;
    }
    else if (strcmp("-w", argv[i]) == 0)
    {
      u8WriteFound = 1;
    }
    else if (strcmp("-s", argv[i]) == 0)
    {
      pxState->u8Silent = 1;
    }
    else
    {
      // Wrong parameter
      
      return 0;
    }

  }
  if (u8WriteFound + u8ReadFound)
  {
    // Default setting invalid
    pxState->u8Write = 0;
    pxState->u8Read = 0;
    // New setting
    pxState->u8Write = u8WriteFound;
    pxState->u8Read = u8ReadFound;
  }

  if ((strcmp(argv[argc - 1], "-r") == 0) ||
      (strcmp(argv[argc - 1], "-w") == 0) ||
      (strcmp(argv[argc - 1], "-s") == 0) ||
      (strncmp(argv[argc - 1], "-", 1) == 0))
  {
    // No device given
    return 0;
  }
  // Device given.
  strcpy(pxState->sDevice, argv[argc - 1]);

  return 1;
}



static void DC_PrepareBuffer(tDcState* pxState, void* pBufMem)
{
  // Staticed here just for efficiency
  static uint64_t u64BytesToWrite;
  static uint64_t u64NumNumbers;
  static uint64_t u64WriteNum;
  static void* pMemUpperBound;
  
  memset(pBufMem, 0, pxState->u32BufSize);
  // Pick smaller amount of full buffer and data left
  u64BytesToWrite = pxState->u32BufSize;
  u64NumNumbers = u64BytesToWrite / ADT_DC_RUNNING_NUM_SIZE_BYTES;
  pMemUpperBound = pBufMem + (u64NumNumbers * ADT_DC_RUNNING_NUM_SIZE_BYTES);
  u64WriteNum = pxState->u64CurrNumber;

  while (pBufMem < pMemUpperBound)
  {
    memcpy(pBufMem, &u64WriteNum, ADT_DC_RUNNING_NUM_SIZE_BYTES);
    u64WriteNum++;
    pBufMem += ADT_DC_RUNNING_NUM_SIZE_BYTES;
  }
  pxState->u64CurrNumber += u64NumNumbers;
}



static void DC_PrintProgress(tDcState* pxState)
{
  /*
  // Staticed here just for efficiency
  static uint32_t u32TimeElapsed;
  static uint32_t u32Secs;
  static uint32_t u32Mins;
  static uint32_t u32Hours;
  static uint64_t u64PassedBytes;
  static float fProgress;
  static float fTimeElapsedFine;
  static float fNowSpeedMbPerSeconds;
  static float fAverageSpeedMbPerSeconds;
  
  u32TimeElapsed = pxState->xNowTime.tv_sec - pxState->xStartTime.tv_sec;
  u32Secs = u32TimeElapsed % 60;
  u32TimeElapsed -= u32Secs;
  u32Mins = (u32TimeElapsed % 3600) / 60;
  u32TimeElapsed -= u32Mins * 60;
  u32Hours = u32TimeElapsed / 3600;
  u64PassedBytes = pxState->u64DevSizeBytes - pxState->u64DataLeftBytes;
  fProgress = 100.0 * (1.0 * u64PassedBytes) / (1.0 * pxState->u64DevSizeBytes);

  // For calculations we need fine-grained stuff.
  fNowSpeedMbPerSeconds = 0.0;
  fAverageSpeedMbPerSeconds = 0.0;
  
  if (timercmp(&(pxState->xLastTime), &(pxState->xNowTime), <) && (pxState->u64LastDataLeftBytes))
  {
    // First calculate current speed
    fTimeElapsedFine = (1.0 * (pxState->xNowTime.tv_sec - pxState->xLastTime.tv_sec)) +
      (0.000001 * (pxState->xNowTime.tv_usec - pxState->xLastTime.tv_usec));
    fNowSpeedMbPerSeconds = (1.0 * (pxState->u64LastDataLeftBytes - pxState->u64DataLeftBytes)) /
      ((1.0 * ADT_BYTES_IN_MEBIBYTE) * fTimeElapsedFine);
    // And now we calculate average speed
    fTimeElapsedFine = (1.0 * (pxState->xNowTime.tv_sec - pxState->xStartTime.tv_sec)) +
      (0.000001 * (pxState->xNowTime.tv_usec - pxState->xStartTime.tv_usec));
    fAverageSpeedMbPerSeconds = (1.0 * (pxState->u64DevSizeBytes - pxState->u64DataLeftBytes)) /
      ((1.0 * ADT_BYTES_IN_MEBIBYTE) * fTimeElapsedFine);
  }

  printf("\x1b[A" "\x1b[A" "\r%" PRIu64 "/%" PRIu64 " bytes, %02.2f%% done. \n"
	 "%uh %02um %02us elapsed. \n"
	 "Speed now: %.2f MiB/s  Average: %.2f MiB/s       ",
	 u64PassedBytes, pxState->u64DevSizeBytes, fProgress,
	 u32Hours, u32Mins, u32Secs, fNowSpeedMbPerSeconds, fAverageSpeedMbPerSeconds);
  fflush(stdout);
  */
}



static uint8_t bDC_ReadTest(tDcState* pxState)
{
  /*
  void* pCompBufMem = NULL;
  void* pReadBufMem = NULL;
  int iFd = -1;
  uint64_t u64ReadCallBytes = 0;

  pxState->u64DataLeftBytes = pxState->u64DevSizeBytes;
  pxState->u64LastDataLeftBytes = 0;
  pxState->u64CurrNumber = 0;
  pxState->u64BufferUsedDataBytes = 0;
  pxState->u64BufferUsedNumbers = 0;

  // No return value checking for performance purposes
  gettimeofday(&(pxState->xStartTime), NULL);
  gettimeofday(&(pxState->xLastTime), NULL);
  pxState->xLastTime.tv_sec -= (ADT_DC_PROGRESS_UPDATE_INTERVAL + 1); // Ensure at least one print
  
  
  pCompBufMem = malloc(pxState->u32BufSize);

  if (pCompBufMem == NULL)
  {
    printf("Error: Malloc failed\n");
    
    return 0;
  }
  pReadBufMem = malloc(pxState->u32BufSize);

  if (pReadBufMem == NULL)
  {
    free(pCompBufMem);

    printf("Error: Malloc failed\n");

    return 0;
  }
  
  //iFd = open(pxState->sDevice, O_RDONLY | O_SYNC);
  iFd = open(pxState->sDevice, O_RDONLY);

  if (iFd == -1)
  {
    printf("Error: Unable to open the device in write mode\n");
    free(pCompBufMem);
    free(pReadBufMem);

    return 0;
  }
  printf("Read test starting\n");
  // Write couple of newlines in sync to the prevline sequences
  printf("\n\n");

  // In loop prepare the buffer, read and compare
  while (pxState->u64DataLeftBytes)
  {
    DC_PrepareBuffer(pxState, pCompBufMem);
    u64ReadCallBytes = read(iFd, pReadBufMem, pxState->u64BufferUsedDataBytes);
  
    if (u64ReadCallBytes != pxState->u64BufferUsedDataBytes)
    {
      printf("Error: Problem reading bytes %" PRIu64 "\n", (pxState->u64DevSizeBytes - pxState->u64DataLeftBytes));
      free(pCompBufMem);
      free(pReadBufMem);
      close(iFd);
    
      return 0;
    }
    // Compare buffers
    if (memcmp(pCompBufMem, pReadBufMem, pxState->u64BufferUsedDataBytes) != 0)
    {
      // TODO: Find out which byte exactly.
      printf("\nError: Comparing failed at block beginning at %" PRIu64 "\n",
	     (pxState->u64DevSizeBytes - pxState->u64DataLeftBytes));
      free(pCompBufMem);
      free(pReadBufMem);
      close(iFd);
    
      return 0;
    }
    pxState->u64DataLeftBytes -= pxState->u64BufferUsedDataBytes;
    pxState->u64CurrNumber += pxState->u64BufferUsedNumbers;

    // Write where we are, if interval seconds passed
    gettimeofday(&(pxState->xNowTime), NULL);

    if ((pxState->xLastTime.tv_sec + ADT_DC_PROGRESS_UPDATE_INTERVAL) < pxState->xNowTime.tv_sec)
    {
      DC_PrintProgress(pxState);
      pxState->u64LastDataLeftBytes = pxState->u64DataLeftBytes;
      pxState->xLastTime = pxState->xNowTime;
    }
  }
  gettimeofday(&(pxState->xNowTime), NULL);
  DC_PrintProgress(pxState);
  printf("\nDone reading, compare OK!\n");
  
  free(pCompBufMem);
  free(pReadBufMem);
  close(iFd);
  */
  return 1;
}



static uint8_t bDC_BufferAllocator(void* pParams)
{
  uint8_t u8RetVal = 0;
  tDcState* pxState = (tDcState*)pParams;
  
  while (1)
  {
    sem_wait(&(pxState->xSemThread));

    if (pxState->u8WantBuffer == 0)
    {
      DC_PrepareBuffer(pxState, pxState->apMemBufs[0]);
      sem_post(&(pxState->xSemBuffer0));
    }
    else if (pxState->u8WantBuffer == 1)
    {
      DC_PrepareBuffer(pxState, pxState->apMemBufs[1]);
      sem_post(&(pxState->xSemBuffer1));
    }
    else
    {
      u8RetVal = 0;
      pthread_exit(&u8RetVal);
    }
  }

  return 1;
}



static uint8_t bDC_WriteTest(tDcState* pxState)
{
  uint64_t u64WrittenCallBytes = 0;
  uint64_t u64FullBuffersToWrite = 0;
  uint64_t u64LeftoverBytesToWrite = 0;
  uint64_t u64WriteBufferNum = 0;
  int iFd = -1;

  pthread_create(&(pxState->xAllocatorThread), NULL,
		 (void*)bDC_BufferAllocator, pxState);

  // Need to allocate both buffers
  pxState->apMemBufs[0] = malloc(pxState->u32BufSize);
  pxState->apMemBufs[1] = malloc(pxState->u32BufSize);

  if ((pxState->apMemBufs[0] == NULL) ||
      (pxState->apMemBufs[1] == NULL))
  {
    printf("Error: Malloc failed\n");
    free(pxState->apMemBufs[0]);
    free(pxState->apMemBufs[1]);
    
    return 0;
  }
  // Now it is time to set counters
  pxState->u64CurrNumber = 0;

  // Thread is now waiting instructions
  pxState->u8WantBuffer = 0;
  // We wake another thread and sleep ourselves
  sem_post(&(pxState->xSemThread));
  sem_wait(&(pxState->xSemBuffer0));
  // Ok, allocator thread has allocated buf 0.
  // In order for the writer loop
  // to work as expected, buffer sem needs additional post.
  sem_post(&(pxState->xSemBuffer0));

  // Figure out how many full buffers and leftover bytes to write
  u64FullBuffersToWrite = pxState->u64DevSizeBytes / pxState->u32BufSize;
  u64LeftoverBytesToWrite = pxState->u64DevSizeBytes - (u64FullBuffersToWrite * pxState->u32BufSize);

  printf("Writing %" PRIu64 " buffers and %" PRIu64 " leftover bytes\n",
	 u64FullBuffersToWrite, u64LeftoverBytesToWrite);

  // FIXME: Correct additional flags
  //iFd = open(pxState->sDevice, O_WRONLY | O_SYNC);
  iFd = open(pxState->sDevice, O_WRONLY);

  if (iFd == -1)
  {
    printf("Error: Unable to open the device in write mode\n");

    return 0;
  }
  printf("Write test starting\n");
  // Write couple of newlines in sync to the prevline sequences
  printf("\n\n");

  // In loop prepare the buffer and write
  for (u64WriteBufferNum = 0; u64WriteBufferNum < u64FullBuffersToWrite; u64WriteBufferNum++)
  {
    if (pxState->u8WantBuffer == 0)
    {
      pxState->u8WantBuffer = 1;
      // Let thread allocate at the same time we write:
      sem_post(&(pxState->xSemThread));
      sem_wait(&(pxState->xSemBuffer0));
      u64WrittenCallBytes = write(iFd, pxState->apMemBufs[0], pxState->u32BufSize);
    }
    else // pxState->u8WantBuffer == 1
    {
      pxState->u8WantBuffer = 0;
      // Let thread allocate at the same time we write:
      sem_post(&(pxState->xSemThread));
      sem_wait(&(pxState->xSemBuffer1));
      u64WrittenCallBytes = write(iFd, pxState->apMemBufs[1], pxState->u32BufSize);
    }
    if (u64WrittenCallBytes != pxState->u32BufSize)
    {
      printf("Error: Problem writing bytes %" PRIu64 "\n",
	     pxState->u32BufSize * u64WriteBufferNum);

      pxState->u8WantBuffer = 100;
      sem_post(&(pxState->xSemThread));
      pthread_join(pxState->xAllocatorThread, NULL);
      free(pxState->apMemBufs[0]);
      free(pxState->apMemBufs[1]);
      close(iFd);

      return 0;
    }
  }
  if (u64LeftoverBytesToWrite)
  {
    if (pxState->u8WantBuffer == 0)
    {
      sem_wait(&(pxState->xSemBuffer0));
      u64WrittenCallBytes = write(iFd, pxState->apMemBufs[0],
				  u64LeftoverBytesToWrite);
    }
    else // pxState->u8WantBuffer == 1
    {
      sem_wait(&(pxState->xSemBuffer1));
      u64WrittenCallBytes = write(iFd, pxState->apMemBufs[1],
				  u64LeftoverBytesToWrite);
    }
    if (u64WrittenCallBytes != u64LeftoverBytesToWrite)
    {
      printf("Error: Problem writing bytes %" PRIu64 "\n",
	     pxState->u32BufSize * u64WriteBufferNum);

      pxState->u8WantBuffer = 100;
      sem_post(&(pxState->xSemThread));
      pthread_join(pxState->xAllocatorThread, NULL);
      free(pxState->apMemBufs[0]);
      free(pxState->apMemBufs[1]);
      close(iFd);

      return 0;
    }
  }



  
  printf("\nSyncinc...\n");
  fsync(iFd);
  printf("Done all writing!\n");
  close(iFd);

  // Bogus value so thread exists:
  pxState->u8WantBuffer = 100;
  sem_post(&(pxState->xSemThread));
  pthread_join(pxState->xAllocatorThread, NULL);
  free(pxState->apMemBufs[0]);
  free(pxState->apMemBufs[1]);
  printf("Finish\n");
  
  return 1;
}



int main(int argc, char* argv[])
{
  int iFd = -1;
  int iTemp = 0;
  tDcState* pxState;
  char sReadBuf[ADT_GEN_BUF_SIZE] = { 0 };
  char sSizeHumReadBuf[ADT_GEN_BUF_SIZE] = { 0 };
  char sModel[ADT_DISK_INFO_MODEL_LEN + 1] = { 0 };
  char sSerial[ADT_DISK_INFO_SERIAL_LEN + 1] = { 0 };

  printf(ADT_DC_VERSION_STR);

  pxState = malloc(sizeof(*pxState));

  if (pxState == NULL)
  {
    printf("Failed to malloc state struct\n");

    return 1;
  }
  if ((sem_init(&(pxState->xSemThread), 0, 0) != 0) ||
      (sem_init(&(pxState->xSemBuffer0), 0, 0) != 0) ||
      (sem_init(&(pxState->xSemBuffer1), 0, 0) != 0))
  {
    printf("Failed to initialize semaphores\n");
    free(pxState);

    return 1;
  }
   
  if (!bDC_GetParams(argc, argv, pxState))
  {
    printf("Error: Params failure, use:\n");
    printf("diskcont [-w] [-r] [-s] /path/to/device\n");
    free(pxState);

    return 1;
  }
  iFd = open(pxState->sDevice, O_RDONLY);

  if (iFd == -1)
  {
    printf("Error: Unable to open device %s (are you not root?)\n", pxState->sDevice);
    free(pxState);
    
    return 1;
  }
  bADT_IdentifyDisk(iFd, sModel, sSerial, NULL, &(pxState->u64DevSizeBytes));
  close(iFd);

  if (iTemp == -1)
  {
    printf("Error: Unable to get info for device (%s)!\n", pxState->sDevice);
    free(pxState);
    
    return 1;
  }
  ADT_BytesToHumanReadable(pxState->u64DevSizeBytes, sSizeHumReadBuf);
  printf("Found device %s   %s\n", pxState->sDevice, sSizeHumReadBuf);
  printf("Model: %s   Serial: %s\n", sModel, sSerial);
  
  if (pxState->u8Write)
  {
    // Write test
    if (!pxState->u8Silent)
    {
      printf("This write test will COMPLETELY WIPE OUT %s\n", pxState->sDevice);
      printf("To continue, type uppercase yes\n");
      fgets(sReadBuf, sizeof(sReadBuf), stdin);

      if (strncmp(sReadBuf, "YES", strlen("YES")) != 0)
      {
	printf("Error: User failed to confirm operation\n");
	free(pxState);
	
	return 1;
      }
    }
    if (!bDC_WriteTest(pxState))
    {
      free(pxState);

      return 1;
    }
  }
  if (pxState->u8Read)
  {
    if (!bDC_ReadTest(pxState))
    {
      free(pxState);

      return 1;
    }
  }
  free(pxState);

  return 0;
}
