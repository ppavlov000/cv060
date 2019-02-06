/*---------------------------------------------------------------------------*/
/**
  @file		mbserial.c
  @brief	MOXA modbus serial API define file

  This file contains higher level modbus serial utility functions.
  It supports both RTU and ASCII protocol and converts them implicity.


  History:
  Date		Author			Comment
  08-02-2005	AceLan Kao.		Create it.

  @author AceLan Kao.(acelan_kao@moxa.com.tw)
 */
/*---------------------------------------------------------------------------*/

#include "mbserial.h"
#include "modbus_utils.h"

static struct MB_SERIAL_INFO MBSerialInfo[ MAX_PORT_NUM];

/*---------------------------------------------------------------------------*/
/**
  @brief	initializ serial port for modbus protocol
  @return	none
 */
/*---------------------------------------------------------------------------*/
void	MBSerialInit(void)
{
	int i= 0;

	for( i= 0; i< MAX_PORT_NUM; i++)
	{
		MBSerialInfo[ i].Protocol= -1;
		MBSerialInfo[ i].FrameTimeout= 3000;
		MBSerialInfo[ i].CharTimeout= 100;
		MBSerialInfo[ i].Timeout= 3;		///< in seconds
	}
}

/*---------------------------------------------------------------------------*/
/**
  @brief	open serial port for modbus protocol
  @param	port		port number
  @param	protocol	which protocol this port use
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialOpen(int port, int protocol)
{
	int RetValue;
	struct termios tio;
	if( ( protocol != MB_RTU_PROTOCOL)
		&& ( protocol != MB_ASCII_PROTOCOL)
		&& ( protocol != MB_TCP_PROTOCOL))
		return ( MB_ERROR_PROTOCOL);

	bzero(&tio, sizeof(tio));	///< clear struct for new port settings

	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_cflag = B9600|CS8|CREAD|CLOCAL;
	tio.c_lflag = 0;

	tio.c_cc[ VTIME] = 0;		///< inter-character timer unused
	tio.c_cc[ VMIN] = 1;		///< blocking read until 1 character arrives

	RetValue= SerialOpen( port, O_RDWR|O_NOCTTY, &tio);
	if( RetValue < 0)
		return MB_ERROR_OPEN;

	MBSerialInfo[ port].Protocol= protocol;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	read RTU data from port
  @param	port		port number
  @param	buf		buffer for store received data
  @param	len		buffer length
  @return	length of data actually read

  It will block to read data, and will return actually one frame of binary data
  It will verify the data, if format error, it return MB_ERROR_FORMAT
 */
