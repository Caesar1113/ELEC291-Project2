SHELL=cmd
CC = xc32-gcc
OBJCPY = xc32-bin2hex
ARCH = -mprocessor=32MX130F064B

OBJ = TimerIRQ.o

TimerIRQ.elf: $(OBJ)
	$(CC) $(ARCH) -o TimerIRQ.elf TimerIRQ.o -mips16 -DXPRJ_default=default -legacy-libc -Wl,-Map=TimerIRQ.map
	$(OBJCPY) TimerIRQ.elf
	@echo Success!
   
TimerIRQ.o: TimerIRQ.c
	$(CC) -g -x c -mips16 -Os -c $(ARCH) -MMD -o TimerIRQ.o TimerIRQ.c -DXPRJ_default=default -legacy-libc

clean:
	@del *.o *.d *.map *.elf *.hex 2>NUL

FlashLoad:
	pro32 -p -v TimerIRQ.hex

dummy: TimerIRQ.hex TimerIRQ.map
	@echo ;-)
	
explorer:
	explorer .
