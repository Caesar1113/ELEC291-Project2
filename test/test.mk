SHELL=cmd
CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B
OBJ = test.o
PORTN=$(shell type COMPORT.inc)

test.elf: $(OBJ)
	$(CC) $(ARCH) -o test.elf test.o -mips16 -DXPRJ_default=default -legacy-libc -Wl,-Map=test.map
	$(OBJCPY) test.elf
	@echo Success!
   
test.o: test.c
	$(CC) -mips16 -g -x c -c $(ARCH) -MMD -o test.o test.c -DXPRJ_default=default -legacy-libc

clean:
	@del *.o *.elf *.hex *.d *.map 2>NUL
	
LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32.exe -p test.hex
	putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N

dummy: test.hex test.map
	$(CC) --version

explorer:
	@explorer .