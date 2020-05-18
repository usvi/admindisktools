#include "adt_shared.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>



#define ADT_DI_VERSION_STR "Diskinfo v. 1.00 by Janne Paalijarvi\n"



typedef struct
{
  char sDevice[ADT_GEN_BUF_SIZE];
  uint64_t u64DevSizeBytes;
  
} tDcState;



static uint8_t bDC_GetParams(int argc, char* argv[], tDcState* pxState)
{
  // Default settings
  memset(pxState->sDevice, 0, ADT_GEN_BUF_SIZE);

  if (argc != 2)
  {
    // Device not given and argc generally too small
    return 0;
  }
  // Basically every other thing: Device given.
  strcpy(pxState->sDevice, argv[argc - 1]);

  return 1;
}



int main(int argc, char* argv[])
{
  int iFd = -1;
  int iTemp = 0;
  tDcState xState;
  char sSizeHumReadBuf[ADT_GEN_BUF_SIZE] = { 0 };
  char sModel[ADT_DISK_INFO_MODEL_LEN + 1] = { 0 };
  char sSerial[ADT_DISK_INFO_SERIAL_LEN + 1] = { 0 };
  char sFirmware[ADT_DISK_INFO_FIRMWARE_LEN + 1] = { 0 };
  
  printf(ADT_DI_VERSION_STR);
  
  if (!bDC_GetParams(argc, argv, &xState))
  {
    printf("Error: Params failure, device path needed\n");

    return 1;
  }
  iFd = open(xState.sDevice, O_RDONLY);

  if (iFd == -1)
  {
    printf("Error: Unable to open device %s (are you not root?)\n", xState.sDevice);
    
    return 1;
  }
  bADT_IdentifyDisk(iFd, sModel, sSerial, sFirmware, &(xState.u64DevSizeBytes));
  close(iFd);

  if (iTemp == -1)
  {
    printf("Error: Unable to get info for device (%s)!\n", xState.sDevice);
    
    return 1;
  }
  ADT_BytesToHumanReadable(xState.u64DevSizeBytes, sSizeHumReadBuf);
  printf("Device: %s\n", xState.sDevice);
  printf("Size: %s\n", sSizeHumReadBuf);
  printf("Bytesize: %" PRIu64 "\n", xState.u64DevSizeBytes);
  printf("Model: %s\n", sModel);
  printf("Serial: %s\n", sSerial);
  printf("Firmware: %s\n", sFirmware);

  return 0;
}
