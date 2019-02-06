//------------------------------------------------------------------------------
#ifndef __TYPES_H
#define __TYPES_H
//------------------------------------------------------------------------------
#define c8  char
#define s8  signed char
#define s16 short
#define s32 long
#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned long
#define f32  float
#define d64  double

#define vs8  volatile char
#define vs16 volatile short
#define vs32 volatile long
#define vu8  volatile unsigned char
#define vu16 volatile unsigned short
#define vu32 volatile unsigned long
#define vf32 volatile float
#define vd64 volatile double

#define es8  extern char
#define es16 extern short
#define es32 extern long
#define eu8  extern unsigned char
#define eu16 extern unsigned short
#define eu32 extern unsigned long
#define ef32 extern float
#define ed64 extern double

#define evs8  extern volatile char
#define evs16 extern volatile short
#define evs32 extern volatile long
#define evu8  extern volatile unsigned char
#define evu16 extern volatile unsigned short
#define evu32 extern volatile unsigned long
#define evf32 extern volatile float
#define evd64 extern volatile double

typedef unsigned char           uchar;
typedef signed   short          sint;
typedef unsigned short          usint;
typedef signed   long           lint;
typedef unsigned long           ulint;

//#ifndef NULL
//#define NULL    ((void*)0)
//#endif

//#define bool u8
#ifndef bool
	#define bool	int
#endif
#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif
//------------------------------------------------------------------------------
#endif
