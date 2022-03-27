SHELL=cmd
PORTN=$(shell type COMPORT.inc)

pro32.exe: pro32.c
	@docl pro32.c
	
clean:
	del *.obj *.exe

dummy: docl.bat
	@echo hello from dummy!
	
flash_verify_putty: serial.hex
	@Taskkill /IM putty.exe /F | wait 500
	pro32 -p -v serial.hex
	c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

flash_verify: serial.hex
	@Taskkill /IM putty.exe /F | wait 500
	pro32 -p -v serial.hex

flash:
	@Taskkill /IM putty.exe /F | wait 500
	pro32 -p serial.hex

verify:
	@Taskkill /IM putty.exe /F | wait 500
	pro32 -v serial.hex

putty: serial.hex
	@Taskkill /IM putty.exe /F | wait 500
	c:\putty\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v

flash_verify2:
	@Taskkill /IM putty.exe /F | wait 500
	pro32 -p -v ..\xc32_stdio\serial.hex

flash2:
	@Taskkill /IM putty.exe /F | wait 500
	pro32 -p ..\xc32_stdio\serial.hex

verify2:
	@Taskkill /IM putty.exe /F | wait 500
	pro32 -v ..\xc32_stdio\serial.hex

FTDI_config:
	pro32 -c
	
CBUS3_SLEEP:
	pro32 -r	