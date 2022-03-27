// Using the FT230XS in synchronous bit bang mode to program
// the PIC32MX family of microcontrollers. 
// Jesus Calvino-Fraga (2016-2017)
// Compile with visual C using:
// cl pro32.c

// #include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#pragma comment(lib, "FTD2XX.lib")
#define FTD2XX_STATIC
#include "ftd2xx.h"
#include <time.h>
#include <errno.h> 

clock_t startm, stopm;
#define START if ( (startm = clock()) == -1) {printf("Error calling clock");exit(1);}
#define STOP if ( (stopm = clock()) == -1) {printf("Error calling clock");exit(1);}
#define PRINTTIME printf( "%.1f seconds.", ((double)stopm-startm)/CLOCKS_PER_SEC);

#define ON  1
#define OFF 0

#define TCK 0x01  // TXD pin connects to pin 17 of PIC32 (TCK/RPB8[U2RX])
#define TDO 0x02  // RXD pin connects to pin 18 of PIC32 (TDO/RPB9[U2TX])
#define TDI 0x04  // RTS pin connects to pin 16 of PIC32 (TDI)
#define TMS 0x08  // CTS pin connects to pin 22 of PIC32 (TMS)
#define OUTPUTS (TDI|TCK|TMS)

#define MTAP_COMMAND        5 ,0x07
#define MTAP_SW_MTAP        5, 0x04
#define MTAP_SW_ETAP        5, 0x05
#define MTAP_IDCODE         5, 0x01
#define ETAP_ADDRESS        5, 0x08
#define ETAP_DATA           5, 0x09
#define ETAP_CONTROL        5, 0x0A
#define ETAP_EJTAGBOOT      5, 0x0C
#define ETAP_FASTDATA       5, 0x0E
#define MCHP_STATUS         8, 0x00 // NOP and return Status.
#define MCHP_ASSERT_RST     8, 0xD1 // Requests the Reset controller to assert device Reset.
#define MCHP_DE_ASSERT_RST  8, 0xD0 // Removes the request for device Reset.
#define MCHP_ERASE          8, 0xFC // Cause the Flash controller to perform a Chip Erase.
#define MCHP_FLASH_ENABLE   8, 0xFE // Enables fetches and loads to the Flash (from the processor).
#define MCHP_FLASH_DISABLE  8, 0xFD // Disables fetches and loads to the Flash (from the processor).
#define MCHP_READ_CONFIG    8, 0xFF
#define DATA_IDCODE        32, 0x00

#define CPS      0x80   // Code Protect State    ( 0 = is code protected )
#define NVMERR   0x20   // NVMCON Status bit     ( 1 = error occurred during NVM op )
#define CFGRDY   0x08   // Configuration Ready   ( 1 = Config read and CP valid )
#define FCBUSY   0x04   // Flash Controller Busy ( 1 = Flash controller busy )
#define FAEN     0x02   // Flash Access Enable   ( 1 = Flash Access Enabled )
#define DEVRST   0x01   // Device Reset State    ( 1 = Device Reset is Active )

#define NVMOP_NOP         0
#define NVMOP_WRITE_WORD  1
#define NVMOP_WRITE_ROW   3
#define NVMOP_ERASE_PAGE  4
#define NVMOP_ERASE_PFM   5   // Erase whole Program Flash Memory (PFM)

