@echo off
::This file was created automatically by CrossIDE to compile with C51.
C:
cd "\CrossIDE\PIC32\PIC32\Servo\"
"C:\Users\weiya\Downloads\CrossIDE (1)\CrossIDE\Call51\Bin\c51.exe" --use-stdout  "C:\CrossIDE\PIC32\PIC32\Servo\Servo.c"
if not exist hex2mif.exe goto done
if exist Servo.ihx hex2mif Servo.ihx
if exist Servo.hex hex2mif Servo.hex
:done
echo done
echo Crosside_Action Set_Hex_File C:\CrossIDE\PIC32\PIC32\Servo\Servo.hex
