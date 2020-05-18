#ifndef _ADT_SHARED_H_
#define _ADT_SHARED_H_

#include <inttypes.h>

#define ADT_GEN_BUF_SIZE ((uint32_t)2000)

#define ADT_BYTES_IN_KIBIBYTE ((uint32_t)1024)
#define ADT_KIBIBYTES_IN_MEBIBYTE ((uint32_t)1024)
#define ADT_BYTES_IN_MEBIBYTE ((uint32_t)((ADT_KIBIBYTES_IN_MEBIBYTE) * (ADT_BYTES_IN_KIBIBYTE)))


#define ADT_DISK_RAW_INFO_IOCTL_SIZE ((uint16_t)256)
#define ADT_DISK_INFO_MODEL_LEN ((uint16_t)40)
#define ADT_DISK_INFO_MODEL_IOCTL_POS ((uint16_t)27)
#define ADT_DISK_INFO_SERIAL_LEN ((uint16_t)20)
#define ADT_DISK_INFO_SERIAL_IOCTL_POS ((uint16_t)10)
#define ADT_DISK_INFO_FIRMWARE_LEN ((uint16_t)8)
#define ADT_DISK_INFO_FIRMWARE_IOCTL_POS ((uint16_t)23)


void ADT_StripEnd(char* sParamString);

uint8_t bADT_IdentifyDisk(int iFd, char* sModel,
                          char* sSerial, char* sFirmware,
                          uint64_t* pu64SizeBytes);

#endif // #define _ADT_SHARED_H_
