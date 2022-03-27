SHELL=cmd
CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B
OBJ = HelloWorld.o
PORTN=$(shell type COMPORT.inc)

HelloWorld.elf: $(OBJ)
	$(CC) $(ARCH) -o HelloWorld.elf HelloWorld.o -mips16 -DXPRJ_default=default -legacy-libc -Wl,-Map=HelloWorld.map
	$(OBJCPY) HelloWorld.elf
	@echo Success!
   
HelloWorld.o: HelloWorld.c
	$(CC) -mips16 -g -x c -c $(ARCH) -MMD -o HelloWorld.o HelloWorld.c -DXPRJ_default=default -legacy-libc

clean:
	@del *.o *.elf *.hex *.d *.map 2>NUL
	
LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32.exe -p -v HelloWorld.hex

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

dummy: HelloWorld.hex HelloWorld.map
	$(CC) --version
	
explorer:
	@explorer .
