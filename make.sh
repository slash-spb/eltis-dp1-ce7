avr-gcc -mmcu=atmega48pa -Os -c main.c
avr-gcc -mmcu=atmega48pa -o main.elf main.o
avr-objcopy -j .text -j .data -O ihex main.elf main.hex
avrdude -c usbasp -p m48p -U flash:w:main.elf

#avr-gcc -mmcu=atmega48pa -Os main.c -o main.o
#avr-objcopy -j .text -j .data -O ihex main.o main.hex
#avrdude -c usbasp -p m48p -U flash:w:main.hex
