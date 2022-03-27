SHELL=cmd
CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B
OBJ = Robot_Base.o
PORTN=$(shell type COMPORT.inc)

Robot_Base.elf: $(OBJ)
	$(CC) $(ARCH) -o Robot_Base.elf Robot_Base.o -mips16 -DXPRJ_default=default -legacy-libc -Wl,-Map=Robot_Base.map
	$(OBJCPY) Robot_Base.elf
	@echo Success!
   
Robot_Base.o: Robot_Base.c
	$(CC) -mips16 -g -x c -c $(ARCH) -MMD -o Robot_Base.o Robot_Base.c -DXPRJ_default=default -legacy-libc

clean:
	@del *.o *.elf *.hex *.d *.map 2>NUL
	
LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32.exe -p Robot_Base.hex
	putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N

dummy: Robot_Base.hex Robot_Base.map
	$(CC) --version

explorer:
	@explorer .