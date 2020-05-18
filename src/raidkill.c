#include "adt_shared.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include <errno.h>

#define ADT_RK_VERSION_STR "Raidkill v. 1.02 by Janne Paalijarvi\n"
// Different vendors have different metadata handling, so we need
// to just guess something for the kill buffer size.
#define ADT_RK_KILL_BUF_SIZE ((uint32_t)((ADT_BYTES_IN_MEBIBYTE) / 2))



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
  pxState->u32BufSize = ADT_RK_KILL_BUF_SIZE;

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









static uint8_t bRK_ReadRaid(tDcState* pxState)
{
  // And how to check we have succeeded?
  // Read back buffer amount.
  void* pCompBufMem = NULL;
  void* pReadBufMem = NULL;
  int iFd = -1;
  int iCompBeginResult = 0;
  int iCompEndResult = 0;
  
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
  
  iFd = open(pxState->sDevice, O_RDONLY);

  if (iFd == -1)
  {
    printf("Error: Unable to open the device in write mode\n");
    free(pCompBufMem);
    free(pReadBufMem);

    return 0;
  }
  // Make two reads and compare later

  // Beginning
  if ((lseek(iFd, 0, SEEK_SET) == -1) ||
      (read(iFd, pReadBufMem, pxState->u32BufSize) != pxState->u32BufSize))
  {
    printf("Error: Unable to read from the beginning\n");
    free(pCompBufMem);
    free(pReadBufMem);
    close(iFd);

    return 1;
  }
  iCompBeginResult = memcmp(pCompBufMem, pReadBufMem, pxState->u32BufSize);

  // And end
  if ((lseek(iFd, (pxState->u64DevSizeBytes - pxState->u32BufSize), SEEK_SET) == -1) ||
      (read(iFd, pReadBufMem, pxState->u32BufSize) != pxState->u32BufSize))
  {
    printf("Error: Unable to read from the end\n");
    free(pCompBufMem);
    free(pReadBufMem);
    close(iFd);

    return 1;
  }
  iCompEndResult = memcmp(pCompBufMem, pReadBufMem, pxState->u32BufSize);

  // We can already close our stuff
  free(pCompBufMem);
  free(pReadBufMem);
  close(iFd);

  if (iCompBeginResult != 0)
  {
    printf("Compare failed for beginning\n");
  }
  if (iCompEndResult != 0)
  {
    printf("Compare failed for end\n");
  }

  if ((iCompBeginResult != 0) || (iCompEndResult != 0))
  {
    printf("Raid might still be active!\n");

    return 0;
  }
  // else
  printf("Raid successfully verified killed\n");

  return 1;
}




static uint8_t bRK_KillRaid(tDcState* pxState)
{
  // Various vendors have different specifications for RAID, some
  // write some amount of metadata to the end and some write it
  // to the beginning. We need to erase both areas with a kill
  // buffer. The size defined at the beginning of file.
  void* pBufMem = NULL;
  int iFd = -1;

  pBufMem = malloc(pxState->u32BufSize);

  if (pBufMem == NULL)
  {
    printf("Error: Malloc failed\n");
    
    return 0;
  }
  iFd = open(pxState->sDevice, O_WRONLY);

  if (iFd == -1)
  {
    printf("Error: Unable to open the device in write mode\n");
    free(pBufMem);

    return 0;
  }

  // Seek, write and flush the beginning
  if ((lseek(iFd, 0, SEEK_SET) == -1) ||
      (write(iFd, pBufMem, pxState->u32BufSize) != pxState->u32BufSize) ||
      (fsync(iFd) == -1))
  {
    printf("Error: Unable to write to the beginning\n");
    free(pBufMem);
    close(iFd);

    return 0;
  }
  printf("Wrote %u bytes to the beginning\n", pxState->u32BufSize);

  // Seek, write and flush the end
  if ((lseek(iFd, (pxState->u64DevSizeBytes - pxState->u32BufSize), SEEK_SET) == -1) ||
      (write(iFd, pBufMem, pxState->u32BufSize) != pxState->u32BufSize) ||
      (fsync(iFd) == -1))
  {
    printf("Error: Unable to write to the end\n");
    free(pBufMem);
    close(iFd);

    return 0;
  }
  // We can already release these
  free(pBufMem);
  close(iFd);

  printf("Wrote %u bytes to the end\n", pxState->u32BufSize);
  printf("Successfully wrote raid kill buffers\n");

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
  
  printf(ADT_RK_VERSION_STR);
  
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
      printf("This command will COMPLETELY WIPE OUT RAID\n");
      printf("remnants on %s\n", xState.sDevice);
      printf("To continue, type uppercase yes\n");
      fgets(sReadBuf, sizeof(sReadBuf), stdin);

      if (strncmp(sReadBuf, "YES", strlen("YES")) != 0)
      {
	printf("Error: User failed to confirm operation\n");
	
	return 1;
      }
    }
    if (!bRK_KillRaid(&xState))
    {
      return 1;
    }
  }
  if (xState.u8Read)
  {
    if (!bRK_ReadRaid(&xState))
    {
      return 1;
    }
  }

  return 0;
}
