SHELL=cmd
CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B
OBJ = I2C_Nunchuck.o
PORTN=$(shell type COMPORT.inc)

I2C_Nunchuck.elf: $(OBJ)
	$(CC) $(ARCH) -o I2C_Nunchuck.elf I2C_Nunchuck.o -mips16 -DXPRJ_default=default -legacy-libc -Wl,-Map=I2C_Nunchuck.map
	$(OBJCPY) I2C_Nunchuck.elf
	@echo Success!
   
I2C_Nunchuck.o: I2C_Nunchuck.c
	$(CC) -mips16 -g -x c -c $(ARCH) -MMD -o I2C_Nunchuck.o I2C_Nunchuck.c -DXPRJ_default=default -legacy-libc

clean:
	@del *.o *.elf *.hex *.map *.d 2>NUL
	
LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32 -p I2C_Nunchuck.hex
	c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

dummy: I2C_Nunchuck.hex I2C_Nunchuck.map
	$(CC) --version
