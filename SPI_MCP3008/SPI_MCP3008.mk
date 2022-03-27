SHELL=cmd
CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B
OBJ = SPI_MCP3008.o
PORTN=$(shell type COMPORT.inc)

SPI_MCP3008.elf: $(OBJ)
	$(CC) $(ARCH) -o SPI_MCP3008.elf SPI_MCP3008.o -mips16 -DXPRJ_default=default -legacy-libc -Wl,-Map=SPI_MCP3008.map
	$(OBJCPY) SPI_MCP3008.elf
	@echo Success!
   
SPI_MCP3008.o: SPI_MCP3008.c
	$(CC) -mips16 -g -x c -c $(ARCH) -MMD -o SPI_MCP3008.o SPI_MCP3008.c -DXPRJ_default=default -legacy-libc

clean:
	@del *.o *.elf *.hex *.map *.d 2>NUL
	
LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32 -p SPI_MCP3008.hex
	cmd /c start c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	cmd /c start c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

SPI_MCP3008.jpg:
	cmd /c start pictures\SPI_MCP3008.jpg

dummy: SPI_MCP3008.hex SPI_MCP3008.map
	$(CC) --version
