SHELL=cmd
CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B
OBJ = ADCtest.o
PORTN=$(shell type COMPORT.inc)

ADCtest.elf: $(OBJ)
	$(CC) $(ARCH) -o ADCtest.elf ADCtest.o -mips16 -DXPRJ_default=default -legacy-libc -Wl,-Map=ADCtest.map
	$(OBJCPY) ADCtest.elf
	@echo Success!
   
ADCtest.o: ADCtest.c
	$(CC) -mips16 -g -x c -c $(ARCH) -MMD -o ADCtest.o ADCtest.c -DXPRJ_default=default -legacy-libc

clean:
	@del *.o *.elf *.hex *.d *.map 2>NUL
	
LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32.exe -p -v ADCtest.hex

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

dummy: ADCtest.hex ADCtest.map
	$(CC) --version

explorer:
	@explorer .