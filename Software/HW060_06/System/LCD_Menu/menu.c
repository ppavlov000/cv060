//-----------------------------------------------------------------------------------
#include "..\main.h"
#include "menuitems_lcd_1.h"
//-----------------------------------------------------------------------------------
vu8 Key0_Time=KEYS_TIME;
vu8 Key1_Time=KEYS_TIME;
vu8 Key2_Time=KEYS_TIME;
vu8 Key3_Time=KEYS_TIME;
vu16 Key_Repeat_Time=0;
vu8 key0=2;
vu8 key1=2;
vu8 key2=2;
vu8 key3=2;
u16 pos=0;
u8 pos2=0;
u16 edit_pos=0;
u8 lcd_txt[32];
u8 menu_stat=0;
u8 current_pid=0;
u8 pass_match=0;
u8 password_lvl=0;
//-----------------------------------------------------------------------------------
in_flash u8	eng_menu_str[3][17]=
{
	"                ",	//0
	"Options",			//1
	"Breakdowns",			//2
};
in_flash u8	ru_menu_str[3][17]=
{
	"                ",	//0
	"���������",		//1
	"������",			//2	
};
//-----------------------------------------------------------------------------------
in_flash u8	eng_title_str[4][16]=
{
	"EastLabs",				//0
	"Controller V.031",		//1
	"HW",					//2
	"SW",  					//3
};
in_flash u8	ru_title_str[4][16]=
{
	"EastLabs",				//0
	"���������� V.031",		//1
	"HW",					//3
	"SW",  					//4
};
//-----------------------------------------------------------------------------------
#define TOP_RECORDS_NUM 4
extern in_flash u8	empty_str[17];
in_flash TMenuRecodr	eng_top_str[TOP_RECORDS_NUM]=
{
	"",				0,		//0
	"",				0,		//4	
	"Brakedowns",	0,		//5
	"Options",		2,		//6	
};
in_flash TMenuRecodr	ru_top_str[TOP_RECORDS_NUM]=
{
	"",				0,		//0
	"",				0,		//4	
	"������",		0,		//5
	"���������",	2,		//6		
};
//-----------------------------------------------------------------------------------
TMenuRegisters MenuRegs;
//-----------------------------------------------------------------------------------
TLCDMenuItem _LCD_Root=
{
	0,
	lcd_menu_show_start,			
	&_LCD_Root,	
	&_LCD_Password,
	NULL,	
	NULL,
};
//-----------------------------------------------------------------------------------
TLCDMenuItem*	LCDMenuPtr=&_LCD_Root;
TLCDMenuItem*	Previos_LCDMenuPtr=&_LCD_Root;
u8 newmenuitem=1;
//TMenuItem*	MenuPtr2;
//-----------------------------------------------------------------------------------
//TMenuItem*	MenuList[Menu_List_Count];
//u8 MenuList_Count;
//-----------------------------------------------------------------------------------
void menu_init()
{
	menu_hardware_init();
//	MenuRegs.mode_of_controller=0;	
//	MenuRegs.mode_of_operation=1;	// Stop Mode
//	MenuRegs.lang=1;				// English
	MenuRegs.lang=1;
}
//-----------------------------------------------------------------------------------
void keys_handler()
{
	// Key 0
	if(menu_is_button_0_pressed())
	{
		Key0_Time=KEYS_TIME;		
		key0=0;	// reset key
	}
	else
	{
//		__sprintf(outtxt,"\n\r Key 0");

		if(key0==0)
		{
			if(Key0_Time==0)		
			{
				key0=1;
				BuzzerTime=MAX_BUZZER_TIME;
				Key_Repeat_Time=KEYS_REPEAT_TIME0;
			}
		}
		if(key0==2)
		{
			if(Key_Repeat_Time==0)		
			{
				key0=1;
				BuzzerTime=MAX_BUZZER_TIME;
				Key_Repeat_Time=KEYS_REPEAT_TIME1;
			}
		}			
	}
	// Key 1
	if(menu_is_button_1_pressed())
	{
		Key1_Time=KEYS_TIME;		
		key1=0;	// reset key
	}
	else
	{
		if(key1==0)
		{
			if(Key1_Time==0)		
			{
				key1=1;
				BuzzerTime=MAX_BUZZER_TIME;
				Key_Repeat_Time=KEYS_REPEAT_TIME0;
			}
		}
		if(key1==2)
		{
			if(Key_Repeat_Time==0)		
			{
				key1=1;
				BuzzerTime=MAX_BUZZER_TIME;
				Key_Repeat_Time=KEYS_REPEAT_TIME1;
			}
		}					
	}
	// Key 2
	if(menu_is_button_2_pressed())
	{
		Key2_Time=KEYS_TIME;		
		key2=0;	// reset key
	}
	else
	{
		if(key2==0)
		{
			if(Key2_Time==0)		
			{
				key2=1;
				BuzzerTime=MAX_BUZZER_TIME;
				Key_Repeat_Time=KEYS_REPEAT_TIME0;
			}
		}		
		if(key2==2)
		{
			if(Key_Repeat_Time==0)		
			{
				key2=1;
				BuzzerTime=MAX_BUZZER_TIME;
				Key_Repeat_Time=KEYS_REPEAT_TIME1;
			}
		}								
	}
	// Key 3
	if(menu_is_button_3_pressed())
	{
		Key3_Time=KEYS_TIME;		
		key3=0;	// reset key
	}
	else
	{
		if(key3==0)
		{
			if(Key3_Time==0)		
			{
				key3=1;
				BuzzerTime=MAX_BUZZER_TIME;
				Key_Repeat_Time=KEYS_REPEAT_TIME0;
			}
		}
		if(key3==2)
		{
			if(Key_Repeat_Time==0)		
			{
				key3=1;
				BuzzerTime=MAX_BUZZER_TIME;
				Key_Repeat_Time=KEYS_REPEAT_TIME1;
			}
		}					
	}
}
//-----------------------------------------------------------------------------------
void keys_menu()
{
	// Up arrow
	if(key0==1)
	{
		if(LCDMenuPtr->Flag==0)
		{
			if(LCDMenuPtr->PreviosItem!=NULL)
			{
				newmenuitem=1;
				LCDMenuPtr=(TLCDMenuItem*)LCDMenuPtr->PreviosItem;				
			}
			key0=2;
		}
	}
	// Down arrow	
	if(key1==1)
	{
		if(LCDMenuPtr->Flag==0)		
		{
			if(LCDMenuPtr->NextItem!=NULL)
			{
				newmenuitem=1;
				LCDMenuPtr=(TLCDMenuItem*)LCDMenuPtr->NextItem;				
			}
			key1=2;	
		}
	}
	// Escape		
	if(key2==1)
	{
		if(LCDMenuPtr->Flag==0)		
		{
			if(LCDMenuPtr->UpItem!=NULL)
			{
				newmenuitem=1;
				LCDMenuPtr=(TLCDMenuItem*)LCDMenuPtr->UpItem;						
			}
			key2=2;				
		}
	}
	// Enter	
	if(key3==1)
	{
		if(LCDMenuPtr->Flag==0)		
		{
			if(LCDMenuPtr->SubItem!=NULL)
			{
				newmenuitem=1;
				LCDMenuPtr=(TLCDMenuItem*)LCDMenuPtr->SubItem;		
			}
			key3=2;				
		}
	}
	LCDMenuPtr->Routine();	
}
//-----------------------------------------------------------------------------------
void lcd_menu_show_start()
{

//	s16 tmp;
//	u8 edit_pos_tmp;	
//	u8 show_pos_tmp;		
	if(newmenuitem)
	{
		if(menu_stat==0)
		{
			lcd_clear();			
			lcd_printf_P(eng_title_str[0]);
			lcd_set_pos(1,0);
			if(MenuRegs.lang==0)
				lcd_printf_P(eng_title_str[1]);			
			if(MenuRegs.lang==1)
				lcd_printf_P(ru_title_str[1]);						
			LCD_Time=2000;					
		}
		if(menu_stat==1)
		{
			lcd_clear();			
			lcd_printf_P(eng_title_str[2]);
			FGet(HAPP_FLASH_START+THW_LOC,(u8*)&ltmp,4);	
			LongToStr(lcd_txt,&ltmp,0);
			lcd_set_pos(0,3);
			lcd_printf(lcd_txt);
			
			lcd_set_pos(1,0);
			if(MenuRegs.lang==0)
				lcd_printf_P(eng_title_str[3]);			
			if(MenuRegs.lang==1)
				lcd_printf_P(ru_title_str[3]);		
			FGet(HAPP_FLASH_START+TSW_LOC,(u8*)&ltmp,4);	
			LongToStr(lcd_txt,&ltmp,0);
			lcd_set_pos(1,3);
			lcd_printf(lcd_txt);								
			LCD_Time=2000;					
		}					
	}
	newmenuitem=0;
	if(LCD_Time==0)
	{
		newmenuitem=1;	
		switch(menu_stat)
		{
			case 0:menu_stat=1;break;
			case 1:
				if(LCDMenuPtr->SubItem!=NULL)
				{
					current_pid=pos2;			
					pos=0;
					edit_pos=0;
					menu_stat=0;
					LCDMenuPtr=(TLCDMenuItem*)LCDMenuPtr->SubItem;						
					newmenuitem=1;		
				}				
			break;			
		}
		
	}
	
}
//-----------------------------------------------------------------------------------
void lcd_menu_show_info_full()
{
	s16 tmp;
//	u8 show_pos_tmp;
	u8 i,j,pos_old;
	u8 cline;
	u8 edit_pos_tmp;	
	u8 show_pos=0;
	if(newmenuitem)
	{
		if(MenuRegs.lang==0)
			menu_str=&eng_top_str[0];
		if(MenuRegs.lang==1)
			menu_str=&ru_top_str[0];
			
		for(i=pos;i<(TOP_RECORDS_NUM-1);i++)
			if(menu_str[i].lvl<=password_lvl)
			{
				pos=i;
				break;
			}								
		LCD_Time=1000;	
		lcd_clear();
//		memset(lcd_txt, 0, sizeof(lcd_txt));	
		show_pos=pos & 0xfe;
		for(i=show_pos;i<show_pos+2;i++)					
		{
			cline=i-show_pos;
			lcd_set_pos(cline,1);
			if(i<TOP_RECORDS_NUM)	
			{						
				if(menu_str[i].lvl>password_lvl)
				{
					pos_old=i;
					for(j=i+1;j<(TOP_RECORDS_NUM);j++)
						if(menu_str[j].lvl<=password_lvl)
						{
							i=j;
							break;
						}
				}	
				if(menu_str[i].lvl<=password_lvl)
				{				
					__sprintf_P(lcd_txt,menu_str[i].str);					
					lcd_printf(lcd_txt);	
//					lcd_set_pos(cline,8);	
					switch(i)
					{
						case 0:			
//						case 1:
//						case 2:						
//						case 3:						
							lcd_set_pos(cline,1);
							tmp=io.ainput[pid[i].channel];
							if(tmp>999)tmp=999;
							ShowTemperatureShort(lcd_txt,&tmp);
							lcd_printf(lcd_txt);			
																
							lcd_set_pos(cline,7);
							memset(lcd_txt, 0, sizeof(lcd_txt));
							if(MenuRegs.lang==0)
							{
								if(pid[i].mode==PID_MODE_STOP)
									__sprintf_P(lcd_txt,eng_mode_of_operation_str[1]);
								if(pid[i].mode==PID_MODE_AUTO)
									__sprintf_P(lcd_txt,eng_mode_of_operation_str[0]);						
								if(pid[i].mode==PID_MODE_ERROR)
									__sprintf_P(lcd_txt,eng_mode_of_operation_str[3]);
								if(pid[current_pid].mode==PID_MODE_WARNING)
									__sprintf_P(lcd_txt,eng_mode_of_operation_str[4]);									
							}
							if(MenuRegs.lang==1)
							{
								if(pid[i].mode==PID_MODE_STOP)
									__sprintf_P(lcd_txt,ru_mode_of_operation_str[1]);
								if(pid[i].mode==PID_MODE_AUTO)
									__sprintf_P(lcd_txt,ru_mode_of_operation_str[0]);						
								if(pid[i].mode==PID_MODE_ERROR)
									__sprintf_P(lcd_txt,ru_mode_of_operation_str[3]);		
								if(pid[current_pid].mode==PID_MODE_WARNING)
									__sprintf_P(lcd_txt,ru_mode_of_operation_str[4]);														
							}
							lcd_printf(lcd_txt);									
							lcd_set_pos(cline,12);
							ShowTemperatureShort2(lcd_txt,&pid[i].t0);					
							lcd_printf(lcd_txt);
						break;				
						case 1:
							lcd_set_pos(cline,1);
							memset(lcd_txt, 0, sizeof(lcd_txt));
		//					__sprintf_P(lcd_txt,DaysOfWeek[errors.item[0].time.field.Day-1]);						
							ByteToStrFillZero(lcd_txt,&ft.field.Date,0,2,'0');lcd_printf(lcd_txt);lcd_printf(".");
							ByteToStrFillZero(lcd_txt,&ft.field.Month,0,2,'0');lcd_printf(lcd_txt);lcd_printf(".");					
		//					__sprintf_P(lcd_txt,Monthes_Name_Short[errors.item[0].time.field.Month-1]);lcd_printf(lcd_txt);
							ByteToStrFillZero(lcd_txt,&ft.field.Year,0,2,'0');lcd_printf(lcd_txt);
		
							lcd_set_pos(cline,10);
							memset(lcd_txt, 0, sizeof(lcd_txt));
							ByteToStrFillZero(lcd_txt,&ft.field.Hour,0,2,'0');lcd_printf(lcd_txt);lcd_printf(":");
							ByteToStrFillZero(lcd_txt,&ft.field.Minute,0,2,'0');//lcd_printf(lcd_txt);		
							lcd_printf(lcd_txt);											
						break;											
					}
				}
				else
				{
					__sprintf_P(lcd_txt,empty_str);				
					lcd_printf(lcd_txt);			
				}	
			}
			else
			{
				__sprintf_P(lcd_txt,empty_str);				
				lcd_printf(lcd_txt);			
			}			
		}			
		edit_pos_tmp=pos & 0x01;		
		lcd_set_pos(0,0);lcd_printf(" ");
		lcd_set_pos(1,0);lcd_printf(" ");
		switch(edit_pos_tmp)
		{
			case 0:lcd_set_pos(0,0);lcd_printf(">");break;
			case 1:lcd_set_pos(1,0);lcd_printf(">");break;
		}		
	}
	newmenuitem=0;
	if(LCD_Time==0)
		newmenuitem=1;	
	
		// -------- Keys --------------//
		// Up arrow
		if(key0==1)
		{
			pos_old=pos;
			if(pos!=0)pos--;
			for(i=pos;i!=0;i--)
				if(menu_str[i].lvl<=password_lvl)
				{
					pos=i;
					break;
				}
			if(menu_str[pos].lvl>password_lvl)pos=pos_old;
			newmenuitem=1;
			key0=2;			
		}
		// Down arrow		
		if(key1==1)
		{
			pos_old=pos;
			if(pos<(TOP_RECORDS_NUM-1))pos++;
			for(i=pos;i<(TOP_RECORDS_NUM-1);i++)
				if(menu_str[i].lvl<=password_lvl)
				{
					pos=i;
					break;
				}
			if(menu_str[pos].lvl>password_lvl)pos=pos_old;			
			newmenuitem=1;
			key1=2;
		}	
		// Esc
		if(key2==1)
		{
//			menu_stat=0;
			pos=0;
			if(MenuRegs.password!=0)
				if(LCDMenuPtr->UpItem!=NULL)
				{
					LCDMenuPtr=(TLCDMenuItem*)LCDMenuPtr->UpItem;						
					pos=0;										
				}	
			newmenuitem=1;				
			key2=2;			
		}
		// Enter		
		if(key3==1)
		{
			switch(pos)
			{
				case 0:
					if(password_lvl==0)break;
//					current_pid=pos2;			
					current_pid=0;								
					pos=0;
					LCDMenuPtr=&_LCD_Choose_Pid_Parameter;						
					newmenuitem=1;			
				break;
//				case 1:				
//					if(password_lvl==0)break;
//					current_pid=1;			
//					pos=0;
//					LCDMenuPtr=&_LCD_Choose_Pid_Parameter;						
//					newmenuitem=1;			
//				break;
				case 3:
					pos=0;
					LCDMenuPtr=&_LCD_Info;						
					newmenuitem=1;			
				break;
				case 2:
					pos=0;
					LCDMenuPtr=&_LCD_Show_Errors_Full;						
					newmenuitem=1;			
				break;							
			}	
			key3=2;			
		}
}
//-----------------------------------------------------------------------------------
