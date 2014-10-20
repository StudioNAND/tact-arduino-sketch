/**
 * Tact example for handling capacitive sensing.
 * Copyright (C) 2013, Jack Rusher, Tomek Ness and Studio NAND
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Arduino sketch https://github.com/StudioNAND/tact-arduino-sketch
 * Processing lib https://github.com/StudioNAND/tact-processing
 *
 * This example is inspired by Mads Hobye's instructables tutorial - thanks!
 * http://www.instructables.com/id/Touche-for-Arduino-Advanced-touch-sensing/
 */

# define VERSION 1

// Serial, symbols per second
#define BAUD_RATE 115200

// Bit set/clear macros
#define SET(x,y) (x |=(1<<y))
#define CLR(x,y) (x &= (~(1<<y)))
#define CHK(x,y) (x & (1<<y))
#define TOG(x,y) (x^=(1<<y))

#define CMD_BUFFER_INDEX_PIN 0
#define CMD_BUFFER_INDEX_START 1
#define CMD_BUFFER_INDEX_COUNT 2
#define CMD_BUFFER_INDEX_STEP 3

#define CMD_SEPARATOR ' '

#define STATE_IDLE 0
#define STATE_RECEIVE_CMD 1
#define STATE_TRANSMIT_SPECTRUM 2
#define STATE_TRANSMIT_PEAK 3
#define STATE_TRANSMIT_BIAS 4
#define STATE_TRANSMIT_BIAS_PEAK 5

// Multiplexer 4051' pins
#define MP_4051_S0 12
#define MP_4051_S1 11
#define MP_4051_S2 10

// Application state
int state = STATE_IDLE;

char cmdKey;
int cmdBuffer[4] = {
  0,0,0,0};
int cmdIndex;

float bias;
float peak;

void setup () {
  // Start up serial communication
  Serial.begin (BAUD_RATE);

  // Set up frequency generator
  TCCR1A = 0b10000010;
  TCCR1B = 0b00011001;

  // Signal generator pins
  pinMode (7, INPUT);
  pinMode (8, OUTPUT);
  // Sync (test) pin
  pinMode (9, OUTPUT);

  pinMode (MP_4051_S0, OUTPUT);  // s0
  pinMode (MP_4051_S1, OUTPUT);  // s1
  pinMode (MP_4051_S2, OUTPUT);  // s2
}


void loop() {

  // While there is anything in the 
  // pipe that has not been processed..
  while (Serial.available() > 0) {
    // Read incoming serial data
    serialEvent (Serial.read());
  }

  // If signal data has been requested 
  // by the client (i.e. Processing) ...
  if (state == STATE_TRANSMIT_SPECTRUM || state == STATE_TRANSMIT_PEAK || state == STATE_TRANSMIT_BIAS || state == STATE_TRANSMIT_BIAS_PEAK ) {
    // Declare sensor value buffer 
    int results[cmdBuffer[CMD_BUFFER_INDEX_COUNT]];
    // Declare peak and bias vars
    int peak = 0;
    int bias = 0;

    // Select the sensor-input / bit
    // (use this with arduino 0013+)
    digitalWrite (MP_4051_S0, bitRead (cmdBuffer[CMD_BUFFER_INDEX_PIN], 0));
    digitalWrite (MP_4051_S1, bitRead (cmdBuffer[CMD_BUFFER_INDEX_PIN], 1));
    digitalWrite (MP_4051_S2, bitRead (cmdBuffer[CMD_BUFFER_INDEX_PIN], 2));

    for (unsigned int d = 0; d < cmdBuffer[CMD_BUFFER_INDEX_COUNT]; d++) {
      // Reload new frequency
      TCNT1 = 0;
      ICR1 = cmdBuffer[CMD_BUFFER_INDEX_START] + cmdBuffer[CMD_BUFFER_INDEX_STEP] * d;
      OCR1A = ICR1 / 2;

      // Restart generator
      SET (TCCR1B, 0);
      // Read response signal
      results[d] = (float) analogRead(0);
      // Stop generator
      CLR (TCCR1B, 0);

      // Check if current result is higher than previously stored peak
      // if true, overwrite peak and bias
      if( results[d] > peak ) {
        peak = results[d];
        bias = d;
      }
    }


    // Announce which of the Tact-inputs that 
    // are multiplexed will be transmitted
    sendInt (1024 + cmdBuffer[CMD_BUFFER_INDEX_PIN]);

    // Send data depending on what is requested
    switch (state) {
      // send spectrum
    case STATE_TRANSMIT_SPECTRUM:
      // send data_type for data to be transmitted
      sendInt( 1088 + 0 );
      // Tell client how many data values are going to be sent
      sendInt (1098 + cmdBuffer[CMD_BUFFER_INDEX_COUNT]);

      // Go! Send signal spectrum ...
      for (int x=0; x < cmdBuffer[CMD_BUFFER_INDEX_COUNT]; x++) {
        sendInt (results[x]);
      }
      break;

    case STATE_TRANSMIT_PEAK:
      // send data_type for data to be transmitted
      sendInt( 1088 + 1 );
      // Tell client how many data values are going to be sent
      sendInt (1098 + 1);
      // send peak
      sendInt(peak);
      break;

    case STATE_TRANSMIT_BIAS:
      // send data_type for data to be transmitted
      sendInt( 1088 + 2 );
      // Tell client how many data values are going to be sent
      sendInt (1098 + 1);
      // send bias
      sendInt(bias);
      break;

    case STATE_TRANSMIT_BIAS_PEAK:
      // send data_type for data to be transmitted
      sendInt( 1088 + 3 );
      // Tell client how many data values are going to be sent
      sendInt (1098 + 2);
      // send bias
      sendInt(bias);
      sendInt(peak);
      break;

    default:
      ; // do nothing
    }

    // Confirm that signal spectrum 
    // has been delivered, done!
    sendInt (2123);

    // Toggle pin 9 after each 
    // sweep (good for scope)
    TOG (PORTB, 0);

    state = STATE_IDLE;
  }

}

/**
 * Function to execute current set command, called 
 * when new-line byte (10) has been received. 
 */
void execute () {
  switch (cmdKey) {
  case 'S':
    state = STATE_TRANSMIT_SPECTRUM;
    break;

  case 'P':
    state = STATE_TRANSMIT_PEAK;
    break;

  case 'B':
    state = STATE_TRANSMIT_BIAS;
    break;

  case 'X':
    state = STATE_TRANSMIT_BIAS_PEAK;
    break;

  case 'V':
    sendInt (2124 + VERSION);
    break;
  }
}

