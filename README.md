NMEA200 data sender based heavily on the Messagesender from Timo Lappalainen at https://github.com/ttlappalainen/NMEA2000/tree/master/Examples/MessageSender.

This version sned both NMEA2000 on a conencted CAN bus and as YachtData format over UDP.

Uses a lot of stuff from my other N2K projects as well.

https://www.tech-sparks.com/using-esp32-with-arduino-ide-to-control-an-lcd-display/

http://www.lcdwiki.com/3.5inch_RPi_Display

https://lastminuteengineers.com/esp32-pinout-reference/

Key Parameters
SKU 	MPI3501
LCD Type 	TFT
LCD Interface 	SPI(Fmax:32MHz)
Touch Screen Type 	Resistive
Touch Screen Controller 	XPT2046
Colors 	65536
Driver IC 	ILI9486
Backlight 	LED
Resolution 	320*480 (Pixel)
Backlight Current 	120ma
Power Dissipation 	0.13A*5V
Operating Temp. (℃) 	-20~60
Active Area 	48.96x73.44(mm)
Product Size 	85.42*55.60(mm)
Package Size 	118*72*34 (mm)
Rough Weight(Package containing) 	75 (g)
Interface
PIN NO. 	SYMBOL 	DESCRIPTION
1, 17 	3.3V 	Power positive (3.3V power input)
2, 4 	5V 	Power positive (5V power input)
3, 5, 7, 8, 10, 12, 13, 15, 16 	NC 	NC
6, 9, 14, 20, 25 	GND 	Ground
11 	TP_IRQ 	Touch Panel interrupt, low level while the Touch Panel detects touching
18 	LCD_RS 	Instruction/Data Register selection
19 	LCD_SI / TP_SI 	SPI data input of LCD/Touch Panel
21 	TP_SO 	SPI data output of Touch Panel
22 	RST 	Reset
23 	LCD_SCK / TP_SCK 	SPI clock of LCD/Touch Panel
24 	LCD_CS 	LCD chip selection, low active
26 	TP_CS 	Touch Panel chip selection, low active 
