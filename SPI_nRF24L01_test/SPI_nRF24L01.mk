SHELL=cmd
CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B
OBJ = SPI_nRF24L01.o nrf24.o radioPinFunctions.o
PORTN=$(shell type COMPORT.inc)

SPI_nRF24L01.elf: $(OBJ)
	$(CC) $(ARCH) -o SPI_nRF24L01.elf $(OBJ) -mips16 -DXPRJ_default=default -legacy-libc -Wl,-Map=SPI_nRF24L01.map
	$(OBJCPY) SPI_nRF24L01.elf
	@echo Success!
   
SPI_nRF24L01.o: SPI_nRF24L01.c
	$(CC) -mips16 -g -x c -c $(ARCH) -MMD -o SPI_nRF24L01.o SPI_nRF24L01.c -DXPRJ_default=default -legacy-libc

nrf24.o: nrf24.c
	$(CC) -mips16 -g -x c -c $(ARCH) -MMD -o nrf24.o nrf24.c -DXPRJ_default=default -legacy-libc

radioPinFunctions.o: radioPinFunctions.c
	$(CC) -mips16 -g -x c -c $(ARCH) -MMD -o radioPinFunctions.o radioPinFunctions.c -DXPRJ_default=default -legacy-libc

clean:
	@del *.o *.elf *.hex *.map *.d 2>NUL
	
LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	pro32 -p SPI_nRF24L01.hex
	cmd /c start c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	cmd /c start c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

SPI_nRF24L01_a.jpg:
	cmd /c start pictures\SPI_nRF24L01_a.jpg

SPI_nRF24L01_b.jpg:
	cmd /c start pictures\SPI_nRF24L01_b.jpg

dummy: SPI_nRF24L01.hex SPI_nRF24L01.map
	$(CC) --version
