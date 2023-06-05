// "LightsOut" Game
//
// Demonstates a single PCF8574 port exander I2C chip with 8 buttons and 8 LEDs
// Reads button values on the same output pins that are driven low to turn LEDs on.

// Careful below.  Only include ONE I2C wire library.

// Wire.h for regular Arduino I2C library. Use this for your atmega, or Uno.
// #include <Wire.h>
//
// TinyWireM.h for Digispark or other attiny chips without "real" hardware I2C.
#include <TinyWireM.h>
// There are lots of different "tiny" versions of i2c.
// LightsOut uses https://github.com/adafruit/TinyWireM becuase its functions are equivalent to Wire.h
// By default (quote): "pin [PB0] is SDA (I2C data), pin [PB2] is SCK (I2C clock)"

// A little difference for TinyWireM:
#ifdef TinyWireM_h
#define Wire TinyWireM
#endif

// This pin is not involved with USB or I2C
#define buzzerPin PB1
// Had to use it for something -- so put a buzzer in.

// From here: http://arduino.cc/en/Tutorial/Tone
#include "pitches.h"

//int scale[] = {NOTE_C5,NOTE_D5,NOTE_E5,NOTE_F5,NOTE_G5,NOTE_A5,NOTE_B5,NOTE_C6};
int scale[] = {NOTE_C4,NOTE_D4,NOTE_E4,NOTE_F4,NOTE_G4,NOTE_A4,NOTE_B4,NOTE_C5};
//int scale[] = {NOTE_C3,NOTE_D3,NOTE_E3,NOTE_F3,NOTE_G3,NOTE_A3,NOTE_B3,NOTE_C4};

// Your base address may vary.  Mine is a PCF8574P chip on 0x20.
#define PCF8574_BASE_ADDR 0x20
// Odd numbered addresses did not work well for me.  Always tie A0 low.



void setup()
{
   Wire.begin();

} // setup()



// Main tick delay ... the speed of game. About 0.65s
#define initial_rotation_delay_us 650000L

// The faster it goes, the faster it goes.
#define initial_tick_decay        1300L

// Wayyy too fast.
#define hopeless_quick_delay_us   10000L

void loop()
{
   static uint32_t rotation_delay_us = initial_rotation_delay_us;
   static uint32_t decay=initial_tick_decay;
   static int game=0, result, losses;
   uint8_t s1, s2;

   if (game == 0)      // everyone is a winner
   {
      result=game0(rotation_delay_us);
   }
   else if (game == 1) // simplest game, counter-rotating chasers
   {
      result=game1(0xBF, 0xEF, rotation_delay_us, decay);
   }
   else if (game == 2) // almost as simple, two chasers, same direction
   {
      result=game1(0xFF, 0xAF, rotation_delay_us, decay);
   }
   else if (game >= 3) // random start data in each chaser
   {
      game_seed(&s1,&s2);
      result=game1(s1, s2, rotation_delay_us, decay);
   }

   if ( result==0 ) // a WIN
   {
      game++;

   // increase the speed
      rotation_delay_us -= rotation_delay_us/10;
   }
   else
   {
      losses++;
   }

   if (( game > 5 ) || ( losses > 1 )) // reset
   {
      // -- could play a show tune here

      game=0;
      losses=0;
      rotation_delay_us = initial_rotation_delay_us;

      // -- perhaps increase the decay?
   }

} // loop()





// Turn all the lights out
// Train player to flip the state of a light, by pressing a button.
// Only progress to real game after doffing all the lights.
//
int game0(uint32_t rotation_delay_us)
{
   uint8_t output_byte=0xFF;
   uint8_t currRead, lastRead, newRead;

   // On startup, the chip register will be all 1's anyway.
   writeI2C(PCF8574_BASE_ADDR, output_byte);
   // All lights off.

   delay(250);
    
   // Light up a bits, LSB to MSB, play ascending scale
   for (int i=0; i<8; i++)
   {
      output_byte &= ~_BV(i);
    
      writeI2C(PCF8574_BASE_ADDR, output_byte);
      tone(buzzerPin, scale[i]);
      delay(rotation_delay_us/4000);
      noTone(buzzerPin);

   } // for bits in byte


   do // flip bits where button is pressed, and play corresponding tone
   {
      currRead = ~read_all_PCF8574_bits(PCF8574_BASE_ADDR);

      // New buttons ... old ones may still be depressed
      newRead = ( currRead & ~lastRead );

      if ( currRead && (currRead != lastRead) )
      {
         output_byte = output_byte ^ newRead;

         tone(buzzerPin, scale[ low_bit_pos(newRead) ]);
      }

      if ( currRead == 0 )
         noTone(buzzerPin);
      
      lastRead = currRead;

      writeI2C(PCF8574_BASE_ADDR, output_byte);

      delay(2); // Spend a little time on the main output display.
 
   } while( (output_byte != 0xFF) || currRead  );
   // Continue when lights are on or buttons pressed
   
   noTone(buzzerPin);
   delay(1000);

   return(0); // Everyone is a winner

} // game0()




