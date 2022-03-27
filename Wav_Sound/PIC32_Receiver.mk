SHELL=cmd

CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B
OBJ = PIC32_Receiver.o
PORTN=$(shell type COMPORT.inc)

PIC32_Receiver.elf: $(OBJ)
	$(CC) $(ARCH) -o PIC32_Receiver.elf PIC32_Receiver.o -mips16 -DXPRJ_default=default \
		-legacy-libc -Wl,-Map=PIC32_Receiver.map
	$(OBJCPY) PIC32_Receiver.elf
	@echo Success!
	
PIC32_Receiver.o: PIC32_Receiver.c
	$(CC) -g -x c -mips16 -Os -c $(ARCH) -MMD -o PIC32_Receiver.o PIC32_Receiver.c \
		-DXPRJ_default=default -legacy-libc

clean:
	@del *.o *.elf *.hex *.d *.map 2>NUL

LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32 -p PIC32_Receiver.hex

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	cmd /c start c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

dummy: PIC32_Receiver.hex PIC32_Receiver.map

explorer:
	explorer .
