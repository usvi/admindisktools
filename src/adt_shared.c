#include "adt_shared.h"

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/hdreg.h>
#include <linux/fs.h>


void ADT_TrimBegin(char* sParamString)
{
  uint32_t i;
  uint32_t u32Len = strlen(sParamString);

  if (u32Len <= 1)
  {
    return;
  }

  for (i = 0; i < u32Len; i++)
  {
    if (sParamString[0] == ' ')
    {
      // Memmove left 1 byte and axe the last byte
      memmove(sParamString, (sParamString + 1), (u32Len - 1 - i));
      sParamString[u32Len - 1 - i] = 0;
    }
    else
    {
      break;
    }
  }
}

void ADT_TrimEnd(char* sParamString)
{
  uint32_t i;
  uint32_t u32Len = strlen(sParamString);

  if (u32Len <= 1)
  {
    return;
  }

  for (i = (u32Len - 1); i > 0; i--)
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



void ADT_Trim(char* sParamString)
{
  ADT_TrimBegin(sParamString);
  ADT_TrimEnd(sParamString);
}






uint8_t bADT_IdentifyDisk(int iFd, char* sModel,
			  char* sSerial, char* sFirmware,
			  uint64_t* pu64SizeBytes)
{
  uint8_t u8RetVal = 1;
  uint64_t u64Temp = 0;
  uint16_t au16DriveInfoRaw[ADT_DISK_RAW_INFO_IOCTL_SIZE] = { 0 };

  if ((sModel != NULL) || (sSerial != NULL) || (sFirmware != NULL))
  {
    // First, zer non-null buffers
    if (sModel != NULL)
    {
      memset(sModel, 0, ADT_DISK_INFO_MODEL_LEN + 1);
    }
    if (sSerial != NULL)
    {
      memset(sSerial, 0, ADT_DISK_INFO_SERIAL_LEN + 1);
    }
    if (sFirmware != NULL)
    {
      memset(sFirmware, 0, ADT_DISK_INFO_FIRMWARE_LEN + 1);
    }
    
    // Get the raw info then
    
    if (!ioctl(iFd, HDIO_GET_IDENTITY, au16DriveInfoRaw))
    {
      // Got it, now put it to only non-null buffers
      if (sModel != NULL)
      {
	strncpy(sModel, (char*)(&(au16DriveInfoRaw[ADT_DISK_INFO_MODEL_IOCTL_POS])),
		ADT_DISK_INFO_MODEL_LEN);
	ADT_Trim(sModel);
      }
      if (sSerial != NULL)
      {
	strncpy(sSerial, (char*)(&(au16DriveInfoRaw[ADT_DISK_INFO_SERIAL_IOCTL_POS])),
		ADT_DISK_INFO_SERIAL_LEN);
	ADT_Trim(sSerial);
      }
      if (sFirmware != NULL)
      {
	strncpy(sFirmware, (char*)(&(au16DriveInfoRaw[ADT_DISK_INFO_FIRMWARE_IOCTL_POS])),
		ADT_DISK_INFO_FIRMWARE_LEN);
	ADT_Trim(sFirmware);
      }
      
      u8RetVal = ((u8RetVal > 0) ? 1 : u8RetVal);
    }
    else
    {
      u8RetVal = 0;
    }
  }
  // ID done, then get size if needed
  if (pu64SizeBytes != NULL)
  {
    *pu64SizeBytes = 0;

    if (!ioctl(iFd, BLKGETSIZE64, &u64Temp))
    {
      *pu64SizeBytes = u64Temp;

      u8RetVal = ((u8RetVal > 0) ? 1 : u8RetVal);
    }
    else
    {
      u8RetVal = 0;
    }
  }

  return u8RetVal;
}

// Uses 1024-bases correctly
void ADT_BytesToHumanReadable(uint64_t u64SizeBytes,
                              char* sHumanReadable)
{
  float fSize = 0.0;
  
  memset(sHumanReadable, 0, ADT_GEN_BUF_SIZE);

  // Classify and set
  if (u64SizeBytes < ADT_BYTES_IN_KIBIBYTE)
  {
    // Bytes
    snprintf(sHumanReadable, ADT_GEN_BUF_SIZE, "%" PRIu64 " B", u64SizeBytes);
  }
  else if (u64SizeBytes < ADT_BYTES_IN_MEBIBYTE)
  {
    // Still kibibytes
    fSize = (1.0 * u64SizeBytes) / (1.0 * ADT_BYTES_IN_KIBIBYTE);
    snprintf(sHumanReadable, ADT_GEN_BUF_SIZE, "%.1f KiB", fSize);
  }
  else if (u64SizeBytes < ADT_BYTES_IN_GIBIBYTE)
  {
    // Still mebibytes
    fSize = (1.0 * u64SizeBytes) / (1.0 * ADT_BYTES_IN_MEBIBYTE);
    snprintf(sHumanReadable, ADT_GEN_BUF_SIZE, "%.1f MiB", fSize);
  }
  else if (u64SizeBytes < ADT_BYTES_IN_TEBIBYTE)
  {
    // Still gibibytes
    fSize = (1.0 * u64SizeBytes) / (1.0 * ADT_BYTES_IN_GIBIBYTE);
    snprintf(sHumanReadable, ADT_GEN_BUF_SIZE, "%.1f GiB", fSize);
  }
  else
  {
    // Still tebibytes
    fSize = (1.0 * u64SizeBytes) / (1.0 * ADT_BYTES_IN_TEBIBYTE);
    snprintf(sHumanReadable, ADT_GEN_BUF_SIZE, "%.1f TiB", fSize);
  }

  
}
