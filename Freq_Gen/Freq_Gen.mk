SHELL=cmd
CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B

OBJ = Freq_Gen.o
COMPORT = $(shell type COMPORT.inc)

Freq_Gen.elf: $(OBJ)
	$(CC) $(ARCH) -o Freq_Gen.elf Freq_Gen.o -mips16 -DXPRJ_default=default -legacy-libc -Wl,-Map=Freq_Gen.map
	$(OBJCPY) Freq_Gen.elf
	@echo Success!
   
Freq_Gen.o: Freq_Gen.c
	$(CC) -g -x c -mips16 -Os -c $(ARCH) -MMD -o Freq_Gen.o Freq_Gen.c -DXPRJ_default=default -legacy-libc

clean:
	@del *.o *.d *.map *.elf *.hex 2>NUL

FlashLoad:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32 -p -v Freq_Gen.hex

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	putty.exe -serial $(COMPORT) -sercfg 115200,8,n,1,N -v

dummy: Freq_Gen.hex Freq_Gen.map
	@echo ;-)
	
explorer:
	explorer .
