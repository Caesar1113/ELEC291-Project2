SHELL=cmd
CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B
OBJ = Freq_Gen2.o
PORTN=$(shell type COMPORT.inc)

Freq_Gen2.elf: $(OBJ)
	$(CC) $(ARCH) -o Freq_Gen2.elf Freq_Gen2.o -mips16 -DXPRJ_default=default -legacy-libc -Wl,-Map=Freq_Gen2.map
	$(OBJCPY) Freq_Gen2.elf
	@echo Success!
   
Freq_Gen2.o: Freq_Gen2.c
	$(CC) -mips16 -g -x c -c $(ARCH) -MMD -o Freq_Gen2.o Freq_Gen2.c -DXPRJ_default=default -legacy-libc

clean:
	@del *.o *.elf *.hex *.map *.d 2>NUL
	
LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32 -p -v Freq_Gen2.hex

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

dummy: Freq_Gen2.hex Freq_Gen2.map
	$(CC) --version
