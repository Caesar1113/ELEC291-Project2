SHELL=cmd

CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B
OBJ = Two_Timers.o
PORTN=$(shell type COMPORT.inc)

c.elf: $(OBJ)
	$(CC) $(ARCH) -o Two_Timers.elf Two_Timers.o -mips16 -DXPRJ_default=default \
		-legacy-libc -Wl,-Map=Two_Timers.map
	$(OBJCPY) Two_Timers.elf
	@echo Success!
	
Two_Timers.o: Two_Timers.c
	$(CC) -g -x c -mips16 -Os -c $(ARCH) -MMD -o Two_Timers.o Two_Timers.c \
		-DXPRJ_default=default -legacy-libc

clean:
	@del *.o *.elf *.hex *.d *.map 2>NUL

LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32 -p Two_Timers.hex
	cmd /c start c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	cmd /c start c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

dummy: Two_Timers.hex Two_Timers.map

explorer:
	explorer .