unsigned char bitloc[]={0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

#define BUFFSIZE 0x100000

unsigned int DeviceID;
char DeviceName[16];
int ProgramMode=OFF;
FT_HANDLE handle;
DWORD bytes;

unsigned char JTAG_Buffer[BUFFSIZE]; // Buffer used to transmit and receive JTAG bits
DWORD JTAG_Buffer_cnt;
short int Bit_Location[BUFFSIZE/2]; // Location of input bits in JTAG_Buffer
DWORD Bit_Location_cnt;
unsigned char Received_JTAG[BUFFSIZE/(8*2)]; // Decoded input bits
DWORD Received_JTAG_cnt;

#define PROGRAM_MAXSIZE      0x80000  // 512k (is the max for the MX series)
#define PROGRAM_BASE_ADDRESS 0x1D000000
#define BOOT_MAXSIZE         0xC00    // 3k (for MX series)
#define BOOT_BASE_ADDRESS    0x1FC00000
#define ROW_SIZE             32       // Number of words that can be programmed at the same time

#define DEVCFG0_MASK 0x1107FC1F
#define DEVCFG1_MASK 0x03FFF7A7
#define DEVCFG2_MASK 0xFFB700F7
#define DEVCFG3_MASK 0x30C00000
#define DEVID_MASK 0x0FFFFFFF

unsigned char Program_Flash_Buffer[PROGRAM_MAXSIZE];
unsigned char Boot_Flash_Buffer[BOOT_MAXSIZE];
char HexName[FILENAME_MAX]="";
int Selected_Device=-1;

/* msleep(): Sleep for the requested number of milliseconds. */
int Sleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

struct _IDList
{
    unsigned int DevID; // Has to be masked with 0x0fffffff
    char DevName[16];   // The program memory size from this string is 1024*atoi(&IDList[x].DevName[11])
};

struct _IDList IDList[] =
{
    {0x4A07053, "PIC32MX110F016B"},
    {0x4A09053, "PIC32MX110F016C"},
    {0x4A0B053, "PIC32MX110F016D"},
    {0x4A06053, "PIC32MX120F032B"},
    {0x4A08053, "PIC32MX120F032C"},
    {0x4A0A053, "PIC32MX120F032D"},
    {0x6A50053, "PIC32MX120F064H"},
    {0x4D07053, "PIC32MX130F064B"},
    {0x4D09053, "PIC32MX130F064C"},
    {0x4D0B053, "PIC32MX130F064D"},
    {0x6A00053, "PIC32MX130F128H"},
    {0x6A01053, "PIC32MX130F128L"},
    {0x4D06053, "PIC32MX150F128B"},
    {0x4D08053, "PIC32MX150F128C"},
    {0x4D0A053, "PIC32MX150F128D"},
    {0x6A10053, "PIC32MX150F256H"},
    {0x6A11053, "PIC32MX150F256L"},
    {0x6610053, "PIC32MX170F256B"},
    {0x661A053, "PIC32MX170F256D"},
    {0x6A30053, "PIC32MX170F512H"},
    {0x6A31053, "PIC32MX170F512L"},
    {0x4A01053, "PIC32MX210F016B"},
    {0x4A03053, "PIC32MX210F016C"},
    {0x4A05053, "PIC32MX210F016D"},
    {0x4A00053, "PIC32MX220F032B"},
    {0x4A02053, "PIC32MX220F032C"},
    {0x4A04053, "PIC32MX220F032D"},
    {0x4D01053, "PIC32MX230F064B"},
    {0x4D03053, "PIC32MX230F064C"},
    {0x4D05053, "PIC32MX230F064D"},
    {0x6A02053, "PIC32MX230F128H"},
    {0x6A03053, "PIC32MX230F128L"},
    {0x4D00053, "PIC32MX250F128B"},
    {0x4D02053, "PIC32MX250F128C"},
    {0x4D04053, "PIC32MX250F128D"},
    {0x6A12053, "PIC32MX250F256H"},
    {0x6A13053, "PIC32MX250F256L"},
    {0x6600053, "PIC32MX270F256B"},
    {0x660A053, "PIC32MX270F256D"},
    {0x6A32053, "PIC32MX270F512H"},
    {0x6A33053, "PIC32MX270F512L"},
    {0, "UNKNOWN"}
};

void Set_MCLR_to (int val);
void Dump_Received_JTAG (void);

void Load_Bits (int val)
{
	int j;
	
	if((JTAG_Buffer_cnt+1)>=BUFFSIZE)
	{
		printf("ERROR: Unable to load %d bytes in buffer.  Max is %d.\n", JTAG_Buffer_cnt+1, BUFFSIZE);
		exit(0);
	}

	JTAG_Buffer[JTAG_Buffer_cnt++]=(unsigned char) val;
}

void Decode_JTAG_Buffer (void)
{
	int i, j;
	
	Received_JTAG_cnt=0;
	for(i=0; i<Bit_Location_cnt; )
	{
		Received_JTAG[Received_JTAG_cnt]=0;
		for(j=0; j<8; j++) // For JTAG the least significant bit is transmitted first.
		{
			Received_JTAG[Received_JTAG_cnt]|=(JTAG_Buffer[Bit_Location[i++]]&TDO)?bitloc[j]:0;
		}
		Received_JTAG_cnt++;
	}
}

void Reset_JTAG_Buffer(void)
{
	JTAG_Buffer_cnt=0;
	Bit_Location_cnt=0;
	Received_JTAG_cnt=0;
}

void Send_JTAG_Buffer (void)
{
    FT_SetBitMode(handle, OUTPUTS, FT_BITMODE_SYNC_BITBANG); // Synchronous bit-bang mode
    FT_SetBaudRate(handle, 125000);  // Actually 125000*16/2=1MHz
	FT_SetLatencyTimer (handle, 7);
	
	JTAG_Buffer[JTAG_Buffer_cnt++]=0; //Need to write one more byte in order to get the last bit
    FT_Write(handle, JTAG_Buffer, JTAG_Buffer_cnt, &bytes);
    FT_Read(handle, JTAG_Buffer, JTAG_Buffer_cnt, &bytes);
	Decode_JTAG_Buffer();
}

void Send_JTAG_Buffer2(void)
{
    FT_SetBitMode(handle, OUTPUTS, FT_BITMODE_SYNC_BITBANG); // Synchronous bit-bang mode
    FT_SetBaudRate(handle, 125000);  // Actually 125000*16/2=1MHz
	FT_SetLatencyTimer (handle, 7);

	JTAG_Buffer[JTAG_Buffer_cnt++]=0; //Need to write one more byte in order to get the last bit
    FT_Write(handle, JTAG_Buffer, JTAG_Buffer_cnt, &bytes);
    FT_Read(handle, JTAG_Buffer, JTAG_Buffer_cnt, &bytes);
}

int hex2dec (char hex_digit)
{
   int j;
   j=toupper(hex_digit)-'0';
   if (j>9) j -= 7;
   return j;
}

unsigned char GetByte(char * buffer)
{
	return hex2dec(buffer[0])*0x10+hex2dec(buffer[1]);
}

unsigned short GetWord(char * buffer)
{
	return hex2dec(buffer[0])*0x1000+hex2dec(buffer[1])*0x100+hex2dec(buffer[2])*0x10+hex2dec(buffer[3]);
}

int Read_Hex_File(char * filename)
{
	char buffer[1024];
	FILE * filein;
	int j;
	unsigned char linesize, recordtype, rchksum, value;
	unsigned int ext_address=0, address=0, total_address;
	int chksum;
	int line_counter=0;
	int numread=0;

	//Set the flash buffer to its default value
	memset(Program_Flash_Buffer, 0xff, PROGRAM_MAXSIZE);
	memset(Boot_Flash_Buffer, 0xff, BOOT_MAXSIZE);

    if ( (filein=fopen(filename, "r")) == NULL )
    {
       printf("Error: Can't open file `%s`.\r\n", filename);
       return -1;
    }

    while(fgets(buffer, sizeof(buffer), filein)!=NULL)
    {
    	numread+=(strlen(buffer)+1);

    	line_counter++;
    	if(buffer[0]==':')
    	{
			linesize = GetByte(&buffer[1]);
			address = GetWord(&buffer[3]);
			recordtype = GetByte(&buffer[7]);
			rchksum = GetByte(&buffer[9]+(linesize*2));
			chksum=linesize+(address/0x100)+(address%0x100)+recordtype+rchksum;

			if (recordtype==1) break; // End of record

			else if (recordtype==0) // Data record
			{
				for(j=0; j<linesize; j++)
				{
					value=GetByte(&buffer[9]+(j*2));
					chksum+=value;
					total_address=ext_address+address+j;
					
					// Data records for the PIC32 can be boot flash, program flash, and configuration words
					if( (total_address>=BOOT_BASE_ADDRESS) &&
					    (total_address<(BOOT_BASE_ADDRESS+BOOT_MAXSIZE)) ) // boot flash and configuration
					{
						Boot_Flash_Buffer[total_address-BOOT_BASE_ADDRESS]=value;
					}
					else if ( (total_address>=PROGRAM_BASE_ADDRESS) &&
					          (total_address<(PROGRAM_BASE_ADDRESS+PROGRAM_MAXSIZE)) ) // program flash
					{
						Program_Flash_Buffer[total_address-PROGRAM_BASE_ADDRESS]=value;
					}
				}
		    }
		    else if (recordtype==4) // Extended address record
		    {
		    	// :020000041fc01b
				ext_address = GetWord(&buffer[9]);
				chksum+=ext_address/256;
				chksum+=ext_address%256;
				ext_address <<= 16;
		    }
		    else // Don't know what to do with other record types.  Just proccess them, and throw a warning
		    {
				printf("WARNING: found HEX record type 0x%02x in file %s:%d\r\n", recordtype, filename, line_counter);
				value=GetByte(&buffer[9]+(j*2));
				chksum+=value;
		    }

			if((chksum%0x100)!=0)
			{
				printf("ERROR: Bad checksum in file '%s' at line %d\r\n", filename, line_counter);
				return -1;
			}
		}
    }
    fclose(filein);

    return 0;
}

unsigned int Make_Uint(unsigned char * v)
{
	return v[0]+v[1]*0x100+v[2]*0x10000+v[3]*0x1000000;
}

/*
6.1 SetMode Pseudo Operation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Format:
	SetMode (mode)
Purpose:
	To set the EJTAG state machine to a specific state.
Description:
	The value of mode is clocked into the device on
	signal TMS. TDI is set to a �0� and TDO is ignored.
Restrictions:
	None.
Example:
	Set Mode to (6�b011111): SetMode(6, 0x1F);
*/

void SetMode(unsigned char nbits, unsigned int mode)
{
	while (nbits--)
	{
		Load_Bits( (mode & 0x01)?TMS:0);      // TMS=bit, TDI=0, TCK=0
		Load_Bits(((mode & 0x01)?TMS:0)|TCK); // TMS=bit, TDI=0, TCK=1
		mode >>= 1;
	}
	Load_Bits(0); // TMS=0, TDI=0, TCK=0
}

/*
6.2 SendCommand Pseudo Operation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Format:
	SendCommand (command)
Purpose:
	To send a command to select a specific TAP register.
Description (in sequence):
	1. The TMS Header is clocked into the device to	select the Shift IR state
	2. The command is clocked into the device on TDI while holding signal TMS low.
	3. The last Most Significant bit (MSb) of the command is clocked in while setting TMS high.
	4. The TMS Footer is clocked in on TMS to return the TAP controller to the Run/Test Idle state.
Restrictions:
	None.
Example:
	SendCommand (5�h0x07)
*/
void SendCommand(unsigned char nbits, unsigned int cmd)
{
	Load_Bits(TMS);     // TDI=0, TMS=1, TCK=0
	Load_Bits(TMS|TCK); // TDI=0, TMS=1, TCK=1
	Load_Bits(TMS);     // TDI=0, TMS=1, TCK=0
	Load_Bits(TMS|TCK); // TDI=0, TMS=1, TCK=1
	Load_Bits(0);       // TDI=0, TMS=0, TCK=0
	Load_Bits(TCK);     // TDI=0, TMS=0, TCK=1
	Load_Bits(0);       // TDI=0, TMS=0, TCK=0
	Load_Bits(TCK);     // TDI=0, TMS=0, TCK=1
	
	while (nbits--)
	{
		Load_Bits(((cmd & 0x01)?TDI:0)|((!nbits)?TMS:0));     // TMS=1 for last bit, TDI=bit, TCK=0
		Load_Bits(((cmd & 0x01)?TDI:0)|((!nbits)?TMS:0)|TCK); // TMS=1 for last bit, TDI=bit, TCK=1
		cmd >>= 1;
		Bit_Location[Bit_Location_cnt++]=JTAG_Buffer_cnt; // The location in the buffer of the received bit in TDO
	}
	Load_Bits(TMS);     // TDI=0, TMS=1, TCK=0
	Load_Bits(TMS|TCK); // TDI=0, TMS=1, TCK=1
	Load_Bits(0);       // TDI=0, TMS=0, TCK=0
	Load_Bits(TCK);     // TDI=0, TMS=0, TCK=1
	Load_Bits(0);       // TDI=0, TMS=0, TCK=0
}

/*
6.3 XferData Pseudo Operation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Format:
	oData = XferData (iData)
Purpose:
	To clock data to and from the register selected by thecommand.
Description (in sequence):
	1. The TMS Header is clocked into the device to select the Shift DR state.
	2. The data is clocked in/out of the device on TDI/TDO while holding signal TMS low.
	3. The last MSb of the data is clocked in/out while setting TMS high.
	4. The TMS Footer is clocked in on TMS to return the TAP controller to the Run/Test Idle state.
Restrictions:
	None.
Example:
	oData = XferData (32�h0x12)
*/
void XferData(unsigned char nbits, unsigned int val)
{
	Load_Bits(TMS);     // TDI=0, TMS=1, TCK=0
	Load_Bits(TMS|TCK); // TDI=0, TMS=1, TCK=1
	Load_Bits(0);       // TDI=0, TMS=0, TCK=0
	Load_Bits(TCK);     // TDI=0, TMS=0, TCK=1
	Load_Bits(0);       // TDI=0, TMS=0, TCK=0
	Load_Bits(TCK);     // TDI=0, TMS=0, TCK=1
	
	while (nbits--)
	{
		Load_Bits(((val & 0x01)?TDI:0)|((!nbits)?TMS:0));     // TMS=1 for last bit, TDI=bit, TCK=0
		Load_Bits(((val & 0x01)?TDI:0)|((!nbits)?TMS:0)|TCK); // TMS=1 for last bit, TDI=bit, TCK=1
		val >>= 1;
		Bit_Location[Bit_Location_cnt++]=JTAG_Buffer_cnt; // The location in the buffer of the received bit in TDO
	}
	Load_Bits(TMS);     // TDI=0, TMS=1, TCK=0
	Load_Bits(TMS|TCK); // TDI=0, TMS=1, TCK=1
	Load_Bits(0);       // TDI=0, TMS=0, TCK=0
	Load_Bits(TCK);     // TDI=0, TMS=0, TCK=1
	Load_Bits(0);       // TDI=0, TMS=0, TCK=0
}

/*
6.4 XferFastData Pseudo Operation
Format:
	oData = XferFastData (iData)
Purpose:
	To quickly send 32 bits of data in/out of the device.
Description (in sequence):
	1. The TMS Header is clocked into the device to select the Shift DR state.
	2. The input value of the PrAcc bit, which is �0�, is clocked in.
	3. TMS Footer = 10 is clocked in to return the TAP controller to the Run/Test Idle state.
Restrictions:
	The SendCommand (ETAP_FASTDATA) must be sent first to select the Fastdata register.
*/
void XferFastData(unsigned int cmd)
{
	unsigned char nbits=32;
	
	Load_Bits(TMS);     // TDI=0, TMS=1, TCK=0
	Load_Bits(TMS|TCK); // TDI=0, TMS=1, TCK=1
	Load_Bits(0);       // TDI=0, TMS=0, TCK=0
	Load_Bits(TCK);     // TDI=0, TMS=0, TCK=1
	Load_Bits(0);       // TDI=0, TMS=0, TCK=0
	Load_Bits(TCK);     // TDI=0, TMS=0, TCK=1
	Load_Bits(0);       // TDI=0, TMS=0, TCK=0
	Load_Bits(TCK);     // TDI=1, TMS=0, TCK=1
	
	while (nbits--)
	{
		Load_Bits(((cmd & 0x01)?TDI:0)|((!nbits)?TMS:0));     // TMS=1 for last bit, TDI=bit, TCK=0
		Load_Bits(((cmd & 0x01)?TDI:0)|((!nbits)?TMS:0)|TCK); // TMS=1 for last bit, TDI=bit, TCK=1
		Bit_Location[Bit_Location_cnt++]=JTAG_Buffer_cnt;     // The location in the buffer of the received bit in TDO
		cmd >>= 1;
	}
	Load_Bits(TMS);     // TDI=0, TMS=1, TCK=0
	Load_Bits(TMS|TCK); // TDI=0, TMS=1, TCK=1
	Load_Bits(0);       // TDI=0, TMS=0, TCK=0
	Load_Bits(TCK);     // TDI=0, TMS=0, TCK=1
	Load_Bits(0);       // TDI=0, TMS=0, TCK=0
}

/*
Format:
	XferInstruction (instruction)
Purpose:
	To send 32 bits of data for the device to execute.
Description:
	The instruction is clocked into the device and then executed by CPU.
Restrictions:
	The device must be in Debug mode.
*/

void XferInstruction(unsigned int instr)
{
	//unsigned int controlVal = 0;
	int j;
	
	SendCommand(ETAP_CONTROL);
	XferData(32, 0x0004C000);
	// Using USB this is not afordable as it takes 2ms per each USB access.
	// Leaving it here commented out to avoid putting it back in the future.
	//do{
	//	controlVal = XferData(32, 0x0004C000);
	//	if ((controlVal & 0x00040000) == 0) Sleep(100);
	//} while ( (controlVal & 0x00040000) == 0 );
	SendCommand(ETAP_DATA);
	XferData(32, instr);
	SendCommand(ETAP_CONTROL);
	XferData(32, 0x0000C000);
}

void EnterProgamMode()
{
	Set_MCLR_to(0);
	Reset_JTAG_Buffer();
    SendCommand(MTAP_SW_ETAP);
    SendCommand(ETAP_EJTAGBOOT);
	Send_JTAG_Buffer();
	Set_MCLR_to(1);
    ProgramMode = ON;
}

void ExitProgamMode()
{
	Reset_JTAG_Buffer();
    SetMode(5, 0x1f);
	Send_JTAG_Buffer();
	Set_MCLR_to(1);
    ProgramMode = OFF;
}

void ReadIDCodeRegister(void)
{
	/* The following steps are required to check the device status using the 4-wire interface:
		1. Set the MCLR pin low.
		2. SetMode (6�b011111) to force the Chip TAPcontroller into Run Test/Idle state.
		3. SendCommand (MTAP_SW_MTAP).
		4. SetMode (6�b011111) to force the Chip TAP controller into Run Test/Idle state.
		5. SendCommand (MTAP_COMMAND).
		6. statusVal = XferData (MCHP_STATUS).
		7. If CFGRDY (statusVal<3>) is not �1� and FCBUSY (statusVal<2>) is not �0� GOTO step 5.
	*/
	
	int j;
	
	Set_MCLR_to(0);
	
	Reset_JTAG_Buffer();
    SetMode(6, 0x1f);
    SendCommand(MTAP_SW_MTAP);
    SetMode(6, 0x1f);
    SendCommand(MTAP_COMMAND);
	Bit_Location_cnt=0; // Discard previusly received bits
    XferData(MCHP_STATUS);
	Send_JTAG_Buffer();

    if ( ((Received_JTAG[0] & CFGRDY) == 0) || (Received_JTAG[0] & FCBUSY) )
    {
		printf("ERROR: Can not communicate with PIC32\n");
		exit(-1);
    }

	Reset_JTAG_Buffer();
	SendCommand(MTAP_IDCODE);
	Bit_Location_cnt=0; // Discard previusly received bits
	XferData(DATA_IDCODE);
	Send_JTAG_Buffer();
	
	DeviceID=Make_Uint(&Received_JTAG[0]) & 0x0FFFFFFF ;// 0x0FFFFFFF is the DEVID mask in the datasheet
	
	for(j=0; IDList[j].DevID!=0; j++)
	{
		if(IDList[j].DevID==DeviceID)
		{
			strcpy(DeviceName, IDList[j].DevName);
			printf("IDCode=0x%x : %s\n", DeviceID, DeviceName); fflush(stdout);
			return;
		}
	}
	printf("IDCode=0x%x is not known to this program.\n", DeviceID); fflush(stdout);
	exit(-4);
}

unsigned int EraseChip(void)
{
	/* The following steps are required to erase a target device:
	1. SendCommand (MTAP_SW_MTAP).
	2. SetMode (6�b011111).
	3. SendCommand (MTAP_COMMAND).
	4. XferData (MCHP_ERASE).
	5. XferData (MCHP_DE_ASSERT_RST). This step is	not required for PIC32MX devices.
	6. Delay 10 ms.
	7. statusVal = XferData (MCHP_STATUS).
	8. If CFGRDY (statusVal<3>) is not �1� and 	FCBUSY (statusVal<2>) is not �0�, GOTO to step 	5.
	*/
	
	Reset_JTAG_Buffer();
    SendCommand(MTAP_SW_MTAP);
    SetMode(6, 0x1f);
    SendCommand(MTAP_COMMAND);
    XferData(MCHP_ERASE);
    //XferData(MCHP_DE_ASSERT_RST);
	Send_JTAG_Buffer();

	Sleep(10);
    
	Reset_JTAG_Buffer();
    XferData(MCHP_STATUS);
	Send_JTAG_Buffer();

    if ( ((Received_JTAG[0] & CFGRDY) == 0) || (Received_JTAG[0] & FCBUSY) )
    {
		printf("ERROR: Can not erase PIC32\n");
		exit(-1);
    }
    return 0;
}

void DownloadData( unsigned short ram_addr, unsigned int data )
{      
	Reset_JTAG_Buffer();
    XferInstruction( 0x3c10a000 );                 // lui s0, 0xa000
    XferInstruction( 0x36100000 );                 // ori s0, 0
    XferInstruction( 0x3c080000 + (data>>16) );    // lui t0, <DATA(31:16)>
    XferInstruction( 0x35080000 + (data&0xffff) ); // ori t0, <DATA(15:0)>
    XferInstruction( 0xae080000 + ram_addr );      // sw t0, <OFFSET>(S0)

    // Copy the same data to NVMDATA register for WRITE_WORD method
    XferInstruction( 0x3c04bf80 ); // lui a0, 0xbf80
    XferInstruction( 0x3484f400 ); // ori a0, 0xf400
    XferInstruction( 0xac880030 ); // sw t0, 48(a0)
	Send_JTAG_Buffer();
}

void DownloadDataBlock( unsigned char * val, unsigned int n )
{
	unsigned int data, j;
	
	Reset_JTAG_Buffer();
    XferInstruction( 0x3c10a000 ); // lui s0, 0xa000
    for(j=0; j<n; j+=4)
    {
    	// Danger!  This is RAM.  So EVERY word needs to be initialized.
    	data=Make_Uint(&val[j]);
	    XferInstruction( 0x3c080000 + (data>>16) );    // lui t0, <DATA(31:16)>
	    XferInstruction( 0x35080000 + (data&0xffff) ); // ori t0, <DATA(15:0)>
	    XferInstruction( 0xae080000 + j );             // sw t0, <OFFSET>(S0)
    }
	Send_JTAG_Buffer2();
}

void FlashOperation( unsigned char nvmop, unsigned int flash_addr, unsigned int ram_addr )
{
	Reset_JTAG_Buffer();
    XferInstruction( 0x00000000 ); // nop

    // FROM PIC32MX flash programming specification 61145J
    // Step 1: Initialize constants
        
    XferInstruction( 0x3c04bf80 );// lui a0, 0xbf80
    XferInstruction( 0x3484f400 );// ori a0, 0xf400
    XferInstruction( 0x34054000 + nvmop );// ori a1, $0, 0x4000 + NVMOP<3:0>
    XferInstruction( 0x34068000 );// ori a2, $0, 0x8000
    XferInstruction( 0x34074000 );// ori a3, $0, 0x4000
    XferInstruction( 0x3c11aa99 );// lui s1, 0xaa99
    XferInstruction( 0x36316655 ); // ori s1, 0x6655
    XferInstruction( 0x3c125566 );// lui s2, 0x5566
    XferInstruction( 0x365299aa );// ori s2, 0x99aa
    //XferInstruction( 0x3c13ff20 );// lui s3, 0xff20

    // Step 2, set NVMADDR (row to be programmed)
        
    XferInstruction( 0x3c080000 + (flash_addr>>16) );// lui t0, <FLASH_ROW_ADDR(31:16)>
    XferInstruction( 0x35080000 + (flash_addr&0xffff) );// ori t0, <FLASH_ROW_ADDR(15:0)>
    XferInstruction( 0xac880020 );// sw t0, 32(a0)

    // Step 3, set NVMSRCADDR (source RAM addr)
        
    XferInstruction( 0x3c100000 );// lui s0, <RAM_ADDR(31:16)>
    XferInstruction( 0x36100000 + ram_addr );// ori s0, <RAM_ADDR(15:0)>
    XferInstruction( 0xac900040 );// sw s0, 64($a0) 

    // Step 4, Set up NVMCON write and poll STAT
       
    XferInstruction( 0xac850000 ); // sw a1, 0(a0)
    XferInstruction( 0x8c880000 ); // lw t0, 0(a0)
    XferInstruction( 0x31080800 ); // andi t0, 0x0800
    XferInstruction( 0x1500fffd ); // bne t0, $0, <here1> 
    XferInstruction( 0x00000000 ); // nop

    // Step 5, unlock NVMCON and start write
        
    XferInstruction( 0xac910010 );// sw s1, 16(a0)
    XferInstruction( 0xac920010 );// sw s2, 16(a0)
    XferInstruction( 0xac860008 );// sw a2, 8(a0)  NOTE: offset 8 here is the "set register"

    // Step 6, Poll for NVMCON(WR) bit to get cleared

    XferInstruction( 0x8c880000 ); // lw t0, 0(a0)
    XferInstruction( 0x01064024 ); // and t0, t0, a2
    XferInstruction( 0x1500fffd ); // bne t0, $0, <here2>
    XferInstruction( 0x00000000 ); // nop

    // Step 7, Wait at least 500ns, 8MHz clock assumed
            
    XferInstruction( 0x00000000 ); // nop
    XferInstruction( 0x00000000 ); // nop
    XferInstruction( 0x00000000 ); // nop
    XferInstruction( 0x00000000 ); // nop

    // Step 8, Clear NVMCON(WREN) bit
            
    XferInstruction( 0xac870004 ); // sw a3, 4(a0)  NOTE: Offset 4 here is the "clear" register

    // Step 9, Check NVMCON(WRERR) bit to ensure correct operation
   
    /*Since I an not using the information bellow...         
    XferInstruction( 0x8c880000 ); // lw t0, 0(a0)
	//XferInstruction( 0x30082000 );// andi t0, $0, 0x2000
    XferInstruction( 0x3c13ff20 ); // lui s3, 0xFF20
    XferInstruction( 0x36730000 ); // ori s3, 0       
    XferInstruction( 0xae680000 ); // sw t0, 0(s3) = transfer NVMCON via fastdata
                
    // XferInstruction( 0x15000000 + err_proc_offs ); // bne t0, $0, <err_proc_offset>
    XferInstruction( 0x00000000 ); // nop

    SendCommand( ETAP_FASTDATA );
    XferFastData( 0 );*/
    
    XferInstruction( 0x00000000 ); // nop
	Send_JTAG_Buffer2();
}

unsigned int ReadFlashData( unsigned int flash_addr )
{                
	Reset_JTAG_Buffer();
    XferInstruction( 0x3c13ff20 ); // lui s3, 0xFF20
    XferInstruction( 0x36730000 );  // ori s3, 0       
    XferInstruction( 0x3c080000 + (flash_addr>>16) ); // lui t0, <FLASH_WORD_ADDR(31:16)>
    XferInstruction( 0x35080000 + (flash_addr&0xffff) ); // ori t0, <FLASH_WORD_ADDR(15:0)>
    XferInstruction( 0x8d090000 );  // lw t1, 0(t0) 
    XferInstruction( 0xae690000 ); // sw t1, 0(s3)
    SendCommand( ETAP_FASTDATA );
	Bit_Location_cnt=0; // Discard previusly received bits
    XferFastData( 0 );
	Send_JTAG_Buffer();
	return Make_Uint(&Received_JTAG[0]);
}

void ReadFlashDataBlock( unsigned int flash_addr, int n )
{
	int j, k;
	DWORD saved_Bit_Location_cnt;
	                
	Reset_JTAG_Buffer();

	for(j=0; j<n; j+=4)
	{
		saved_Bit_Location_cnt=Bit_Location_cnt;
	    XferInstruction( 0x3c13ff20 );                       // lui s3, 0xFF20
	    XferInstruction( 0x36730000 );                       // ori s3, 0
	    XferInstruction( 0x3c080000 + (flash_addr>>16) );    // lui t0, <FLASH_WORD_ADDR(31:16)>
	    XferInstruction( 0x35080000 + (flash_addr&0xffff) ); // ori t0, <FLASH_WORD_ADDR(15:0)>
	    XferInstruction( 0x8d090000 );                       // lw t1, 0(t0) 
	    XferInstruction( 0xae690000 );                       // sw t1, 0(s3)
	    SendCommand( ETAP_FASTDATA );
		Bit_Location_cnt=saved_Bit_Location_cnt;
	    XferFastData(0);
	    flash_addr+=4;
	}
	Send_JTAG_Buffer();
	// At this point Received_JTAG contains the bytes of 32-bit words we want
}

void Dump_Program_Flash_Buffer (int start, int n)
{
	int i;

	for(i=0; i<n; i++)
	{
		if((i&0xf)==0) printf("\n%08x: ", i+start);
		printf(" %02x", Program_Flash_Buffer[start+i]);
	}
	printf("\n");
}
   
void Dump_Received_JTAG (void)
{
	int i;

	for(i=0; i<Received_JTAG_cnt; i++)
	{
		if((i&0xf)==0) printf("\n%08x: ", i);
		printf(" %02x", Received_JTAG[i]);
	}
	printf("\n");
}

void VerifyFlash (void)
{
	#define VBLOCK 64
	unsigned int j, k, nbytes, total_bytes, star_count;
	int verify_block;
	
	START;
	EnterProgamMode();
	
	star_count=0;
	nbytes=0;
	printf("\nVerifiying program flash: ");
	for(j=0; j<PROGRAM_MAXSIZE; j+=VBLOCK)
	{
		verify_block=0;
		// Check if ANY byte in the row is different from 0xff
		for(k=0; k<(ROW_SIZE*4); k++)
		{
			if(Program_Flash_Buffer[(j&-(ROW_SIZE*4))+k]!=0xff)
			{
				verify_block=1;
				break;
			}
		}
		if(verify_block==1)
		{
			nbytes+=VBLOCK;
			if((j%(ROW_SIZE*4))==0)
			{
				if(star_count==50)
				{
					star_count=0;
					printf("\nVerifiying program flash: ");
				}
				printf("#"); fflush(stdout);
				star_count++;
			}
			ReadFlashDataBlock(PROGRAM_BASE_ADDRESS+j, VBLOCK);
			for(k=0; k<VBLOCK; k++)
			{
				if(Received_JTAG[k]!=Program_Flash_Buffer[j+k])
				{
					printf("\nError at location 0x%08x\n", PROGRAM_BASE_ADDRESS+j+k);
					return;
				}
			}
		}
	}
	printf(" Done (%d bytes).", nbytes); fflush(stdout);
    total_bytes=nbytes;
	
	star_count=0;
	nbytes=0;
	printf("\nVerifiying boot flash: ");
	for(j=0; j<BOOT_MAXSIZE; j+=VBLOCK)
	{
		verify_block=0;
		// Check if ANY byte in the row is different from 0xff
		for(k=0; k<(ROW_SIZE*4); k++)
		{
			if(Boot_Flash_Buffer[(j&-(ROW_SIZE*4))+k]!=0xff)
			{
				verify_block=1;
				break;
			}
		}
		if(verify_block==1)
		{
			nbytes+=VBLOCK;
			if((j%(ROW_SIZE*4))==0)
			{
				if(star_count==50)
				{
					star_count=0;
					printf("\nVerifiying program flash: ");
				}
				printf("#"); fflush(stdout);
				star_count++;
			}
			ReadFlashDataBlock(BOOT_BASE_ADDRESS+j, VBLOCK);
			for(k=0; k<VBLOCK; k++)
			{
				unsigned char mask=0xff;
				if( (j+k)==(BOOT_MAXSIZE-1)) mask=0x7f;
				if(Received_JTAG[k]!=(Boot_Flash_Buffer[j+k]&mask))
				{
					printf("\nError at location 0x%08x: read 0x%02x but should be 0x%02x\n",
						BOOT_BASE_ADDRESS+j+k, Received_JTAG[k], Boot_Flash_Buffer[j+k]);
					return;
				}
			}
		}
	}
    printf(" Done (%d bytes).\n", nbytes); fflush(stdout);
	total_bytes+=nbytes;
	
	ExitProgamMode();
	
    printf("Done verifying %d bytes in ", total_bytes);
    STOP;
	PRINTTIME;
    printf(" (%.1f bytes/sec)\n",  total_bytes/(((double)stopm-startm)/CLOCKS_PER_SEC));
	fflush(stdout);
}

void FTDI_Configuration (void)
{
	FT_STATUS status;
	char Manufacturer[64];
	char ManufacturerId[64];
	char Description[64];
	char SerialNumber[64];
	
	FT_EEPROM_HEADER ft_eeprom_header;
	FT_EEPROM_X_SERIES ft_eeprom_x_series;
	
	ft_eeprom_header.deviceType = FT_DEVICE_X_SERIES; // FTxxxx device type to be accessed
	ft_eeprom_x_series.common = ft_eeprom_header;
	ft_eeprom_x_series.common.deviceType = FT_DEVICE_X_SERIES;
	
	status = FT_EEPROM_Read(handle, &ft_eeprom_x_series, sizeof(ft_eeprom_x_series),
							Manufacturer, ManufacturerId, Description, SerialNumber);
	if (status != FT_OK)
	{
		printf("EEPROM_Read status not ok %d\n", status);
	}
	else
	{
		printf("VendorID = 0x%04x\n", ft_eeprom_x_series.common.VendorId);
		printf("ProductID = 0x%04x\n", ft_eeprom_x_series.common.ProductId);
		printf("Manufacturer = %s\n", Manufacturer);
		printf("ManufacturerId = %s\n", ManufacturerId);
		printf("Description = %s\n", Description);
		printf("SerialNumber = %s\n", SerialNumber);
		printf("Cbus3 = %d, which functions as: ", ft_eeprom_x_series.Cbus3);
		switch(ft_eeprom_x_series.Cbus3)
		{
			case  FT_X_SERIES_CBUS_TRISTATE: printf("Tristate"); break;
			case  FT_X_SERIES_CBUS_TXLED: printf("Tx LED"); break;
			case  FT_X_SERIES_CBUS_RXLED: printf("Rx LED"); break;
			case  FT_X_SERIES_CBUS_TXRXLED: printf("Tx and Rx LED"); break;
			case  FT_X_SERIES_CBUS_PWREN: printf("Power Enable"); break;
			case  FT_X_SERIES_CBUS_SLEEP: printf("Sleep"); break;
			case  FT_X_SERIES_CBUS_DRIVE_0: printf("Drive pin to logic 0"); break;
			case  FT_X_SERIES_CBUS_DRIVE_1: printf("Drive pin to logic 1"); break;
			case  FT_X_SERIES_CBUS_IOMODE: printf("IO Mode for CBUS bit-bang"); break;
			case  FT_X_SERIES_CBUS_TXDEN: printf("Tx Data Enable"); break;
			case  FT_X_SERIES_CBUS_CLK24: printf("24MHz clock"); break;
			case  FT_X_SERIES_CBUS_CLK12: printf("12MHz clock"); break;
			case  FT_X_SERIES_CBUS_CLK6: printf("6MHz clock"); break;
			case  FT_X_SERIES_CBUS_BCD_CHARGER: printf("Battery charger detected"); break;
			case  FT_X_SERIES_CBUS_BCD_CHARGER_N: printf("Battery charger detected inverted"); break;
			case  FT_X_SERIES_CBUS_I2C_TXE: printf("I2C Tx empty"); break;
			case  FT_X_SERIES_CBUS_I2C_RXF: printf("I2C Rx full"); break;
			case  FT_X_SERIES_CBUS_VBUS_SENSE: printf("Detect VBUS"); break;
			case  FT_X_SERIES_CBUS_BITBANG_WR: printf("Bit-bang write strobe"); break;
			case  FT_X_SERIES_CBUS_BITBANG_RD: printf("Bit-bang read strobe"); break;
			case  FT_X_SERIES_CBUS_TIMESTAMP: printf("Toggle output when a USB SOF token is received"); break;
			case  FT_X_SERIES_CBUS_KEEP_AWAKE: printf("Keep Awake"); break;	
			default: printf("Unknown function"); break;
		}
		printf("\n");
	}
}

// CBUS3 is used as reset, but must be configured for GPIO first.
void FTDI_Set_CBUS3_Mode (unsigned char pinmode)
{
	FT_STATUS status;
	char Manufacturer[64];
	char ManufacturerId[64];
	char Description[64];
	char SerialNumber[64];
	
	FT_EEPROM_HEADER ft_eeprom_header;
	FT_EEPROM_X_SERIES ft_eeprom_x_series;
	
	ft_eeprom_header.deviceType = FT_DEVICE_X_SERIES; // FTxxxx device type to be accessed
	ft_eeprom_x_series.common = ft_eeprom_header;
	ft_eeprom_x_series.common.deviceType = FT_DEVICE_X_SERIES;
	
	status = FT_EEPROM_Read(handle, &ft_eeprom_x_series, sizeof(ft_eeprom_x_series),
							Manufacturer, ManufacturerId, Description, SerialNumber);
	if (status == FT_OK)
	{
		if (ft_eeprom_x_series.Cbus3!=pinmode)
		{
			ft_eeprom_x_series.Cbus3=pinmode;
			status = FT_EEPROM_Program(handle, &ft_eeprom_x_series, sizeof(ft_eeprom_x_series),
									Manufacturer, ManufacturerId, Description, SerialNumber);
			if (status == FT_OK)
			{
				FT_ResetDevice(handle);
				Sleep(100);
				printf("WARNING: Pin CBUS3 has been configured as '");
				switch(pinmode)
				{	
					case FT_X_SERIES_CBUS_TRISTATE:
						printf("Tristate");
						break;
					case FT_X_SERIES_CBUS_TXLED:
						printf("Tx LED");
						break;
					case FT_X_SERIES_CBUS_RXLED:
						printf("Rx LED");
						break;
					case FT_X_SERIES_CBUS_TXRXLED:
						printf("Tx and Rx LED");
						break;
					case FT_X_SERIES_CBUS_PWREN:
						printf("Power Enable");
						break;
					case FT_X_SERIES_CBUS_SLEEP:
						printf("Sleep");
						break;
					case FT_X_SERIES_CBUS_DRIVE_0:
						printf("Drive pin to logic 0");
						break;
					case FT_X_SERIES_CBUS_DRIVE_1:
						printf("Drive pin to logic 1");
						break;
					case FT_X_SERIES_CBUS_IOMODE:
						printf("IO Mode for CBUS bit-bang");
						break;
					case FT_X_SERIES_CBUS_TXDEN:
						printf("Tx Data Enable");
						break;
					case FT_X_SERIES_CBUS_CLK24:
						printf("24MHz clock");
						break;
					case FT_X_SERIES_CBUS_CLK12:
						printf("12MHz clock");
						break;
					case FT_X_SERIES_CBUS_CLK6:
						printf("6MHz clock");
						break;
					case FT_X_SERIES_CBUS_BCD_CHARGER:
						printf("Battery charger detected");
						break;
					case FT_X_SERIES_CBUS_BCD_CHARGER_N:
						printf("Battery charger detected inverted");
						break;
					case FT_X_SERIES_CBUS_I2C_TXE:
						printf("I2C Tx empty");
						break;
					case FT_X_SERIES_CBUS_I2C_RXF:
						printf("I2C Rx full");
						break;
					case FT_X_SERIES_CBUS_VBUS_SENSE:
						printf("Detect VBUS");
						break;
					case FT_X_SERIES_CBUS_BITBANG_WR:
						printf("Bit-bang write strobe");
						break;
					case FT_X_SERIES_CBUS_BITBANG_RD:
						printf("Bit-bang read strobe");
						break;
					case FT_X_SERIES_CBUS_TIMESTAMP:
						printf("Toggle output when a USB SOF token is received");
						break;
					case FT_X_SERIES_CBUS_KEEP_AWAKE:
						printf("Keep awake");
						break;
					default:
						printf("Something that is probably not correct!");
						break;
				}
				printf("'\n"
				       "Please unplug/plug the BO230XS board for the changes to take effect\n"
				       "and try again.\n");
				fflush(stdout);
			}
		}
	}
}

// PIC32's MCLR is connected to the CBUS3 pin.
void Set_MCLR_to (int val)
{
	FT_SetBitMode(handle, (unsigned char)(val?0x88:0x80), FT_BITMODE_CBUS_BITBANG);
	Sleep(20);
	FT_W32_PurgeComm(handle, PURGE_TXCLEAR|PURGE_RXCLEAR);
}

void Check_DEVCFG0(void)
{
	unsigned int DEVCFG0;
	
	DEVCFG0=Make_Uint(&Boot_Flash_Buffer[BOOT_MAXSIZE-4]);
    printf("DEVCFG0: %08x\n", DEVCFG0);
    printf("   CP=%c\n",     (DEVCFG0&0x10000000)?'1':'0');
    printf("   BWP=%c\n",    (DEVCFG0&0x01000000)?'1':'0');
    printf("   ICESEL=%c\n", (DEVCFG0&0x00000008)?'1':'0');
    printf("   DEBUG<1:0>=%c%c\n", (DEVCFG0&0x00000002)?'1':'0', (DEVCFG0&0x00000001)?'1':'0' );
    exit(0);    
}


void Flash_PIC32 (void)
{
	unsigned int j, k, nbytes, total_bytes, star_count;
	int blank_row;
	
	START;
	
    printf("\nErasing flash memory."); fflush(stdout);
	EraseChip();
	
	EnterProgamMode();
	
    nbytes=0;
    star_count=0;
	for(j=0; j<PROGRAM_MAXSIZE; j+=ROW_SIZE*4)
	{
		blank_row=1;
		// Check if the row to be programmed is all 0xff
		for(k=0; k<ROW_SIZE*4; k++)
		{
			if(Program_Flash_Buffer[j+k]!=0xff)
			{
				blank_row=0;
				break;
			}
		}
		// If there is a byte in the row that is not 0xff, the entire row must be programmed.
		if(blank_row==0)
		{
			if(star_count%50==0)
			{
				star_count=0;
    			printf("\nLoading Program Flash: "); fflush(stdout);
			}
			star_count++;
			nbytes+=ROW_SIZE*4;
    		printf("#"); fflush(stdout);
   			DownloadDataBlock(&Program_Flash_Buffer[j], ROW_SIZE*4);
    		FlashOperation( NVMOP_WRITE_ROW,  PROGRAM_BASE_ADDRESS+j, 0 );
		}
	}
    printf(" Done (%d bytes).", nbytes); fflush(stdout);
    total_bytes=nbytes;
	
    nbytes=0;
    star_count=0;
    for(j=0; j<(BOOT_MAXSIZE) ;j+=ROW_SIZE*4)
	{
		blank_row=1;
		// Check if the row to be programmed is all 0xff
		for(k=0; k<ROW_SIZE*4; k++)
		{
			if(Boot_Flash_Buffer[j+k]!=0xff)
			{
				blank_row=0;
				break;
			}
		}
		// If there is a byte in the row that is not 0xff, the entire row must be programmed.
		if(blank_row==0)
		{
			if(star_count%50==0)
			{
				star_count=0;
			    printf("\nLoading Boot Flash: "); fflush(stdout);
			}
			star_count++;
 			nbytes+=ROW_SIZE*4;
   		    printf("#"); fflush(stdout);
    		DownloadDataBlock(&Boot_Flash_Buffer[j], ROW_SIZE*4);
    		FlashOperation( NVMOP_WRITE_ROW,  BOOT_BASE_ADDRESS+j, 0 );
		}
	}
    printf(" Done (%d bytes).\n", nbytes); fflush(stdout);
	total_bytes+=nbytes;

	ExitProgamMode();
	
    printf("Done %d bytes in ", total_bytes);
    STOP;
	PRINTTIME;
    printf(" (%.1f bytes/sec)\n",  total_bytes/(((double)stopm-startm)/CLOCKS_PER_SEC));
	fflush(stdout);
}

int List_FTDI_Devices (void)
{
	FT_STATUS ftStatus;
	FT_HANDLE ftHandleTemp;
	DWORD numDevs;
	DWORD Flags;
	DWORD ID;
	DWORD Type;
	DWORD LocId;
	char SerialNumber[16];
	char Description[64];
	int j, toreturn=0;
	LONG PortNumber;
	
	if (Selected_Device>=0) return Selected_Device;
	
	// create the device information list
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	if (ftStatus == FT_OK)
	{
		//printf("Number of devices is %d\n",numDevs);
	}
	
	if (numDevs > 1)
	{
		printf("More than one device detected.  Use -d option to select device to use:\n");
		for(j=0; j<numDevs; j++)
		{
			ftStatus = FT_GetDeviceInfoDetail(j, &Flags, &Type, &ID, &LocId, SerialNumber, Description, &ftHandleTemp);
			if (ftStatus == FT_OK)
			{
				printf("-d%d: ", j);
				//printf("Flags=0x%x ",Flags);
				//printf("Type=0x%x ",Type);
				printf("ID=0x%x ",ID);
				//printf("LocId=0x%x ",LocId);
				printf("Serial=%s ",SerialNumber);
				printf("Description=%s ",Description);
				//printf(" ftHandle=0x%x",ftHandleTemp);
				FT_Open(j, &handle);
				FT_GetComPortNumber(handle, &PortNumber);				
				FT_Close(handle);
				printf("Port=COM%d\n", PortNumber); fflush(stdout);
			}
		}
		fflush(stdout);
		exit(-1);
	}
	
	return toreturn;
}

void Print_Help (char * progname)
{
	printf("Run the program like one of these:\n");
	printf("   %s -p somefile.hex  (to program a file)\n", progname);
	printf("   %s -v somefile.hex  (to verify against a file)\n", progname);
	printf("   %s -p -v somefile.hex  (to program and verify)\n", progname);
	printf("   %s -c (shows the configuration of the FTDI chip in the BO230X board)\n", progname);
	printf("   %s -r (restores original function of pin CBUS3 in the BO230X board)\n", progname);
	printf("   %s -dx (use device x, where x is 0, 1, 2, etc.)\n", progname);
	printf("   %s -h (show this help)\n", progname);
	fflush(stdout);
}

int main(int argc, char ** argv)
{
    int j, k;
	int b_program=0, b_verify=0, b_show_ftdi_config=0, b_restore_cbus3=0;
	LONG lComPortNumber;
	char buff[256];

    if(argc<2)
    {
    	Print_Help(argv[0]);
    	return 1;
    }
    
    for(j=1; j<argc; j++)
    {
    	if(argv[j][0]=='-')
    	{
    		     if(argv[j][1]=='v') b_verify=1;
    		else if(argv[j][1]=='p') b_program=1;
    		else if(argv[j][1]=='c') b_show_ftdi_config=1;
    		else if(argv[j][1]=='r') b_restore_cbus3=1;
    		else if(argv[j][1]=='h') { Print_Help(argv[0]); return(0); }
    		else if(argv[j][1]=='d') Selected_Device=atoi(&argv[j][2]);
    		else
    		{
    			printf("WARNING: unknown option '%c'\n", argv[j][1]); fflush(stdout);
    			Print_Help(argv[0]);
    			return(0);
    		}
    		
    	}
    	else
    	{
		    strcpy(HexName, argv[j]);
    	}
    }

    if(FT_Open(List_FTDI_Devices(), &handle) != FT_OK)
    {
        puts("Can not open FTDI adapter.\n");
        return 3;
    }

    if (FT_GetComPortNumber(handle, &lComPortNumber) == FT_OK)
    { 
    	if (lComPortNumber != -1)
    	{
    		printf("Connected to COM%d\n", lComPortNumber); fflush(stdout);
    		sprintf(buff,"echo COM%d>COMPORT.inc", lComPortNumber);
    		system(buff);
    	}
    }

	if(strlen(HexName)>0)
	{
	    if(Read_Hex_File(HexName)<0)
	    {
	    	return 2;
	    }
    }

	if(b_show_ftdi_config)
	{
		FTDI_Configuration();
		return 0;
	}
	
	if(b_restore_cbus3)
	{
		FTDI_Set_CBUS3_Mode(FT_X_SERIES_CBUS_SLEEP);
		return 0;
	}
	else
	{
		FTDI_Set_CBUS3_Mode(FT_X_SERIES_CBUS_IOMODE);
	}
	
	if(strlen(HexName)>0)
	{
		ReadIDCodeRegister();
		if(b_program) Flash_PIC32();
		if(b_verify)  VerifyFlash();
	}
	
	Set_MCLR_to(0); // Reset the microcontroller
	Sleep(10);
	FT_SetBitMode(handle, (unsigned char)(0x00), FT_BITMODE_CBUS_BITBANG); // Set reset pin as input
	FT_SetBitMode(handle, 0x0, FT_BITMODE_RESET); // Back to serial port mode
	FT_Close(handle);
	
	printf("\n");
	
    return 0;
}
