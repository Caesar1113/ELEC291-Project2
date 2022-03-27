SHELL=cmd
CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B
OBJS = SerialPort.o
COMPORT = $(shell type COMPORT.inc)
COPT = -g -x c -c $(ARCH) -MMD -DXPRJ_default=default -legacy-libc
LOPT = -DXPRJ_default=default -legacy-libc -Wl,-Map=SerialPort.map

SerialPort.elf: $(OBJS)
	$(CC) $(ARCH) -o SerialPort.elf $(OBJS) $(LOPT)
	$(OBJCPY) SerialPort.elf
	@echo Success!
   
SerialPort.o: SerialPort.c
	$(CC) $(COPT) -o SerialPort.o SerialPort.c 

clean:
	@del $(OBJS) *.elf *.hex *.map *.d 2>NUL

# Load flash, but don't verify.  It takes too long to verify...	
LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32 -p SerialPort.hex

VerifyFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32 -v SerialPort.hex

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	c:\putty\putty.exe -serial $(COMPORT) -sercfg 115200,8,n,1,N -v

dummy: SerialPort.hex SerialPort.map
	@echo Hi!

explorer:
	@explorer .
	
# Compile, link, load, and start putty in one click
Everything:
	@echo ************* Compiling *************
	$(CC) $(COPT) -o SerialPort.o SerialPort.c 
	@echo ************* Linking *************
	$(CC) $(ARCH) -o SerialPort.elf $(OBJS) $(LOPT)
	$(OBJCPY) SerialPort.elf
	@echo ************* Loading *************
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32 -p SerialPort.hex
	@echo ************* Running *************
	c:\putty\putty.exe -serial $(COMPORT) -sercfg 115200,8,n,1,N -v
	@echo done!
