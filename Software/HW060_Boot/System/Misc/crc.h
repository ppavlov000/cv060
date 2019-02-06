#ifndef __CRC
#define __CRC
//------------------------------------------------------------------------------
#include "types.h"
//------------------------------------------------------------------------------
#define CRC16_INIT      0xffff
#define CRC16_POLY      0x8408
#define CRC16_FINAL     0xffff
//------------------------------------------------------------------------------
void  InitCRC(u16 *crc);
void  EndCRC(u16 *crc);
void  CalcCRC(u8 *p,u16 size, u16 *crc);
//------------------------------------------------------------------------------
#endif
