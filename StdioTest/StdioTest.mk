SHELL=cmd
CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B
OBJ = StdioTest.o
PORTN=$(shell type COMPORT.inc)

StdioTest.elf: $(OBJ)
	$(CC) $(ARCH) -o StdioTest.elf StdioTest.o -mips16 -DXPRJ_default=default -legacy-libc -Wl,-Map=StdioTest.map
	$(OBJCPY) StdioTest.elf
	@echo Success!
   
StdioTest.o: StdioTest.c
	$(CC) -mips16 -g -x c -c $(ARCH) -MMD -o StdioTest.o StdioTest.c -DXPRJ_default=default -legacy-libc

clean:
	@del *.o *.elf *.hex *.map *.d 2>NUL
	
LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32 -p -v StdioTest.hex

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

dummy: StdioTest.hex StdioTest.map
	$(CC) --version
