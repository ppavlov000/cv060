#include "main.h"
//-----------------------------------------------------------------------------------
#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
//-----------------------------------------------------------------------------------
#define None 0	// no error
#define XMODEM_Err_Attempt 2
#define XMODEM_Err_BigImage 3
#define Ok 1
//-----------------------------------------------------------------------------------
XMODEM Xmodem;
//-----------------------------------------------------------------------------------
void Xmodem_Start()
{
 Xmodem.Block		= 1;
 Xmodem.Attempt	= 0;
 sendchar(NAK);
 Xmodem_SetState(XMOD_St_WaitBegin,XMOD_SSt_WaitSohOrEot);
}
//-----------------------------------------------------------------------------------
void Xmodem_SetState(u8 st,u8 sst)
{
 Xmodem.State		= st;
 Xmodem.SubState = sst;
}
//-----------------------------------------------------------------------------------
void Xmodem_End()
{
	 sendchar(CAN);	// There is error!
	 sendchar(CAN);
	 Delay(1000);
}
//-----------------------------------------------------------------------------------
u8 Xmodem_Processor( void (* write_data)(u8* data,u8 len))
{
 s16 c;
 u8 status;
 status = XMOD_None;
 switch (Xmodem.State)
 {
	 //-------------------------------------------
	 case XMOD_St_NextAttempt	 :
		 Xmodem.Attempt++;
		 if (Xmodem.Attempt > XMOD_AttemptsCount)
		 {
			 Xmodem_End();
			 status = XMOD_Error;
			 break;
		 }
		 sendchar(NAK);
		 Time0=XMOD_AttemptsTimer;
		 Xmodem_SetState(XMOD_St_WaitBegin,XMOD_SSt_WaitSohOrEot);
	 break;
	//-------------------------------------------
	 case XMOD_St_WaitBegin	 :
		 if (Time0==0)
		 {
			 Xmodem_SetState(XMOD_St_NextAttempt,0);
			 break;
		 }
		 c = recchar();
		 if (c < 0) break;
		 switch (Xmodem.SubState)
		 {
	 		//-------------------- 	
			 case XMOD_SSt_WaitSohOrEot :
					 if(c==SOH)	// Start of new packet
					 {
					 		Xmodem_SetState(XMOD_St_WaitBegin,XMOD_SSt_WaitSeq);
					 		break;
					 }
					 if(c==EOT)	// End of session
					 {
					 		sendchar(ACK);
					 		status=XMOD_Ok;
					 		break;
					 }
			 break;
			 //--------------------
			 case XMOD_SSt_WaitSeq:
				 if (c == (Xmodem.Block&0xff))
					 Xmodem_SetState(XMOD_St_WaitBegin,XMOD_SSt_WaitCmplSeq);		// Ok right number of blok
				 else
					 Xmodem_SetState(XMOD_St_WaitBegin,XMOD_SSt_WaitSohOrEot);	// oncorrect number of block
			 break;
			 //--------------------
			 case XMOD_SSt_WaitCmplSeq	:
				 if (c == ((Xmodem.Block&0xff)^0xff))
				 {
					 Xmodem.Received = 0;
					 Xmodem.Lrc			= 0;
					 Xmodem_SetState(XMOD_St_ReceivePacket,0);
					 Time0=XMOD_PacketTimer;
					 break;
				 }
				 Xmodem_SetState(XMOD_St_WaitBegin,XMOD_SSt_WaitSohOrEot);// incorrect ^number of block
			 break;
		 }
	 break;
	 //-------------------------------------------
	 case XMOD_St_ReceivePacket :
		 if (Time0==0)
		 {
			 Xmodem_SetState(XMOD_St_NextAttempt,0);
			 break;
		 }
		 c = recchar();
		 if (c < 0) break;

		 if (Xmodem.Received >= XMOD_DataSize) // If entire packet is loaded
		 {
			 if (c != Xmodem.Lrc)
			 {
				 Xmodem_SetState(XMOD_St_NextAttempt,0);
				 break;
			 }
			 Xmodem_SetState(XMOD_St_SavePacket,0);
			 break;
		 }
		 Xmodem.Packet[Xmodem.Received] = (u8)c;
		 Xmodem.Lrc+= c;
		 Xmodem.Received++;
	 break;
	 //-------------------------------------------
	 case XMOD_St_SavePacket	 :
//	 	 if(Flash_Write(&Xmodem.Packet[0],XMOD_DataSize)!=0)	// 0 - no error
//			 status = XMOD_Error;
//		 AT45_Write(&Xmodem.Packet[0],XMOD_DataSize);

//		fram_mem_write(Fram_Addr,&Xmodem.Packet[0],XMOD_DataSize);
//		Fram_Addr+=XMOD_DataSize;
		write_data(&Xmodem.Packet[0],Xmodem.Received);
		 if (status!=XMOD_Error)
		 {
			 Xmodem.Block++;
			 Xmodem.Attempt=0;
			 sendchar(ACK);						
			 Xmodem_SetState(XMOD_St_WaitBegin,XMOD_SSt_WaitSohOrEot);
			 //Xmodem.Received = 0;			
		 }
		 else
			 Xmodem_End();
	 break;
 }
 return status;
}
//-----------------------------------------------------------------------------------

