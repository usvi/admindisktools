#include "adt_shared.h"

#include <string.h>
#include <sys/ioctl.h>
#include <linux/hdreg.h>
#include <linux/fs.h>


void ADT_StripEnd(char* sParamString)
{
  uint32_t i;
  uint64_t u64Len = strlen(sParamString);

  if (u64Len <= 1)
  {
    return;
  }

  for (i = (u64Len - 1); i > 0; i--)
  {
    if (sParamString[i] == ' ')
    {
      sParamString[i] = 0;
    }
    else
    {
      break;
    }
  }
}



uint8_t bADT_IdentifyDisk(int iFd, char* sModel,
			  char* sSerial, char* sFirmware,
			  uint64_t* pu64SizeBytes)
{
  uint8_t u8RetVal = 0;
  uint64_t u64Temp = 0;
  uint16_t au16DriveInfoRaw[ADT_DISK_RAW_INFO_IOCTL_SIZE] = { 0 };

  
  memset(sModel, 0, ADT_DISK_INFO_MODEL_LEN + 1);
  memset(sSerial, 0, ADT_DISK_INFO_SERIAL_LEN + 1);
  memset(sFirmware, 0, ADT_DISK_INFO_FIRMWARE_LEN + 1);
  *pu64SizeBytes = 0;

  if (!ioctl(iFd, HDIO_GET_IDENTITY, au16DriveInfoRaw))
  {
    strncpy(sModel, (char*)(&(au16DriveInfoRaw[ADT_DISK_INFO_MODEL_IOCTL_POS])),
            ADT_DISK_INFO_MODEL_LEN);
    strncpy(sSerial, (char*)(&(au16DriveInfoRaw[ADT_DISK_INFO_SERIAL_IOCTL_POS])),
            ADT_DISK_INFO_SERIAL_LEN);
    strncpy(sFirmware, (char*)(&(au16DriveInfoRaw[ADT_DISK_INFO_FIRMWARE_IOCTL_POS])),
            ADT_DISK_INFO_FIRMWARE_LEN);
    // Strip stuff from end
    ADT_StripEnd(sModel);
    ADT_StripEnd(sSerial);
    ADT_StripEnd(sFirmware);

    if (!ioctl(iFd, BLKGETSIZE64, &u64Temp))
    {
      *pu64SizeBytes = u64Temp;
      u8RetVal = 1;
    }
    else
    {
      u8RetVal = 0;
    }
  }

  return u8RetVal;
}
