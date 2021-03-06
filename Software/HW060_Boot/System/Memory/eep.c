//-----------------------------------------------------------------------------------
#include "..\main.h"
//-----------------------------------------------------------------------------------
void EEPut(u8 *Data,u16 Addr,u8 Len)
{
	u8 i;
	for(i=0;i<Len;i++)
	{
		__watchdog_reset();
		//*((u8 __eeprom *)(Addr+i))=*Data;
		__EEPUT(Addr+i,*Data);
		__watchdog_reset();
		Data++;
	}
}
//-----------------------------------------------------------------------------------
void EEGet(u8 *Data,u16 Addr,u8 Len)
{
	u8 i;
	for(i=0;i<Len;i++)
	{
		__watchdog_reset();
		//*Data= *((u8 __eeprom *)(Addr+i));
		__EEGET(*Data,Addr+i);
		__watchdog_reset();
		Data++;
	}
}
//-----------------------------------------------------------------------------------
