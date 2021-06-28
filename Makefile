FLAGS = -mmcu=attiny461 -DF_CPU=8000000UL -Os -std=c99

CC = avr-gcc
SIZE = avr-size

all: main.hex

%.hex: %.elf
	avr-objcopy -O ihex $< $@

clean:
	rm -f *.elf *.hex

%.elf: %.c
	$(CC) $(FLAGS) $^ -o $@
	$(SIZE) -C --mcu=attiny461 $@
