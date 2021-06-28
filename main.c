// define CPU speed - actual speed is set using CLKPSR in main()
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "dim_curve.h"

#define R PA0
#define G PA1
#define B PA2
#define W PA3
#define HUE PA4
#define SAT PA5
#define VAL PA6

volatile uint8_t r = 0;
volatile uint8_t g = 0;
volatile uint8_t b = 0;
volatile uint8_t w = 0;
volatile uint8_t state = 0;
volatile uint16_t rotate = 0;

void getRGB(uint16_t hue, uint8_t sat, uint8_t val, volatile uint8_t *r, volatile uint8_t *g, volatile uint8_t *b, volatile uint8_t *w) {
  // Most of this taken from https://www.vagrearg.org/content/hsvrgb
  if( val == 0 ) {
    *r = 0;
    *g = 0;
    *b = 0;
    *w = 0;
  }
  if( sat == 0 ) {
    *r = val;
    *g = val;
    *b = val;
    *w = val;
  }

  uint8_t top, ramping, bottom;

	uint8_t sextant = hue >> 8;

	top = pgm_read_byte(&dim_curve[val]);		// Top level

	// Perform actual calculations
	uint8_t bb;
	uint16_t ww;

	/*
	 * Bottom level: v * (1.0 - s)
	 * --> (v * (255 - s) + error_corr) / 256
	 */
	bb = ~sat;
	ww = val * bb;
	ww += 1;		// Error correction
	ww += ww >> 8;		// Error correction
	bottom = pgm_read_byte(&dim_curve[ww >> 8]);

	uint8_t h_fraction = hue & 0xff;	// 0...255

	if(!(sextant & 1)) {
		// *r = ...slope_up...;
		/*
		 * Slope up: v * (1.0 - s * (1.0 - h))
		 * --> (v * (255 - (s * (256 - h) + error_corr1) / 256) + error_corr2) / 256
		 */
		ww = !h_fraction ? ((uint16_t)sat << 8) : (sat * (uint8_t)(-h_fraction));
		ww += ww >> 8;		// Error correction 1
		bb = ww >> 8;
		bb = ~bb;
		ww = val * bb;
		ww += val >> 1;		// Error correction 2
		ramping = pgm_read_byte(&dim_curve[ww >> 8]);
	} else {
		// *r = ...slope_down...;
		/*
		 * Slope down: v * (1.0 - s * h)
		 * --> (v * (255 - (s * h + error_corr1) / 256) + error_corr2) / 256
		 */
		ww = sat * h_fraction;
		ww += ww >> 8;		// Error correction 1
		bb = ww >> 8;
		bb = ~bb;
		ww = val * bb;
		ww += val >> 1;		// Error correction 2
		ramping = pgm_read_byte(&dim_curve[ww >> 8]);
	}

  switch(sextant) {
    case 0:
      *r = top;
      *g = ramping;
      *b = bottom;
      *w = bottom;
      break;
    case 1:
      *r = ramping;
      *g = top;
      *b = bottom;
      *w = bottom;
      break;
    case 2:
      *r = bottom;
      *g = top;
      *b = ramping;
      *w = bottom;
      break;
    case 3:
      *r = bottom;
      *g = ramping;
      *b = top;
      *w = bottom;
      break;
    case 4:
      *r = ramping;
      *g = bottom;
      *b = top;
      *w = bottom;
      break;
    case 5:
      *r = top;
      *g = bottom;
      *b = ramping;
      *w = bottom;
      break;
  }
}

ISR(TIMER0_COMPA_vect) {
  state++;
  PORTA = (r>state? 1<<R: 0) | (g>state? 1<<G: 0) | (b>state? 1<<B: 0) | (w>state? 1<<W: 0);
}

int main() {
  cli();
  // Allow changes to protected registers
  CLKPR = (1<<CLKPCE);
  // Set clock prescaler to 0, using full 8MHz internal oscillator
  CLKPR = 0;

  // Set R, G, B, and W channels as outputs
  DDRA = (1<<R) | (1<<G) | (1<<B) | (1<<W);

  // Set timer count register to clear on match (CTC mode)
  TCCR0A = (1<<CTC0);
  // Set the timer to enable the interrupt on clearing the count register
  TIMSK = (1<<OCIE0A);
  // Enable timer 0 and set the timer prescaler to be clk/8, which on 8MHz, is 1MHz
  TCCR0B = (1<<CS00);
  // Set the timer to clear on 10, or approximately 100kHz
  OCR0A = 20;

	sei();

  // main loop
  while (1) {
    rotate = (rotate + 5) % (6<<8);
    getRGB(rotate, 200, 255, &r, &g, &b, &w);
    _delay_ms(5);
  }

  return 0;
}
