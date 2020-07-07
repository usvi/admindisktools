#include "adt_shared.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>



#define ADT_DC_VERSION_STR "Diskcont v. 1.02 by Janne Paalijarvi\n"
#define ADT_DC_RUNNING_NUM_SIZE_BYTES ((uint64_t)(8))
#define ADT_DC_PROGRESS_UPDATE_INTERVAL ((uint32_t)(5))
#define ADT_DC_DEFAULT_BUF_SIZE (((uint32_t)(100)) * ADT_BYTES_IN_MEBIBYTE)



typedef struct
{
  uint8_t u8Silent;
  uint8_t u8Write;
  uint8_t u8Read;
  uint32_t u32BufSize;
  char sDevice[ADT_GEN_BUF_SIZE];
  uint64_t u64DevSizeBytes;
  
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



static void DC_PrepareBuffer(void* pBufMem,
			     uint64_t u64AvailBufSizeBytes,
			     uint64_t u64DataLeftBytes,
			     uint64_t u64StartNumber,
			     uint64_t* pu64UsedDataBytes,
			     uint64_t* pu64UsedNumbers)
{
  // Staticed here just for efficiency
  static uint64_t u64BytesToWrite;
  static uint64_t u64NumNumbers;
  static uint64_t u64LeftoverBytes;
  static uint64_t u64WriteNum;
  static void* pMemUpperBound;
  
  memset(pBufMem, 0, u64AvailBufSizeBytes);
  // Pick smaller amount of full buffer and data left
  u64BytesToWrite = ((u64AvailBufSizeBytes < u64DataLeftBytes) ? u64AvailBufSizeBytes : u64DataLeftBytes);
  u64NumNumbers = u64BytesToWrite / ADT_DC_RUNNING_NUM_SIZE_BYTES;
  u64LeftoverBytes = u64BytesToWrite % ADT_DC_RUNNING_NUM_SIZE_BYTES;
  pMemUpperBound = pBufMem + (u64NumNumbers * ADT_DC_RUNNING_NUM_SIZE_BYTES);
  u64WriteNum = u64StartNumber;

  while (pBufMem < pMemUpperBound)
  {
    memcpy(pBufMem, &u64WriteNum, ADT_DC_RUNNING_NUM_SIZE_BYTES);
    u64WriteNum++;
    pBufMem += ADT_DC_RUNNING_NUM_SIZE_BYTES;
  }
  if (u64LeftoverBytes)
  {
    memcpy(pBufMem, &u64WriteNum, u64LeftoverBytes);
    u64WriteNum++;
  }
  *pu64UsedDataBytes = u64BytesToWrite;
  *pu64UsedNumbers = u64WriteNum - u64StartNumber;
}



static void DC_PrintProgress(uint64_t u64LastDataBytesLeft,
			     uint64_t u64NowDataBytesLeft,
			     uint64_t u64DevSizeBytes,
			     struct timeval* pxStartTime,
			     struct timeval* pxLastTime,
			     struct timeval* pxNowTime)
{
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
  
  u32TimeElapsed = pxNowTime->tv_sec - pxStartTime->tv_sec;
  u32Secs = u32TimeElapsed % 60;
  u32TimeElapsed -= u32Secs;
  u32Mins = (u32TimeElapsed % 3600) / 60;
  u32TimeElapsed -= u32Mins * 60;
  u32Hours = u32TimeElapsed / 3600;
  u64PassedBytes = u64DevSizeBytes - u64NowDataBytesLeft;
  fProgress = 100.0 * (1.0 * u64PassedBytes) / (1.0 * u64DevSizeBytes);

  // For calculations we need fine-grained stuff.
  fNowSpeedMbPerSeconds = 0.0;
  fAverageSpeedMbPerSeconds = 0.0;
  
  if (timercmp(pxLastTime, pxNowTime, <) && (u64LastDataBytesLeft))
  {
    // First calculate current speed
    fTimeElapsedFine = (1.0 * (pxNowTime->tv_sec - pxLastTime->tv_sec)) + (0.000001 * (pxNowTime->tv_usec - pxLastTime->tv_usec));
    fNowSpeedMbPerSeconds = (1.0 * (u64LastDataBytesLeft - u64NowDataBytesLeft)) / ((1.0 * ADT_BYTES_IN_MEBIBYTE) * fTimeElapsedFine);
    // And now we calculate average speed
    fTimeElapsedFine = (1.0 * (pxNowTime->tv_sec - pxStartTime->tv_sec)) + (0.000001 * (pxNowTime->tv_usec - pxStartTime->tv_usec));
    fAverageSpeedMbPerSeconds = (1.0 * (u64DevSizeBytes - u64NowDataBytesLeft)) / ((1.0 * ADT_BYTES_IN_MEBIBYTE) * fTimeElapsedFine);
  }

  printf("\x1b[A" "\x1b[A" "\r%" PRIu64 "/%" PRIu64 " bytes, %02.2f%% done. \n"
	 "%uh %02um %02us elapsed. \n"
	 "Speed now: %.2f MiB/s  Average: %.2f MiB/s       ",
	 u64PassedBytes, u64DevSizeBytes, fProgress,
	 u32Hours, u32Mins, u32Secs, fNowSpeedMbPerSeconds, fAverageSpeedMbPerSeconds);
  fflush(stdout);
}



static uint8_t bDC_ReadTest(tDcState* pxState)
{
  void* pCompBufMem = NULL;
  void* pReadBufMem = NULL;
  int iFd = -1;
  struct timeval xStartTime;
  struct timeval xLastTime;
  struct timeval xNowTime;
  uint64_t u64DataLeftBytes = pxState->u64DevSizeBytes;
  uint64_t u64LastDataLeftBytes = 0;
  uint64_t u64ReadNumber = 0;
  uint64_t u64BufferUsedDataBytes = 0;
  uint64_t u64BufferUsedNumbers = 0;
  uint64_t u64ReadCallBytes = 0;

  // No return value checking for performance purposes
  gettimeofday(&xStartTime, NULL);
  gettimeofday(&xLastTime, NULL);
  xLastTime.tv_sec -= (ADT_DC_PROGRESS_UPDATE_INTERVAL + 1); // Ensure at least one print
  
  
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
  // Write couple of newlines in sync to the prevline sequences
  printf("\n\n");

  // In loop prepare the buffer, read and compare
  while (u64DataLeftBytes)
  {
    DC_PrepareBuffer(pCompBufMem, pxState->u32BufSize,
		     u64DataLeftBytes,
		     u64ReadNumber,
		     &u64BufferUsedDataBytes,
		     &u64BufferUsedNumbers);
    
    u64ReadCallBytes = read(iFd, pReadBufMem, u64BufferUsedDataBytes);
  
    if (u64ReadCallBytes != u64BufferUsedDataBytes)
    {
      printf("Error: Problem reading bytes %" PRIu64 "\n", (pxState->u64DevSizeBytes - u64DataLeftBytes));
      free(pCompBufMem);
      free(pReadBufMem);
      close(iFd);
    
      return 0;
    }
    // Compare buffers
    if (memcmp(pCompBufMem, pReadBufMem, u64BufferUsedDataBytes) != 0)
    {
      // TODO: Find out which byte exactly.
      printf("\nError: Comparing failed at block beginning at %" PRIu64 "\n",
	     (pxState->u64DevSizeBytes - u64DataLeftBytes));
      free(pCompBufMem);
      free(pReadBufMem);
      close(iFd);
    
      return 0;
    }
    u64DataLeftBytes -= u64BufferUsedDataBytes;
    u64ReadNumber += u64BufferUsedNumbers;

    // Write where we are, if interval seconds passed
    gettimeofday(&xNowTime, NULL);

    if ((xLastTime.tv_sec + ADT_DC_PROGRESS_UPDATE_INTERVAL) < xNowTime.tv_sec)
    {
      DC_PrintProgress(u64LastDataLeftBytes, u64DataLeftBytes, pxState->u64DevSizeBytes,
		       &xStartTime, &xLastTime, &xNowTime);
      u64LastDataLeftBytes = u64DataLeftBytes;
      xLastTime = xNowTime;
    }
  }
  gettimeofday(&xNowTime, NULL);
  DC_PrintProgress(u64LastDataLeftBytes, u64DataLeftBytes, pxState->u64DevSizeBytes,
		   &xStartTime, &xLastTime, &xNowTime);
  printf("\nDone reading, compare OK!\n");
  
  free(pCompBufMem);
  free(pReadBufMem);
  close(iFd);

  return 1;
}



static uint8_t bDC_WriteTest(tDcState* pxState)
{
  void* pBufMem = NULL;
  int iFd = -1;
  struct timeval xStartTime;
  struct timeval xLastTime;
  struct timeval xNowTime;
  uint64_t u64DataLeftBytes = pxState->u64DevSizeBytes;
  uint64_t u64LastDataLeftBytes = 0;
  uint64_t u64WriteNumber = 0;
  uint64_t u64BufferUsedDataBytes = 0;
  uint64_t u64BufferUsedNumbers = 0;
  uint64_t u64WrittenCallBytes = 0;

  gettimeofday(&xStartTime, NULL);
  gettimeofday(&xLastTime, NULL);
  xLastTime.tv_sec -= (ADT_DC_PROGRESS_UPDATE_INTERVAL + 1); // Ensure at least one print
  
  pBufMem = malloc(pxState->u32BufSize);

  if (pBufMem == NULL)
  {
    printf("Error: Malloc failed\n");
    
    return 0;
  }
  // FIXME: Correct additional flags
  //iFd = open(pxState->sDevice, O_WRONLY | O_SYNC);
  iFd = open(pxState->sDevice, O_WRONLY);

  if (iFd == -1)
  {
    printf("Error: Unable to open the device in write mode\n");
    free(pBufMem);

    return 0;
  }
  // Write couple of newlines in sync to the prevline sequences
  printf("\n\n");

  // In loop prepare the buffer and write
  while (u64DataLeftBytes)
  {
    DC_PrepareBuffer(pBufMem, pxState->u32BufSize,
		     u64DataLeftBytes,
		     u64WriteNumber,
		     &u64BufferUsedDataBytes,
		     &u64BufferUsedNumbers);
    
    u64WrittenCallBytes = write(iFd, pBufMem, u64BufferUsedDataBytes);
  
    if (u64WrittenCallBytes != u64BufferUsedDataBytes)
    {
      printf("Error: Problem writing bytes %" PRIu64 "\n", (pxState->u64DevSizeBytes - u64DataLeftBytes));
      free(pBufMem);
      close(iFd);
    
      return 0;
    }
    u64DataLeftBytes -= u64BufferUsedDataBytes;
    u64WriteNumber += u64BufferUsedNumbers;

    // Write where we are, if interval seconds passed
    gettimeofday(&xNowTime, NULL);

    if ((xLastTime.tv_sec + ADT_DC_PROGRESS_UPDATE_INTERVAL) < xNowTime.tv_sec)
    {
      DC_PrintProgress(u64LastDataLeftBytes, u64DataLeftBytes, pxState->u64DevSizeBytes,
		       &xStartTime, &xLastTime, &xNowTime);
      u64LastDataLeftBytes = u64DataLeftBytes;
      xLastTime = xNowTime;
    }
  }
  gettimeofday(&xNowTime, NULL);
  DC_PrintProgress(u64LastDataLeftBytes, u64DataLeftBytes, pxState->u64DevSizeBytes,
		   &xStartTime, &xLastTime, &xNowTime);
  printf("\nSyncinc...\n");
  fsync(iFd);
  printf("Done all writing!\n");
  
  free(pBufMem);
  close(iFd);

  return 1;
}



int main(int argc, char* argv[])
{
  int iFd = -1;
  int iTemp = 0;
  tDcState xState;
  char sReadBuf[ADT_GEN_BUF_SIZE] = { 0 };
  char sSizeHumReadBuf[ADT_GEN_BUF_SIZE] = { 0 };
  char sModel[ADT_DISK_INFO_MODEL_LEN + 1] = { 0 };
  char sSerial[ADT_DISK_INFO_SERIAL_LEN + 1] = { 0 };
  
  printf(ADT_DC_VERSION_STR);
  
  if (!bDC_GetParams(argc, argv, &xState))
  {
    printf("Error: Params failure, use:\n");
    printf("diskcont [-w] [-r] [-s] /path/to/device\n");

    return 1;
  }
  iFd = open(xState.sDevice, O_RDONLY);

  if (iFd == -1)
  {
    printf("Error: Unable to open device %s (are you not root?)\n", xState.sDevice);
    
    return 1;
  }
  bADT_IdentifyDisk(iFd, sModel, sSerial, NULL, &(xState.u64DevSizeBytes));
  close(iFd);

  if (iTemp == -1)
  {
    printf("Error: Unable to get info for device (%s)!\n", xState.sDevice);
    
    return 1;
  }
  ADT_BytesToHumanReadable(xState.u64DevSizeBytes, sSizeHumReadBuf);
  printf("Found device %s   %s\n", xState.sDevice, sSizeHumReadBuf);
  printf("Model: %s   Serial: %s\n", sModel, sSerial);
  
  if (xState.u8Write)
  {
    // Write test
    if (!xState.u8Silent)
    {
      printf("This write test will COMPLETELY WIPE OUT %s\n", xState.sDevice);
      printf("To continue, type uppercase yes\n");
      fgets(sReadBuf, sizeof(sReadBuf), stdin);

      if (strncmp(sReadBuf, "YES", strlen("YES")) != 0)
      {
	printf("Error: User failed to confirm operation\n");
	
	return 1;
      }
    }
    if (!bDC_WriteTest(&xState))
    {
      return 1;
    }
  }
  if (xState.u8Read)
  {
    if (!bDC_ReadTest(&xState))
    {
      return 1;
    }
  }

  return 0;
}