// Turn off the counter-rotating chasers by inverting bits, when they converge.
//
int game1(uint8_t output_byte_Clockwise, uint8_t output_byte_CntrClckw, uint32_t rotation_delay_us, uint32_t decay_rate )
{

   uint8_t currRead, lastRead, newRead;
   uint32_t last_rotation_time_us = micros();

  do
  {
  
  // With raw read 0xFF means no buttons pressed.
     currRead = ~read_all_PCF8574_bits(PCF8574_BASE_ADDR);
  // Buttons are read, driven to low voltage.
  // Invert currRead, so that 1's are where buttons ARE pressed.

  // New buttons ... old ones may still be depressed
     newRead = ( currRead & ~lastRead );
  
  
  // if buttons are pressed, and they are different
     if ( currRead && (currRead != lastRead) )
     {
  
     // Invert/flip (XOR) bits where a different button (from lastRead) is depressed.
        output_byte_Clockwise = output_byte_Clockwise ^ newRead;
        output_byte_CntrClckw = output_byte_CntrClckw ^ newRead;

        tone(buzzerPin, scale[ low_bit_pos(newRead) ]);
  
     }
     lastRead = currRead;
  
     if ( currRead == 0 )
        noTone(buzzerPin);
  
  // The read_all_PCF8574_bits() call above wrote 1's, so the display has no LEDs on
     writeI2C(PCF8574_BASE_ADDR, output_byte_Clockwise & output_byte_CntrClckw);
  // Immediately restore output LEDs, perhpas with new values.  This is the main write statement.
  
  
  // Rotate bits after rotation_delay microseconds have passed
     if ( (uint32_t)(micros() - last_rotation_time_us) >= rotation_delay_us ) 
     {
        uint8_t endBit;
  
     // Clockwise, save LSB value ... 1 or bit 0 or 0b00000001
        endBit = output_byte_Clockwise & _BV(0);
        output_byte_Clockwise >>= 1;
        if (endBit)
           output_byte_Clockwise |= _BV(7);
  
     // CounterClockwise, save MSB value ... 128 or bit 7 or 0b10000000
        endBit = output_byte_CntrClckw & _BV(7);
        output_byte_CntrClckw <<= 1;
        if (endBit)
           output_byte_CntrClckw |= _BV(0);
  
        if ( currRead == 0 )
           tone(buzzerPin, 8000,10);

        // quicken the turn
        rotation_delay_us -= decay_rate;
        
        last_rotation_time_us = micros();
        
     } // if rotation_delay met
  
     if ( output_byte_Clockwise==255 &&
          output_byte_CntrClckw==255 )
     {
        noTone(buzzerPin);
        delay(250);
        tone(buzzerPin, NOTE_B5);
        delay(250);
        tone(buzzerPin, NOTE_C6);
        delay(1000);
        noTone(buzzerPin);
        delay(1000);
        return(0);
  
     } // if You Won the game

     if ( rotation_delay_us < hopeless_quick_delay_us )
     {
        noTone(buzzerPin);
        delay(250);
        tone(buzzerPin, NOTE_AS3);
        delay(1500);
        noTone(buzzerPin);
        delay(1000);
        return(1);

     } // if You Lost
  
     delay(1); // Spend a little time on the main output display.
  
  } while (1);

} // game1()




uint8_t writeI2C(uint8_t addr, uint8_t val)
{
   Wire.beginTransmission(addr);
   
// write() is send() on Rambo's version of TinyWireM
   Wire.write( val );
   
   return (Wire.endTransmission());

} // writeI2C()



// Read all buttons; request 1 byte.
//
// If you dare read a uint8_t with beginTransmission()/endTransmission(),
// just after the MSB is set, Rambo's version of TinyWireM will crash.
// Best not to use begin/end functions for reading.
//
uint8_t read_all_PCF8574_bits(uint8_t addr )
{
// Write 1's to enable all buttons for reading. Output is high, with high resistance.
   writeI2C(PCF8574_BASE_ADDR, 0xFF);

   Wire.requestFrom(PCF8574_BASE_ADDR,1);
   
// It is receive() instead of read() on Rambo's version of TinyWireM (not this one).
   return(Wire.read());

} // read_all_PCF8574_bits()



// Using this to find position of bit set, for a button pressed
// C++20 has (std::bit_width(index) - 1) for high bit position
// Perhaps there is an integer version of log2(n)?
// This really should throw exception for 0, or pehaps return -1, 8 or other indicator.
//
uint8_t low_bit_pos(uint8_t b)
{
   uint8_t bitpos = 0;

   while (b >>= 1) bitpos++;

   return(bitpos);

} // position of lowest bit that is set




// Was a mind-bending game in itself, to devise a game that is always winable.
// Fair games need an even number even bits, and an even number of odd bits set.
// Otherwise, it is impossible to turn all the lights out.
//
void game_seed(uint8_t *b1, uint8_t *b2)
{
   uint16_t seed=micros();
   uint16_t evenBits=0, oddBits=0;

// Count the even and odd bits
   for ( int i=0; i<16; i++ )
   {
      if ( seed & _BV(i)) evenBits++;
      i++;
      if ( seed & _BV(i)) oddBits++;
   }

// If odd num of bits, flip one
   if ( evenBits % 2 ) seed ^= 1;
   if ( oddBits  % 2 ) seed ^= 2;

// Split the 16 bit seed into two bytes
   *b1=seed;
   *b2=seed>>8;

} // game_seed()