/*---------------------------------------------------------------------------*/
int	MBSerialReadRTU( int port, u8 *buf, int len)
{
	u32 starttime, endtime;
	struct timeval tv;
	int readsocks, len1= 0, len2= 0;
	fd_set fdset;
	u16 crc, org_crc;

	tv.tv_sec = 0;					///< char timeout
	tv.tv_usec = MBSerialGetCharTimeout( port)* 1000;

	/// Serial timeout
	GETCLOCKSECOND( starttime);
	while( 1)
	{
		if( SerialDataInInputQueue( port) > 2)
			break;
		GETCLOCKSECOND( endtime);
		if( (endtime-starttime) > MBSerialGetTimeout( port))
			return 0;
	}

	len1= SerialBlockRead( port, (char *)buf, len);
	GETCLOCKMS( starttime);				///< for frame timeout

	FD_ZERO(&fdset);
	FD_SET( FindFD(port), &fdset);
	while( 1)
	{
		readsocks = select( FindFD(port)+ 1, &fdset, NULL, NULL, &tv);
		if(readsocks == 0)			///< char timeout
			break;

		len2= SerialBlockRead( port, (char *)buf+ len1, len- len1);
		len1+= len2;

		GETCLOCKMS( endtime);
		if( endtime > starttime)		///< frame timeout
		{
			if( endtime-starttime > MBSerialGetFrameTimeout( port))
				break;
		}
		else					///< number loop back
			if( ( U32_MAX_VALUE- starttime) + endtime > MBSerialGetFrameTimeout( port))
				break;
	}

	// Check CRC
	crc= CRC16( buf, len1- 2);
	GETU16( org_crc, buf+ len1- 2);
	if( crc != org_crc)
		return MB_ERROR_FORMAT;

	return len1;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	read ASCII data from port
  @param	port		port number
  @param	buf		buffer for store received data
  @param	len		buffer length
  @return	length of data actually read

  It will block to read data, and will return actually one frame of binary data
  It will verify the data, if format error, it return MB_ERROR_FORMAT
 */
/*---------------------------------------------------------------------------*/
int	MBSerialReadASCII( int port, u8 *buf, int len)
{
	u8 tmp[ MB_ASCII_ADU_MAX_LENGTH];
	int tmp_len= 0, ret_len= 0, end_flag= 0;
	u32 starttime, endtime;
	u8 ch;
	u8 lrc;

	struct timeval tv;
	int readsocks, len1= 0, len2= 0;
	fd_set fdset;

	tv.tv_sec = 1;					///< ascii char timeout
	tv.tv_usec = 0;

	/// Serial timeout
	GETCLOCKSECOND( starttime);
	while( 1)
	{
		if( SerialDataInInputQueue( port) > 2)
			break;
		GETCLOCKSECOND( endtime);
		if( (endtime-starttime) > MBSerialGetTimeout( port))
			return 0;
	}

	do
	{
		SerialBlockRead( port, (char *)&ch, 1);
	}while( ch != ':');

	FD_ZERO(&fdset);
	FD_SET( FindFD(port), &fdset);
	while( 1)
	{
		readsocks = select( FindFD(port)+ 1, &fdset, NULL, NULL, &tv);
		if(readsocks == 0)			// char timeout
			break;

		if( SerialBlockRead( port, (char *)&ch, 1))
		{
			if( ch == ':')			// a new frame start here!!
			{				// normally, we'll drop a frame
				tmp_len= 0;		// but it's better to read it in
			}
			else if( ch == 0x0d)		// prepare to stop
			{
				if( end_flag == 0)
					end_flag= 1;
				else			// receive two CR char
					return MB_ERROR_FORMAT;
			}
			else if( ch == 0x0a)
			{
				if( end_flag == 1)	// actually one frame receive
					break;
				else
					return MB_ERROR_FORMAT;
			}

			tmp[ tmp_len++]= ch;
		}
	}
	ret_len= MBASCIIToData( buf, tmp, tmp_len);

	// check LRC
	lrc= LRC( buf, ret_len- 1);
	if( lrc != buf[ ret_len- 1])
		return MB_ERROR_FORMAT;

	return ret_len;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	block read data from port
  @param	port		port number
  @param	buf		buffer for store received data
  @param	len		buffer length
  @return	length of data actually read

  It blocks and reads data, and will return actually one frame of binary data
  On error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialBlockRead(int port, u8 *buf, int len)
{
	int ret_len= 0;
	if( MBSerialInfo[ port].Protocol == MB_RTU_PROTOCOL)
		ret_len= MBSerialReadRTU( port, buf, len);
	else if( MBSerialInfo[ port].Protocol == MB_ASCII_PROTOCOL)
		ret_len= MBSerialReadASCII( port, buf, len);
	else
		return MB_ERROR_PROTOCOL;

	return ret_len;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	non-block read data from port
  @param	port		port number
  @param	buf		buffer for store received data
  @param	len		buffer length
  @return	length of data actually read
 */
/*---------------------------------------------------------------------------*/
int	MBSerialNonBlockRead(int port, u8 *buf, int len)
{
	return SerialNonBlockRead( port, (char *)buf, len);
}

/*---------------------------------------------------------------------------*/
/**
  @brief	write data to port
  @param	port		port number
  @param	pdu		PDU packet
  @param	len		PDU size
  @param	address		destination device id
  @return	length of data actually write, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialWrite(int port, u8 *pdu, int len, u8 address)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH];
	int adu_len;

	adu_len= MBMakeADU( adu, MBSerialInfo[ port].Protocol, address, pdu, len, 0);

	return SerialWrite( port, (char *)adu, adu_len);
}

/*---------------------------------------------------------------------------*/
/**
  @brief	close the serial port
  @param	port		port number
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialClose(int port)
{
	// TODO : need to do more checking before close
	return SerialClose( port);
}

/*---------------------------------------------------------------------------*/
/**
  @brief	set flow control
  @param	port		port number
  @param	control		HW_FLOW_CONTROL or SW_FLOW_CONTROL
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialFlowCtrl(int port, int control)
{
	return SerialFlowControl( port, control);
}

/*---------------------------------------------------------------------------*/
/**
  @brief	set serial port parameter
  @param	port		port number
  @param	parity		parity check, 0: none, 1: odd, 2: even, 3: space
  @param	databits	data bits
  @param	stopbit		stop bit
  @return	return MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialSetParam( int port, int parity, int databits, int stopbit)
{
	return SerialSetParam( port, parity, databits, stopbit);
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send adu packet through serial port
  		and wait response adu from slave
  @param	port		port number
  @param	buf		input buffer for receive adu from slave
  @param	pdu		pdu data prepare to send to slave
  @param	len		pdu length
  @param	address		slave id
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialSendAndWaitResponse(int port, u8 *buf, u8 *pdu, int len, u8 address)
{
	int ret_len= 0;

	MBSerialWrite( port, pdu, len, address);

	if( MBSerialInfo[ port].Protocol == MB_ASCII_PROTOCOL)
		ret_len= MBSerialReadASCII( port, buf, MB_ASCII_ADU_MAX_LENGTH);
	else if( MBSerialInfo[ port].Protocol == MB_RTU_PROTOCOL)
		ret_len= MBSerialReadRTU( port, buf, MB_RTU_ADU_MAX_LENGTH);
	else
		return MB_ERROR_PROTOCOL;

	// Multiple slave devices connect to the same serial port
	// TODO: make the response data useable
	//if( MBGetAddress( buf) != address)
	//	return MB_ERROR_EXECPTION;

	return ret_len;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	get frame timeout
  @param	port		port number
  @return	frame timeout value
 */
/*---------------------------------------------------------------------------*/
u32	MBSerialGetFrameTimeout(int port)
{
	return MBSerialInfo[ port].FrameTimeout;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	set frame timeout
  @param	port		port number
  @param	timeout		new frame timeout value
  @return	none
 */
/*---------------------------------------------------------------------------*/
void	MBSerialSetFrameTimeout(int port, u32 timeout)
{
	MBSerialInfo[ port].FrameTimeout= timeout;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	get char timeout
  @param	port		port number
  @return	char timeout value
 */
/*---------------------------------------------------------------------------*/
u32	MBSerialGetCharTimeout(int port)
{
	return MBSerialInfo[ port].CharTimeout;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	set char timeout
  @param	port		port number
  @param	timeout		new frame timeout value
  @return	none
 */
/*---------------------------------------------------------------------------*/
void	MBSerialSetCharTimeout(int port, u32 timeout)
{
	MBSerialInfo[ port].CharTimeout= timeout;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	get response timeout
  @param	port		port number
  @return	timeout value
 */
/*---------------------------------------------------------------------------*/
u32	MBSerialGetTimeout(int port)
{
	return MBSerialInfo[ port].Timeout;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	set new response timeout
  @param	port		port number
  @param	timeout		new response timeout value
  @return	none
 */
/*---------------------------------------------------------------------------*/
void	MBSerialSetTimeout(int port, u32 timeout)
{
	MBSerialInfo[ port].Timeout= timeout;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	get device address from ADU
  @param	adu		ADU data
  @return	none
 */
/*---------------------------------------------------------------------------*/
u8	MBSerialGetAddress( const u8 *adu)
{
	return MBGetAddress( adu);
}

/*---------------------------------------------------------------------------*/
/**
  @brief	get PDU part from ADU data
  @param	buf		output buffer
  @param	adu		ADU data with error check code
  @param	len		length of the adu data
  @return	the length of PDU data
 */
/*---------------------------------------------------------------------------*/
int	MBSerialGetPDU( u8 *buf, u8 *adu, int len)
{
	return MBGetPDU( buf, adu, len);
}

/*---------------------------------------------------------------------------*/
/**
  @brief	set serial port mode for RS232/RS422/RS485
  @param	port		port number
  @param	mode		serial port mode
		{RS232_MODE/RS485_2WIRE_MODE/RS422_MODE/RS485_4WIRE_MODE}
  @return	return MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int     MBSerialSetMode( int port, unsigned int mode)
{
	return SerialSetMode( port, mode);
}

/*---------------------------------------------------------------------------*/
/**
  @brief	set serial speed and make changes now
  @param	port		port number
  @param	speed		unsigned integer for new speed
  @return	return SERIAL_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialSetSpeed( int port, unsigned int speed)
{
	return SerialSetSpeed( port, speed);
}

/* Modbus master function */
/*---------------------------------------------------------------------------*/
/**
  @brief	send Read Decrete Inputs request and get response(0x02)
  @param	port		which port to use
  @param	address		slave device address
  @param	startdec	starting decrete inputs register number
  @param	no		number of registers, between 1 ~ 2000
  @param	value		registers status slave returned
  @return	number of status bytes, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialReadDecreteInputs(int port, u8 address, u16 startdec, u16 no, u8 *value)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;
	u8 ret_no;

	pdu_len= MBReadDecreteInputs( pdu, startdec, no);	///< make PDU

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseReadDecreteInputs( pdu, &ret_no, value);
	if( ret < 0)
		return ret;
	return ret_no;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Read Coils request and get response(0x01)
  @param	port		which port to use
  @param	address		slave device address
  @param	startcoils	starting coil register numbere
  @param	no		number of registers, between 1 ~ 2000
  @param	value		coils status slave returned
  @return	number of status bytes, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialReadCoils(int port, u8 address, u16 startcoils, u16 no, u8 *value)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;
	u8 ret_no;

	pdu_len= MBReadCoils( pdu, startcoils, no);		///< make PDU

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseReadCoils( pdu, &ret_no, value);
	if( ret < 0)
		return ret;
	return ret_no;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Write Single Coil request and get response(0x05)
  @param	port		which port to use
  @param	address		slave device address
  @param	coilreg		coil register number
  @param	onoff		0xFF00 for on,0x0000 for off.
  @return	return MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialWriteSingleCoil(int port, u8 address, u16 coilreg, u16 onoff)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;
	u16 ret_addr, ret_value;

	pdu_len= MBWriteSingleCoil( pdu, coilreg, onoff);	///< make PDU

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseWriteSingleCoil( pdu, &ret_addr, &ret_value);
	if( ret < 0)
		return ret;
	if( ( ret_addr != coilreg) || ( ret_value != onoff))
		return MB_ERROR_EXECPTION;
	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Write Multiple Coils request and get response(0x0F)
  @param	port		which port to use
  @param	address		slave device address
  @param	startcoils	starting coil register number
  @param	no		number of registers, between 1 ~ 2000
  @param	value		value to write
  @return	return MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialWriteMultipleCoils(int port, u8 address, u16 startcoils, u16 no, u8 *value)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;
	u16 count;
	u16 ret_addr, ret_value;

	count= (u8)no/8;
	if( count*8 < no)
		count++;

	/// make PDU
	pdu_len= MBWriteMultipleCoils( pdu, startcoils, no, count, value);

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseWriteMultipleCoils( pdu, &ret_addr, &ret_value);
	if( ret < 0)
		return ret;
	if( ( ret_addr != startcoils) || ( ret_value != no))
		return MB_ERROR_EXECPTION;
	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Read Input Registers request and get response(0x04)
  @param	port		which port to use
  @param	address		slave device address
  @param	startreg	starting coil register number
  @param	no		number of registers, between 1 ~ 125
  @param	value		register status slave returned
  @return	number of registers for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialReadInputRegisters(int port, u8 address, u16 startreg, u16 no, u16 *value)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;
	u8 ret_no;

	/// make PDU
	pdu_len= MBReadInputRegisters( pdu, startreg, no);

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseReadInputRegisters( pdu, &ret_no, value);
	if( ret < 0)
		return ret;
	return ret_no;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Read Holding Registers request and get response(0x03)
  @param	port		which port to use
  @param	address		slave device address
  @param	startreg	starting coil register number
  @param	no		number of registers, between 1 ~ 125
  @param	value		register status slave returned
  @return	number of registers for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialReadHoldingRegisters(int port, u8 address, u16 startreg, u16 no, u16 *value)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;
	u8 ret_no;

	/// make PDU
	pdu_len= MBReadHoldingRegisters( pdu, startreg, no);

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseReadHoldingRegisters( pdu, &ret_no, value);
	if( ret < 0)
		return ret;
	return ret_no;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Write Single Register request and get response(0x06)
  @param	port		which port to use
  @param	address		slave device address
  @param	devicereg	register number
  @param	value		register status slave returned
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialWriteSingleRegister(int port, u8 address, u16 devicereg, u16 value)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;
	u16 ret_addr, ret_value;

	/// make PDU
	pdu_len= MBWriteSingleRegister( pdu, devicereg, value);

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseWriteSingleRegister( pdu, &ret_addr, &ret_value);
	if( ret < 0)
		return ret;
	if( ( ret_addr != devicereg) || ( ret_value != value))
		return MB_ERROR_EXECPTION;
	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Write Multiple Registers request and get response(0x10)
  @param	port		which port to use
  @param	address		slave device address
  @param	startdevreg	starting register number
  @param	noreg		number of registers, between 1 ~ 120
  @param	count		value counter
  @param	value		register status slave returned
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialWriteMultipleRegisters(int port, u8 address, u16 startdevreg, u16 noreg, u8 count, u16 *value)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;
	u16 ret_addr, ret_value;

	/// make PDU
	pdu_len= MBWriteMultipleRegisters( pdu, startdevreg, noreg, count, value);		// make PDU

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseWriteMultipleRegisters( pdu, &ret_addr, &ret_value);
	if( ret < 0)
		return ret;
	if( ( ret_addr != startdevreg) || ( ret_value != noreg))
		return MB_ERROR_EXECPTION;
	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Read Write Multiple Registers request and get response(0x17)
  @param	port		which port to use
  @param	address		slave device address
  @param	rsreg		starting register number to read
  @param	rno		number of registers ro read, between 1 ~ 120
  @param	rv		registers value slave returned
  @param	wsreg		starting register number to write
  @param	wno		number of registers ro write, between 1 ~ 120
  @param	count		value counter
  @param	wv		value to write
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialReadWriteMultipleRegisters(int port, u8 address, u16 rsreg, u16 rno, u16 *rv, u16 wsreg, u16 wno, u8 count, u16 *wv)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;
	u8 ret_no;

	/// make PDU
	pdu_len= MBReadWriteMultipleRegisters( pdu, rsreg, rno, wsreg, wno, count, wv);		// make PDU

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseReadWriteMultipleRegisters( pdu, (u8 *)&ret_no, wv);

	if( ret < 0)
		return ret;
	if( ret_no != rno*2)
		return MB_ERROR_EXECPTION;
	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Mask Write Register request and get response(0x16)
  @param	port		which port to use
  @param	address		slave device address
  @param	reg		register number
  @param	andmask		and mask
  @param	ormask		or mask
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialMaskWriteRegister(int port, u8 address, u16 reg, u16 andmask, u16 ormask)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;
	u16 ret_reg, ret_and, ret_or;

	/// make PDU
	pdu_len= MBMaskWriteRegister( pdu, reg, andmask, ormask);

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseMaskWriteRegister( pdu, &ret_reg, &ret_and, &ret_or);

	if( ret < 0)
		return ret;
	if( ( ret_reg != reg) || ( ret_and != andmask) || ( ret_or != ormask))
		return MB_ERROR_EXECPTION;
	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Read FIFO Queue request and get response(0x18)
  @param	port		which port to use
  @param	address		slave device address
  @param	FIFOAddr	FIFO address
  @param	FIFOCount	FIFO value buffer size(<=31)
  @param	FIFOValue	values of FIFO register
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialReadFIFOQueue(int port, u8 address, u16 FIFOAddr, u16 *FIFOCount, u16 *FIFOValue)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;

	/// make PDU
	pdu_len= MBReadFIFOQueue( pdu, FIFOAddr);

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseReadFIFOQueue( pdu, FIFOCount, FIFOValue);

	if( ret < 0)
		return ret;

	return MB_OK;
}

/* Modbus master function over serial only */
/*---------------------------------------------------------------------------*/
/**
  @brief	send Read Exception Status request and get response(0x07)
  @param	port		which port to use
  @param	address		slave device address
  @param	status		eight exception status in one byte
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialReadExceptionStatus(int port, u8 address, u8 *status)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;

	/// make PDU
	pdu_len= MBReadExceptionStatus( pdu);

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseReadExceptionStatus( pdu, status);

	if( ret < 0)
		return ret;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Diagnostic request and get response(0x08)
  @param	port		which port to use
  @param	address		slave device address
  @param	subfunc		subfunction code
  @param	data		data to be sent to slave device
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialDiagnostic(int port, u8 address, u16 subfunc, u16 data)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;
	u16 ret_subfunc, ret_data;

	/// make PDU
	pdu_len= MBDiagnostic( pdu, subfunc, data);

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseDiagnostic( pdu, &ret_subfunc, &ret_data);

	if( ret < 0)
		return ret;
	if( ( ret_subfunc != subfunc) || ( ret_data != data))
		return MB_ERROR_EXECPTION;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Get Comm Event Counter request and get response(0x0B)
  @param	port		which port to use
  @param	address		slave device address
  @param	status		status of Comm Conuter
  @param	eventcount	event counter
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialGetCommEventCounter(int port, u8 address, u16 *status, u16 *eventcount)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;

	/// make PDU
	pdu_len= MBGetCommEventCounter( pdu);

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseGetCommEventCounter( pdu,  status,  eventcount);

	if( ret < 0)
		return ret;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Get Comm Event Counter request and get response(0x0C)
  @param	port		which port to use
  @param	address		slave device address
  @param	status		status of Comm Conuter
  @param	eventcount	event counter
  @param	messagecount	counter for processed message
  @param	events		new events
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialGetCommEventLog(int port, u8 address, u16 *status, u16 *eventcount, u16 *messagecount, u8 *events)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;

	/// make PDU
	pdu_len= MBGetCommEventLog( pdu);

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseGetCommEventLog( pdu, status, eventcount, messagecount, events);

	if( ret < 0)
		return ret;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Report Slave ID request and get response(0x11)
  @param	port		which port to use
  @param	address		slave device address
  @param	slave_id	buffer to store slave id
  @param	status		status of the device
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialReportSlaveID(int port, u8 address, u8 *slave_id, u8 *status)
{
	u8 adu[ MB_ASCII_ADU_MAX_LENGTH], pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0, ret= 0;

	/// make PDU
	pdu_len= MBReportSlaveID( pdu);

	adu_len= MBSerialSendAndWaitResponse( port, adu, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	pdu_len= MBGetPDU( pdu, adu, adu_len);
	ret= MBGetResponseReportSlaveID( pdu, slave_id, status);

	if( ret < 0)
		return ret;

	return MB_OK;
}

/* Modbus slave function or response to master */
/*---------------------------------------------------------------------------*/
/**
  @brief	send Read Decrete Inputs response(0x02)
  @param	port		which port to use
  @param	address		slave device address
  @param	no		number of registers
  @param	value		registers status
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialSendReadDecreteInputs(int port, u8 address, u16 no, u8 *value)
{
	u8 pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0;

	/// make PDU
	pdu_len= MBResponseReadDecreteInputs( pdu, no, value);

	adu_len= MBSerialWrite( port, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Read Coils response(0x01)
  @param	port		which port to use
  @param	address		slave device address
  @param	no		number of registers
  @param	value		registers status
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialSendReadCoils(int port, u8 address, u16 no, u8 *value)
{
	u8 pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0;

	/// make PDU
	pdu_len= MBResponseReadCoils( pdu, no, value);

	adu_len= MBSerialWrite( port, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Read Input Registers response(0x04)
  @param	port		which port to use
  @param	address		slave device address
  @param	no		number of registers
  @param	value		registers status
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialSendReadInputRegisters(int port, u8 address, u16 no, u16 *value)
{
	u8 pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0;

	/// make PDU
	pdu_len= MBResponseReadInputRegisters( pdu, no, value);

	adu_len= MBSerialWrite( port, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Read Holding Registers response(0x03)
  @param	port		which port to use
  @param	address		slave device address
  @param	no		number of registers
  @param	value		registers status
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialSendReadHoldingRegisters(int port, u8 address, u16 no, u16 *value)
{
	u8 pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0;

	/// make PDU
	pdu_len= MBResponseReadHoldingRegisters( pdu, no, value);

	adu_len= MBSerialWrite( port, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Write Multiple Registers response(0x10)
  @param	port		which port to use
  @param	address		slave device address
  @param	startdevreg	starting register address
  @param	noreg		number of registers, between 1 ~ 120
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialSendWriteMultipleRegisters(int port, u8 address, u16 startdevreg, u16 noreg)
{
	u8 pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0;

	/// make PDU
	pdu_len= MBResponseWriteMultipleRegisters( pdu, startdevreg, noreg);

	adu_len= MBSerialWrite( port, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Write Single Coil response(0x05)
  @param	port		which port to use
  @param	address		slave device address
  @param	coilreg		coil register address
  @param	onoff		0xFF00 for on,0x0000 for off
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialSendWriteSingleCoil(int port, u8 address, u16 coilreg, u16 onoff)
{
	u8 pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0;

	/// make PDU
	pdu_len= MBResponseWriteSingleCoil( pdu, coilreg, onoff);

	adu_len= MBSerialWrite( port, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Write Single Register response(0x06)
  @param	port		which port to use
  @param	address		slave device address
  @param	addr		register address
  @param	value		value to write
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int 	MBSerialSendWriteSingleRegister(int port, u8 address, u16 addr, u16 value)
{
	u8 pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0;

	/// make PDU
	pdu_len= MBResponseWriteSingleRegister( pdu, addr, value);

	adu_len= MBSerialWrite( port, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Read FIFO Queue response(0x18)
  @param	port		which port to use
  @param	address		slave device address
  @param	no		bytes of the following data
  @param	count		FIFO value buffer size(<=31)
  @param	value		values of FIFO register
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialSendReadFIFOueue(int port, u8 address, u16 no, u16 count, u16 *value)
{
	u8 pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0;

	/// make PDU
	pdu_len= MBResponseReadFIFOQueue( pdu, no, count, value);

	adu_len= MBSerialWrite( port, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Read Execption Status response(0x07)
  @param	port		which port to use
  @param	address		slave device address
  @param	status		eight exception status in one byte
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialSendReadExecptionStatus(int port, u8 address, u8 status)
{
	u8 pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0;

	/// make PDU
	pdu_len= MBResponseReadExceptionStatus( pdu, status);

	adu_len= MBSerialWrite( port, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Diagnostic response(0x08)
  @param	port		which port to use
  @param	address		slave device address
  @param	subfunc	subfunction code
  @param	data	response data of request
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int 	MBSerialSendDiagnostic(int port, u8 address, u16 subfunc, u16 data)
{
	u8 pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0;

	/// make PDU
	pdu_len= MBResponseDiagnostic( pdu, subfunc, data);

	adu_len= MBSerialWrite( port, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Get Comm Event Counter response(0x0B)
  @param	port		which port to use
  @param	address		slave device address
  @param	status		status of Comm Conuter
  @param	eventcount	event counter
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int	MBSerialSendGetCommEventCounter(int port, u8 address, u16 status, u16 eventcount)
{
	u8 pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0;

	/// make PDU
	pdu_len= MBResponseGetCommEventCounter( pdu, status, eventcount);

	adu_len= MBSerialWrite( port, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	return MB_OK;
}

/*---------------------------------------------------------------------------*/
/**
  @brief	send Get Comm Event Counter response(0x0C)
  @param	port		which port to use
  @param	address		slave device address
  @param	no		number of the following data
  @param	status		status of Comm Conuter
  @param	eventcount	event counter
  @param	messagecount	counter for processed message
  @param	events		new events
  @return	MB_OK for success, on error return error code
 */
/*---------------------------------------------------------------------------*/
int 	MBSerialSendGetCommEventLog(int port, u8 address, u8 no, u16 status, u16 eventcount, u16 messagecount, u8 *events)
{
	u8 pdu[ MAX_BUFFER_SIZE];
	int pdu_len= 0, adu_len= 0;

	/// make PDU
	pdu_len= MBResponseGetCommEventLog( pdu, no, status, eventcount, messagecount, events);

	adu_len= MBSerialWrite( port, pdu, pdu_len, address);

	if( adu_len < 0)
		return adu_len;

	return MB_OK;
}
