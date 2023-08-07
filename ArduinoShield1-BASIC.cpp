// This is written for the Arduino UNO
// To run this, you can either use the Arduino IDE AppImage,
// OR you can use 'arduino-cli'.
// Follow typical installation process for the 'arduino-cli'.
// The following bash script can be used to automatically program the Arduino UNO:

// echo "ArduinoProgrammer.sh <sketch_file>"
// x=${1%.*}
// y=${x##*/}
// /path/to/arduino-cli sketch new $x
// cp $1 $x/$y.ino
// /path/to/arduino-cli compile --fqbn arduino:avr:uno $x/$y.ino
// /path/to/arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno $x/$y.ino
// /path/to/arduino-cli monitor -p /dev/ttyACM0

// You can put that script into a .sh file to let it program automatically.
// No extra libraries should be needed to be installed here,
// the <EEPROM.h> library should be built in and does not need to be installed separately.


#include <EEPROM.h>

unsigned char shared_memory[512]; // duplicated in first 4K from $0000-$0FFF
unsigned char screen_memory[960]; // used for 40x24 and 64x15 fast vertical scrolling (and rogue variables too...)

const bool serial_debug = false; // change to stop error messages
const bool serial_output = true; // change to stop serial output
const bool serial_input = true; // change to stop serial input
unsigned char serial_last = 0x00;

int display_width = 40; // 40 or 64
int display_height = 24; // 24 or 15
int display_left = 12; // 12 or 0
int display_top = 1; // 1 or 5

const bool display_check = false; // double checks reads and writes
const int display_delay = 1; // microseconds

const int display_input_clock = 6;
const int display_input_data = 7;

const int display_output_clock = 4;
const int display_output_data = 5;

//const int display_ram_bank = 8; // not used anymore
//int display_ram_bank_value = 0;

void display_toggleinput()
{
	digitalWrite(display_input_clock, HIGH);
	digitalWrite(display_input_clock, LOW);
};

void display_toggleoutput()
{
	digitalWrite(display_output_clock, HIGH);
	digitalWrite(display_output_clock, LOW);
};

unsigned char display_receivebyte()
{
	unsigned char temp_value = 0x00;

	for (int i=0; i<8; i++)
	{
		temp_value = temp_value << 1;

		if (digitalRead(display_input_data) == HIGH)
		{
			temp_value += 0x01;
		}

		display_toggleinput();
	}
	
	return temp_value;
};

void display_sendbyte(const unsigned char value)
{
	unsigned char temp_value = value;

	for (int i=0; i<8; i++)
	{
		if (temp_value >= 0x80)
		{
			digitalWrite(display_output_data, HIGH);
		}
		else
		{
			digitalWrite(display_output_data, LOW);
		}

		temp_value = temp_value << 1;

		display_toggleoutput();
	}
};

unsigned char display_receivepacket(const unsigned char high, const unsigned char low) // double check each byte received
{
	if (display_check)
	{
		display_sendbyte((unsigned char)((high&0x3F)+0x80));
		display_sendbyte((unsigned char)(low));
		display_sendbyte((unsigned char)(0x00));
		delayMicroseconds(display_delay); // just in case
		display_toggleoutput();

		unsigned char temp_check[2];

		temp_check[1] = temp_check[0];

		temp_check[0] = display_receivebyte(); 

		do
		{
			display_sendbyte((unsigned char)((high&0x3F)+0x80));
			display_sendbyte((unsigned char)(low));
			display_sendbyte((unsigned char)(0x00));
			delayMicroseconds(display_delay); // just in case
			display_toggleoutput();
	
			temp_check[1] = temp_check[0];
	
			temp_check[0] = display_receivebyte();
	
			if (serial_debug)
			{
				if (temp_check[0] != temp_check[1])
				{
					unsigned char temp_binary;
	
					Serial.println("");
	
					Serial.print('R');
					Serial.print(' ');
					
					temp_binary = temp_check[0];
	
					for (int i=0; i<8; i++)
					{
						if (temp_binary >= 0x80) Serial.print('1');
						else Serial.print('0');
					
						temp_binary = temp_binary << 1;
		
						if (i == 3) Serial.print(' ');
					}
					
					Serial.println("");
	
					Serial.print(' ');
					Serial.print(' ');
	
					temp_binary = temp_check[1];
	
					for (int i=0; i<8; i++)
					{
						if (temp_binary >= 0x80) Serial.print('1');
						else Serial.print('0');
					
						temp_binary = temp_binary << 1;
	
						if (i == 3) Serial.print(' ');
					}
	
					Serial.println("");
				}
			}
		} 
		while (temp_check[0] != temp_check[1]);

		return temp_check[0];
	}
	else
	{
		display_sendbyte((unsigned char)((high&0x3F)+0x80));
		display_sendbyte((unsigned char)(low));
		display_sendbyte((unsigned char)(0x00));
		delayMicroseconds(display_delay); // just in case
		display_toggleoutput();	
	
		return display_receivebyte();
	}
};

void display_sendpacket(const unsigned char high, const unsigned char low, const unsigned char value) // only check bytes NOT sent to screen
{
	if (display_check)
	{
		unsigned char temp_check;

		do
		{
			display_sendbyte((unsigned char)((high&0x3F)+0xC0));
			display_sendbyte((unsigned char)(low));
			display_sendbyte((unsigned char)(value));
			delayMicroseconds(display_delay); // just in case
			display_toggleoutput();
	
			if ((unsigned char)(high&0x08) == 0x08)
			{
				display_sendbyte((unsigned char)((high&0x3F)+0x80));
				display_sendbyte((unsigned char)(low));
				display_sendbyte((unsigned char)(value));
				delayMicroseconds(display_delay); // just in case
				display_toggleoutput();
	
				temp_check = display_receivebyte();
			}
			else
			{
				temp_check = value;
			}
	
			if (serial_debug)
			{
				if (temp_check != value)
				{
					unsigned char temp_binary;
	
					Serial.println("");
	
					Serial.print('W');
					Serial.print(' ');
					
					temp_binary = temp_check;
	
					for (int i=0; i<8; i++)
					{
						if (temp_binary >= 0x80) Serial.print('1');
						else Serial.print('0');
					
						temp_binary = temp_binary << 1;
		
						if (i == 3) Serial.print(' ');
					}
					
					Serial.println("");
	
					Serial.print(' ');
					Serial.print(' ');
	
					temp_binary = value;
	
					for (int i=0; i<8; i++)
					{
						if (temp_binary >= 0x80) Serial.print('1');
						else Serial.print('0');
					
						temp_binary = temp_binary << 1;
	
						if (i == 3) Serial.print(' ');
					}
	
					Serial.println("");
				}
			}
		}
		while (temp_check != value);
	}
	else
	{
		display_sendbyte((unsigned char)((high&0x3F)+0xC0));
		display_sendbyte((unsigned char)(low));
		display_sendbyte((unsigned char)(value));
		delayMicroseconds(display_delay); // just in case
		display_toggleoutput();
	}
};

unsigned char display_receivecharacter(const unsigned char row, const unsigned char column)
{
	unsigned char low = (unsigned char)((unsigned char)(column & 0x3F) + (unsigned char)((row & 0x03) << 6));
	unsigned char high = (unsigned char)((row & 0xFC) >> 2);

	return display_receivepacket(high, low);
};

void display_sendcharacter(const unsigned char row, const unsigned char column, const unsigned char value)
{
	unsigned char low = (unsigned char)((unsigned char)(column & 0x3F) + (unsigned char)((row & 0x03) << 6));
	unsigned char high = (unsigned char)((row & 0xFC) >> 2);

	display_sendpacket(high, low, value);
};

void display_initialize()
{
	pinMode(display_output_clock, OUTPUT);
	pinMode(display_output_data, OUTPUT);

	digitalWrite(display_output_clock, LOW);
	digitalWrite(display_output_data, LOW);

	pinMode(display_input_clock, OUTPUT);
	pinMode(display_input_data, INPUT);

	digitalWrite(display_input_clock, LOW);

	for (int i=0; i<32; i++)
	{
		display_toggleoutput(); // clears shift register
	}
};

void display_clearmemory()
{	
	for (int i=0; i<16384; i++)
	{
		display_sendpacket((unsigned char)(i/256), (unsigned char)(i%256), 0x00);
	}
};


void string_copy(const char *A, char *B)
{
	for (int i=0; i<256; i++)
	{
		if (A[i] == 0 || A[i] == '\n') break;
		else B[i] = A[i];
	}
};

void string_print(int Y, int X, const char *A)
{
	for (int i=0; i<256; i++)
	{
		if (A[i] == 0 || A[i] == '\n') break;
		else
		{
			display_sendcharacter(Y, X, A[i]);

			X++;

			if (serial_output)
			{
				Serial.print((char)A[i]);
				delay(1);
			}
		}
	}

	if (serial_output)
	{
		Serial.println("");
		delay(1);
	}
};


const int keyboard_clock = 2;
const int keyboard_data = 3;

volatile unsigned char keyboard_bit = 0x00;
volatile unsigned char keyboard_byte = 0x00;
volatile unsigned char keyboard_counter = 0x00;
volatile unsigned char keyboard_buffer[16];
volatile unsigned char keyboard_read_pos = 0x00;
volatile unsigned char keyboard_write_pos = 0x00;
unsigned char keyboard_extended = 0x00;
unsigned char keyboard_release = 0x00;
unsigned char keyboard_shift = 0x00;
unsigned char keyboard_capslock = 0x00;
unsigned char keyboard_mode = 0x00;
unsigned char keyboard_serial = 0x00;

unsigned char keyboard_pos_x = 0x00;
unsigned char keyboard_pos_y = 0x00;
unsigned char keyboard_invert = 0x80; // 0x00 or 0x80

const unsigned char keyboard_text[128] PROGMEM = 
"\\ for Editor, and to Break. Commands:\nHELP, QUIT, CLEAR, ALIGN, INVERT,\nLOAD, SAVE, NEW, DIR, RUN, and MON\" \" _";

const unsigned char keyboard_conversion[256] PROGMEM = {
	 0x00,0x16,0x0C,0x0F,0x1E,0x1C,0x1D,0x15,
	 0x00,0x18,0x07,0x0E,0x1F,0x09,0x60,0x00,
	 0x00,0x00,0x00,0x00,0x00,0x71,0x31,0x00,
	 0x00,0x00,0x7A,0x73,0x61,0x77,0x32,0x00,
	 0x00,0x63,0x78,0x64,0x65,0x34,0x33,0x00,
	 0x00,0x20,0x76,0x66,0x74,0x72,0x35,0x00,
	 0x00,0x6E,0x62,0x68,0x67,0x79,0x36,0x00,
	 0x00,0x00,0x6D,0x6A,0x75,0x37,0x38,0x00,
	 0x00,0x2C,0x6B,0x69,0x6F,0x30,0x39,0x00,
	 0x00,0x2E,0x2F,0x6C,0x3B,0x70,0x2D,0x00,
	 0x00,0x00,0x27,0x00,0x5B,0x3D,0x00,0x00,
	 0x00,0x00,0x0D,0x5D,0x00,0x5C,0x00,0x00,
	 0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,
	 0x00,0x31,0x00,0x34,0x37,0x00,0x00,0x00,
	 0x30,0x2E,0x32,0x35,0x36,0x38,0x1B,0x00,
	 0x19,0x2B,0x33,0x2D,0x2A,0x39,0x00,0x00,

	 0x00,0x16,0x0C,0x0F,0x1E,0x1C,0x1D,0x15,
	 0x00,0x18,0x07,0x0E,0x1F,0x09,0x7E,0x00,
	 0x00,0x00,0x00,0x00,0x00,0x51,0x21,0x00,
	 0x00,0x00,0x5A,0x53,0x41,0x57,0x40,0x00,
	 0x00,0x43,0x58,0x44,0x45,0x24,0x23,0x00,
	 0x00,0x20,0x56,0x46,0x54,0x52,0x25,0x00,
	 0x00,0x4E,0x42,0x48,0x47,0x59,0x5E,0x00,
	 0x00,0x00,0x4D,0x4A,0x55,0x26,0x2A,0x00,
	 0x00,0x3C,0x4B,0x49,0x4F,0x29,0x28,0x00,
	 0x00,0x3E,0x3F,0x4C,0x3A,0x50,0x5F,0x00,
	 0x00,0x00,0x22,0x00,0x7B,0x2B,0x00,0x00,
	 0x00,0x00,0x0D,0x7D,0x00,0x7C,0x00,0x00,
	 0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,
	 0x00,0x03,0x00,0x13,0x02,0x00,0x00,0x00,
	 0x1A,0x7F,0x12,0x35,0x14,0x11,0x1B,0x00,
	 0x19,0x2B,0x04,0x2D,0x2A,0x01,0x00,0x00
};

void keyboard_interrupt()
{
	keyboard_bit = digitalRead(keyboard_data);
	
	keyboard_byte = keyboard_byte >> 1;

	if (keyboard_bit == HIGH) keyboard_byte += 0x80;

	keyboard_counter++;

	if (keyboard_counter == 0x09)
	{
		keyboard_buffer[keyboard_write_pos] = keyboard_byte;
		keyboard_write_pos++;
		if (keyboard_write_pos >= 0x10) keyboard_write_pos = 0x00;
	}
	else if (keyboard_counter == 0x0B)
	{
		keyboard_counter = 0x00;
	}

	while (digitalRead(keyboard_clock) == LOW) {}

	return;
};

void keyboard_initialize()
{
	pinMode(keyboard_clock, INPUT_PULLUP);
	pinMode(keyboard_data, INPUT_PULLUP);

	attachInterrupt(digitalPinToInterrupt(keyboard_clock), keyboard_interrupt, LOW);

	interrupts(); // just in case
};

void keyboard_clearscreen()
{
	keyboard_pos_x = 0x00;
	keyboard_pos_y = 0x00;

	for (int i=0; i<2048; i++)
	{
		display_sendpacket(keyboard_pos_y, keyboard_pos_x, 0x00);

		keyboard_pos_x++;

		if (keyboard_pos_x == 0x00) keyboard_pos_y++;
	}
	
	for (int i=0; i<display_height; i++)
	{
		for (int j=0; j<display_width; j++)
		{
			screen_memory[i*display_width+j] = 0x00;
		}
	}

	for (int i=display_top; i<display_height+display_top+4; i++)
	{
		for (int j=display_left; j<display_width+display_left; j++)
		{
			display_sendcharacter(i, j, keyboard_invert);
		}
	}

	keyboard_pos_x = (unsigned char)display_left;
	keyboard_pos_y = (unsigned char)display_top;
};

void keyboard_menu()
{
	for (int i=0; i<display_width; i++)
	{
		display_sendcharacter((unsigned char)(display_height+display_top), (unsigned char)(i+display_left), (unsigned char)('_'+keyboard_invert));
	}

	char temp_char, temp_mode;

	keyboard_pos_x = (unsigned char)display_left;
	keyboard_pos_y = (unsigned char)(display_height+display_top) + 1;

	temp_mode = keyboard_mode;

	keyboard_mode = 0x00;

	for (int i=0; i<128; i++)
	{
		temp_char = (char)pgm_read_byte_near(keyboard_text + (unsigned char)(i));

		if (temp_char == '_') break;

		if (temp_char == '\n')
		{
			temp_char = 0x00; 

			display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(' '+keyboard_invert));

			keyboard_pos_x = (unsigned char)display_left;
			keyboard_pos_y += 1;

			if (serial_output)
			{
				Serial.println("");
			}
		}
		else
		{
			keyboard_print(temp_char);
		}
	}

	display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(' '+keyboard_invert));

	if (serial_output)
	{
		Serial.println("");
	}

	keyboard_mode = temp_mode;

	audio_note(2000); // just a beep

	keyboard_pos_x = (unsigned char)display_left;
	keyboard_pos_y = (unsigned char)display_top;
	display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(screen_memory[(keyboard_pos_y-display_top)*display_width+keyboard_pos_x-display_left]+0x80+keyboard_invert)); // cursor
};

void keyboard_verticalscroll()
{
	for (int i=0; i<display_height; i++)
	{
		for (int j=0; j<display_width; j++)
		{
			screen_memory[i*display_width+j] = screen_memory[(i+1)*display_width+j];
		}
	}
		
	for (int j=0; j<display_width; j++)
	{
		screen_memory[(display_height-1)*display_width+j] = 0x00;
	}

	for (int i=0; i<display_height; i++)
	{
		for (int j=0; j<display_width; j++)
		{
			display_sendcharacter(i+display_top, j+display_left, screen_memory[i*display_width+j]+keyboard_invert);
		}
	}

	return;
};

unsigned char keyboard_character()
{
	unsigned char temp_value = 0x00;
	unsigned char temp_second = 0x00;

	if (keyboard_read_pos != keyboard_write_pos)
	{
		temp_value = keyboard_buffer[keyboard_read_pos];

		if (temp_value == 0xF0) // release
		{
			keyboard_release = 0xF0;
		}
		else if (temp_value == 0xE0) // extended
		{
			keyboard_extended = 0xE0;
		}
		else
		{
			if (keyboard_release > 0x00)
			{
				if (temp_value == 0x12 || temp_value == 0x59) keyboard_shift = 0x00;
			}
			else
			{
				if (temp_value == 0x58) keyboard_capslock += 0x80;
				else if (temp_value == 0x12 || temp_value == 0x59) keyboard_shift = 0xFF;

				if (keyboard_capslock != 0x00) temp_value += 0x80;

				if (keyboard_shift == 0xFF) temp_value += 0x80;

				if (keyboard_extended > 0x00)
				{
					if (temp_value == 0x4A || temp_value == 0xCA) temp_value += 0x80; // numpad slash
					
					temp_second = (unsigned char)pgm_read_byte_near(keyboard_conversion + (unsigned char)(temp_value+0x80));
				}
				else
				{
					temp_second = (unsigned char)pgm_read_byte_near(keyboard_conversion + (unsigned char)(temp_value+0x00));
				}
			}

			keyboard_release = 0x00;
			keyboard_extended = 0x00;
		}

		keyboard_read_pos++;
		if (keyboard_read_pos >= 0x10) keyboard_read_pos = 0x00;

		return temp_second;
	}
	else
	{
		if (serial_input)
		{
			if (Serial.available() > 0)
			{
				if (serial_last == 0x10) // special serial key
				{
					serial_last = Serial.read();
				}
				else
				{
					serial_last = 0x10;
				}

				delay(1);

				return (unsigned char)serial_last;					
			}
			else
			{
				if (serial_last == 0x10)
				{
					serial_last = 0x0D;
				}
				else if (serial_last == 0x0D)
				{
					serial_last = 0x00;
				}
				else if (serial_last != 0x00)
				{
					serial_last = 0x10;
				}

				delay(1);

				return serial_last;
			}
		}
	
		return 0x00;
	}
};

void keyboard_print(unsigned char value)
{
	unsigned char temp_value = 0x00;
	unsigned char temp_x = 0x00;
	unsigned char temp_y = 0x00;

	if (value == 0x00) return;
	else if (value == 0x10) // special serial key
	{
		keyboard_serial = 0x01;
	}
	else if (value == 0x1C) // F1
	{

	}
	else if (value == 0x1D) // F2
	{
		
	}
	else if (value == 0x1E) // F3
	{
	
	}
	else if (value == 0x1F) // F4
	{

	}
	else if (value == 0x1A) // insert
	{

	}
	else if (value == 0x7F) // delete
	{

	}
	else if (value == 0x01) // page up
	{

	}
	else if (value == 0x02) // home
	{

	}
	else if (value == 0x03) // end
	{

	}
	else if (value == 0x04) // page down
	{

	}
	else if (value == 0x0D || value == 0x8D) // carriage return
	{
		display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(screen_memory[(keyboard_pos_y-display_top)*display_width+keyboard_pos_x-display_left]+keyboard_invert+0x00));

		keyboard_pos_x = (unsigned char)display_left;
		if (keyboard_pos_y < (unsigned char)(display_height+display_top)-1) keyboard_pos_y++;
		else
		{
			keyboard_verticalscroll();
	
			keyboard_pos_y = (unsigned char)(display_height+display_top)-1;
		}

		display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(screen_memory[(keyboard_pos_y-display_top)*display_width+keyboard_pos_x-display_left]+keyboard_invert+0x80));

		if (serial_output)
		{
			if (keyboard_serial == 0x00)
			{
				Serial.println("");
				delay(1);
			}
		}
	}
	else if (value == 0x11 || value == 0x91) // up arrow
	{
		if (keyboard_mode == 0x00)
		{
			display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(screen_memory[(keyboard_pos_y-display_top)*display_width+keyboard_pos_x-display_left]+keyboard_invert+0x00));
	
			if (keyboard_pos_y > (unsigned char)display_top) keyboard_pos_y--;
	
			display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(screen_memory[(keyboard_pos_y-display_top)*display_width+keyboard_pos_x-display_left]+keyboard_invert+0x80));
		}
	}
	else if (value == 0x12 || value == 0x92) // down arrow
	{
		if (keyboard_mode == 0x00)
		{
			display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(screen_memory[(keyboard_pos_y-display_top)*display_width+keyboard_pos_x-display_left]+keyboard_invert+0x00));
	
			if (keyboard_pos_y < (unsigned char)(display_height+display_top)-1) keyboard_pos_y++;
	
			display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(screen_memory[(keyboard_pos_y-display_top)*display_width+keyboard_pos_x-display_left]+keyboard_invert+0x80));
		}
	}
	else if (value == 0x08 || value == 0x13 || value == 0x88 || value == 0x93) // backspace or left arrow
	{
		if (keyboard_mode == 0x00 || value == 0x08 || value == 0x88)
		{
			display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(screen_memory[(keyboard_pos_y-display_top)*display_width+keyboard_pos_x-display_left]+keyboard_invert+0x00));

			if (keyboard_pos_x > (unsigned char)display_left) keyboard_pos_x--;

			display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(screen_memory[(keyboard_pos_y-display_top)*display_width+keyboard_pos_x-display_left]+keyboard_invert+0x80));
		}
	}
	else if (value == 0x09 || value == 0x14 || value == 0x89 || value == 0x94) // tab or right arrow
	{
		if (keyboard_mode == 0x00 || value == 0x09 || value == 0x89)
		{
			display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(screen_memory[(keyboard_pos_y-display_top)*display_width+keyboard_pos_x-display_left]+keyboard_invert+0x00));

			if (keyboard_pos_x < (unsigned char)(display_width+display_left-1)) keyboard_pos_x++;

			display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(screen_memory[(keyboard_pos_y-display_top)*display_width+keyboard_pos_x-display_left]+keyboard_invert+0x80));
		}
	}
	else if (value == 0x1B || value == 0x9B) // escape
	{
		if (keyboard_mode == 0x00)
		{
			keyboard_clearscreen();

			keyboard_menu();
	
			if (serial_output)
			{
				if (keyboard_serial == 0x00)
				{
					Serial.println("");
					delay(1);
				}
			}
		}
	}
	else
	{
		if (keyboard_mode != 0x00)
		{
			if (value > 0x60 && value <= 0x7A) value = (unsigned char)(value - 0x20); // upper case only
		}

		display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(value+keyboard_invert));
		screen_memory[(keyboard_pos_y-display_top)*display_width+keyboard_pos_x-display_left] = (unsigned char)value;

		if (keyboard_pos_x < (unsigned char)display_width+display_left-1) keyboard_pos_x++;
		else
		{
			keyboard_pos_x = (unsigned char)display_left;
			if (keyboard_pos_y < (unsigned char)(display_height+display_top)-1) keyboard_pos_y++;
			else
			{
				keyboard_verticalscroll();
				keyboard_pos_y = (unsigned char)(display_height+display_top)-1;
			}
		}

		display_sendcharacter(keyboard_pos_y, keyboard_pos_x, (unsigned char)(screen_memory[(keyboard_pos_y-display_top)*display_width+keyboard_pos_x-display_left]+keyboard_invert+0x80));

		if (serial_output)
		{
			if (keyboard_serial == 0x00)
			{
				Serial.print((char)value);
				delay(1);
			}
		}
	}
	
	if (value != 0x10) // special serial key
	{
		keyboard_serial = 0x00;
	}
};


const int sdcard_ss = 10;
const int sdcard_mosi = 11;
const int sdcard_miso = 12;
const int sdcard_sclk = 13;

void sdcard_enable()
{
	digitalWrite(sdcard_ss, LOW);
};

void sdcard_disable()
{
	digitalWrite(sdcard_ss, HIGH);
};

void sdcard_toggle()
{
	digitalWrite(sdcard_sclk, HIGH);
	digitalWrite(sdcard_sclk, LOW);
}

void sdcard_longdelay()
{
	delay(10);
};

void sdcard_sendbyte(unsigned char value)
{
	unsigned char temp_value = value;

	for (int i=0; i<8; i++)
	{
		if (temp_value >= 0x80)
		{
			digitalWrite(sdcard_mosi, HIGH);
		}
		else
		{
			digitalWrite(sdcard_mosi, LOW);
		}

		temp_value = temp_value << 1;

		sdcard_toggle();
	}
};

unsigned char sdcard_receivebyte()
{
	unsigned char temp_value = 0x00;

	for (int i=0; i<8; i++)
	{
		temp_value = temp_value << 1;

		if (digitalRead(sdcard_miso) == HIGH)
		{
			temp_value += 0x01;
		}

		sdcard_toggle();
	}

	return temp_value;
};

unsigned char sdcard_waitresult()
{
	unsigned char temp_value = 0xFF;

	for (int i=0; i<2048; i++) // arbitrary wait time
	{
		temp_value = sdcard_receivebyte();

		if (temp_value != 0xFF)
		{
			return temp_value;
		}
	}

	return 0xFF;
};

void sdcard_pump()
{
	digitalWrite(sdcard_ss, HIGH); // must disable the device
	digitalWrite(sdcard_mosi, HIGH); // AND leave mosi high!!!

	sdcard_longdelay();

	for (int i=0; i<80; i++)
	{
		sdcard_toggle();
	}
};

int sdcard_initialize()
{
	unsigned char temp_value = 0x00;

	pinMode(sdcard_ss, OUTPUT);
	pinMode(sdcard_mosi, OUTPUT);
	pinMode(sdcard_miso, INPUT_PULLUP);
	pinMode(sdcard_sclk, OUTPUT);

	digitalWrite(sdcard_ss, HIGH);
	digitalWrite(sdcard_mosi, LOW);
	digitalWrite(sdcard_sclk, LOW);

	sdcard_disable();
	sdcard_pump();
	sdcard_longdelay();
	sdcard_enable();
	sdcard_sendbyte(0x40); // CMD0 = 0x40 + 0x00 (0 in hex)
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x95); // CRC for CMD0
	temp_value = sdcard_waitresult(); // command response
	if (temp_value == 0xFF) { return 0; }
	sdcard_disable();
	if (temp_value != 0x01) { return 0; } // expecting 0x01
	sdcard_longdelay();
	sdcard_pump();
	sdcard_enable();
	sdcard_sendbyte(0x48); // CMD8 = 0x40 + 0x08 (8 in hex)
	sdcard_sendbyte(0x00); // CMD8 needs 0x000001AA argument
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x01);
	sdcard_sendbyte(0xAA); 
	sdcard_sendbyte(0x87); // CRC for CMD8
	temp_value = sdcard_waitresult(); // command response
	if (temp_value == 0xFF) { return 0; }
	sdcard_disable();
	if (temp_value != 0x01) { return 0; } // expecting 0x01
	sdcard_enable();
	temp_value = sdcard_receivebyte(); // 32-bit return value, ignore
	temp_value = sdcard_receivebyte();
	temp_value = sdcard_receivebyte();
	temp_value = sdcard_receivebyte();
	sdcard_disable();
	do {
		sdcard_pump();
		sdcard_longdelay();
		sdcard_enable();
		sdcard_sendbyte(0x77); // CMD55 = 0x40 + 0x37 (55 in hex)
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x01); // CRC (general)
		temp_value = sdcard_waitresult(); // command response
		if (temp_value == 0xFF) { return 0; }
		sdcard_disable();
		if (temp_value != 0x01) { return 0; } // expecting 0x01
		sdcard_pump();
		sdcard_longdelay();
		sdcard_enable();
		sdcard_sendbyte(0x69); // CMD41 = 0x40 + 0x29 (41 in hex)
		sdcard_sendbyte(0x40); // needed for CMD41?
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x00);
		sdcard_sendbyte(0x01); // CRC (general)
		temp_value = sdcard_waitresult(); // command response
		if (temp_value == 0xFF) { return 0; }
		sdcard_disable();
		if (temp_value != 0x00 && temp_value != 0x01) { return 0; } // expecting 0x00, if 0x01 try again
		sdcard_longdelay();
	} while (temp_value == 0x01);

	return 1;
};

int sdcard_readblock(unsigned char high, unsigned char low)
{
	unsigned char temp_value = 0x00;

	sdcard_disable();
	sdcard_pump();
	sdcard_longdelay();
	sdcard_enable();
	sdcard_sendbyte(0x51); // CMD17 = 0x40 + 0x11 (17 in hex)
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(high);
	sdcard_sendbyte(low&0xFE); // only blocks of 512 bytes
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x01); // CRC (general)
	temp_value = sdcard_waitresult(); // command response
	if (temp_value == 0xFF) { return 0; }
	else if (temp_value != 0x00) { return 0; } // expecting 0x00
	temp_value = sdcard_waitresult(); // data packet starts with 0xFE
	if (temp_value == 0xFF) { return 0; }
	else if (temp_value != 0xFE) { return 0; }
	for (int i=0; i<512; i++) // packet of 512 bytes
	{
		temp_value = sdcard_receivebyte();
		shared_memory[i] = temp_value;
	}
	temp_value = sdcard_receivebyte(); // data packet ends with 0x55 then 0xAA
	temp_value = sdcard_receivebyte(); // ignore here
	sdcard_disable();

	return 1;
};

int sdcard_writeblock(unsigned char high, unsigned char low)
{
	unsigned char temp_value = 0x00;

	sdcard_disable();
	sdcard_pump();
	sdcard_longdelay();
	sdcard_enable();
	sdcard_sendbyte(0x58); // CMD24 = 0x40 + 0x18 (24 in hex)
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(high);
	sdcard_sendbyte(low&0xFE); // only blocks of 512 bytes
	sdcard_sendbyte(0x00);
	sdcard_sendbyte(0x01); // CRC (general)
	temp_value = sdcard_waitresult(); // command response
	if (temp_value == 0xFF) { return 0; }
	else if (temp_value != 0x00) { return 0; } // expecting 0x00
	sdcard_sendbyte(0xFE); // data packet starts with 0xFE
	for (int i=0; i<512; i++) // packet of 512 bytes
	{
		temp_value = shared_memory[i]; 
		sdcard_sendbyte(temp_value);
	}
	sdcard_sendbyte(0x55); // data packet ends with 0x55 then 0xAA
	sdcard_sendbyte(0xAA);
	temp_value = sdcard_receivebyte(); // toggle clock 8 times
	sdcard_disable();

	return 1;
};

const int audio_output = 9;

void audio_note(unsigned int value)
{
	pinMode(audio_output, OUTPUT);

	digitalWrite(audio_output, LOW);

	unsigned char temp_value = 0x00;

	for (unsigned long i=0; i<(unsigned long)(100000/(value+1)); i++) // needs modifying a litte
	{
		digitalWrite(audio_output, HIGH);

		delayMicroseconds(value);

		digitalWrite(audio_output, LOW);

		delayMicroseconds(value);
	}
};

unsigned char eeprom_read(unsigned char low, unsigned char high)
{	
	return EEPROM.read((unsigned int)((high*256)+low)%1024);
};

void eeprom_write(unsigned char low, unsigned char high, unsigned char data)
{
	EEPROM.write((unsigned int)((high*256)+low)%1024, (unsigned char)data);
};


unsigned char x6502_A = 0x00;
unsigned char x6502_X = 0x00;
unsigned char x6502_Y = 0x00;
unsigned char x6502_L = 0x00;
unsigned char x6502_H = 0x00;
unsigned char x6502_S = 0x00;
unsigned char x6502_F = 0x00;

unsigned char x6502_U = 0x00;
unsigned char x6502_V = 0x00;
unsigned char x6502_W = 0x00;

unsigned char x6502_read(unsigned char BL, unsigned char BH)
{
	if (BH < 0x40)
	{
		return (unsigned char)shared_memory[(unsigned int)((BH&0x01)*256+BL)]; // duplicated in first 16K from $0000-$3FFF
	}
	else
	{
		return display_receivepacket((unsigned char)(BH&0x3F), BL); // duplicated on every other 16KB from $4000-$FFFF
	}
};

void x6502_write(unsigned char BL, unsigned char BH, unsigned char BD)
{
	if (BH < 0x40)
	{
		shared_memory[(unsigned int)((BH&0x01)*256+BL)] = (unsigned char)BD; // duplicated in first 16K from $0000-$3FFF
	}
	else
	{
		display_sendpacket((unsigned char)(BH&0x3F), BL, BD); // duplicated on every other 16KB from $4000-$FFFF
	}	
};

void x6502_next()
{
	x6502_L += 0x01;
	if (x6502_L == 0x00)
	{
		x6502_H += 0x01;
	}
};

void x6502_abs()
{
	x6502_U = x6502_read(x6502_L, x6502_H);
	x6502_next();
	x6502_V = x6502_read(x6502_L, x6502_H);
	x6502_next();
};

void x6502_absx()
{
	x6502_U = x6502_read(x6502_L, x6502_H);
	x6502_next();
	x6502_V = x6502_read(x6502_L, x6502_H);
	x6502_next();
	
	if ((int)x6502_U + (int)x6502_X > 255)
	{
		x6502_U += x6502_X;
		x6502_V++;
	}
	else x6502_U += x6502_X;
};

void x6502_absy()
{
	x6502_U = x6502_read(x6502_L, x6502_H);
	x6502_next();
	x6502_V = x6502_read(x6502_L, x6502_H);
	x6502_next();
	
	if ((int)x6502_U + (int)x6502_Y > 255)
	{
		x6502_U += x6502_Y;
		x6502_V++;
	}
	else x6502_U += x6502_Y;
};

void x6502_imm()
{
	x6502_U = x6502_read(x6502_L, x6502_H);
	x6502_next();
};

void x6502_rel()
{
	x6502_U = x6502_read(x6502_L, x6502_H);
	x6502_next();
};

void x6502_zp()
{
	x6502_U = x6502_read(x6502_L, x6502_H);
	x6502_next();
};

void x6502_zpx()
{
	x6502_U = (unsigned char)(x6502_read(x6502_L, x6502_H) + x6502_X);
	x6502_next();
};

void x6502_zpy()
{
	x6502_U = (unsigned char)(x6502_read(x6502_L, x6502_H) + x6502_X);
	x6502_next();
};

void x6502_jump(unsigned char BL, unsigned char BH)
{
	x6502_L = BL;
	x6502_H = BH;
};

void x6502_jump_indirect(unsigned char BL, unsigned char BH)
{
	x6502_L = x6502_read(BL, BH);
	if ((unsigned char)(BL+0x01) == 0x00)
	{
		x6502_H = x6502_read(0x00, (unsigned char)(BH+0x01));
	}
	else
	{
		x6502_H = x6502_read((unsigned char)(BL+0x01), BH);
	}
};

void x6502_branch(unsigned char B)
{
	if (B >= 0x80)
	{
		if ((int)x6502_L - (int)(0xFF - B + 1) < 0)
		{
			x6502_L -= (0xFF - B + 1);
			x6502_H--;
		}
		else x6502_L -= (0xFF - B + 1);
	}
	else
	{
		if ((int)x6502_L + (int)B > 255)
		{
			x6502_L += B;
			x6502_H++;
		}
		else x6502_L += B;
	}
};

void x6502_push(unsigned char B)
{
	x6502_write(x6502_S, 0x01, B);
	x6502_S--;
};

unsigned char x6502_pull()
{
	x6502_S++;
	return x6502_read(x6502_S, 0x01);
};

void x6502_neg(unsigned char B)
{
	if (B > 0x00) x6502_F = x6502_F | 0x80;
	else x6502_F = x6502_F & 0x7F;
};

void x6502_over(unsigned char B)
{
	if (B > 0x00) x6502_F = x6502_F | 0x40;
	else x6502_F = x6502_F & 0xBF;
};

void x6502_zero(unsigned char B)
{
	if (B > 0x00) x6502_F = x6502_F | 0x02;
	else x6502_F = x6502_F & 0xFD;
};

void x6502_carry(unsigned char B)
{
	if (B > 0x00) x6502_F = x6502_F | 0x01;
	else x6502_F = x6502_F & 0xFE;
};

unsigned char x6502_adc(unsigned char B)
{
	if ((int)x6502_A + (int)B + (int)(x6502_F & 0x01) > 255) x6502_over(0xFF);

	unsigned char C = B + (x6502_F & 0x01);
	
	x6502_neg(C&0x80);
	x6502_zero(C);
	x6502_carry(0x00); // ???

	return C;
};

unsigned char x6502_and(unsigned char B)
{
	unsigned char C = x6502_A & B;

	x6502_neg(C&0x80);
	x6502_zero(C);

	return C;
};

unsigned char x6502_asl(unsigned char B)
{
	if (B >= 0x80) x6502_carry(0xFF);
	else x6502_carry(0x00);

	unsigned char C = B << 1;
	
	x6502_neg(C&0x80);
	x6502_zero(C);
	
	return C;
};

void x6502_bit(unsigned char B)
{
	x6502_neg(B&0x80);
	x6502_over(B&0x40);
	x6502_zero(B);

	return;
};

void x6502_cmp(unsigned char B)
{
	unsigned char C = x6502_A - B - (0x01 - (x6502_F&0x01));

	if ((int)x6502_A - (int)B < 0) x6502_carry(0xFF);
	else x6502_carry(0x00);

	x6502_neg(C&0x80);
	x6502_zero(C);
	
	return;
};

void x6502_cpx(unsigned char B)
{
	unsigned char C = x6502_X - B - (0x01 - (x6502_F&0x01));

	if ((int)x6502_X - (int)B < 0) x6502_carry(0xFF);
	else x6502_carry(0x00);

	x6502_neg(C&0x80);
	x6502_zero(C);
	
	return;
};

void x6502_cpy(unsigned char B)
{
	unsigned char C = x6502_Y - B - (0x01 - (x6502_F&0x01));

	if ((int)x6502_Y - (int)B < 0) x6502_carry(0xFF);
	else x6502_carry(0x00);

	x6502_neg(C&0x80);
	x6502_zero(C);
	
	return;
};

unsigned char x6502_dec(unsigned char B)
{
	unsigned char C = B - 1;
	
	x6502_neg(C&0x80);
	x6502_zero(C);

	return C;
};

unsigned char x6502_eor(unsigned char B)
{
	unsigned char C = x6502_A ^ B;

	x6502_neg(C&0x80);
	x6502_zero(C);

	return C;
};

unsigned char x6502_inc(unsigned char B)
{
	unsigned char C = B + 1;
	
	x6502_neg(C&0x80);
	x6502_zero(C);

	return C;
};

void x6502_lda(unsigned char B)
{
	x6502_A = B;

	x6502_neg(x6502_A&0x80);
	x6502_zero(x6502_A);

	return;
};

void x6502_ldx(unsigned char B)
{
	x6502_X = B;

	x6502_neg(x6502_X&0x80);
	x6502_zero(x6502_X);

	return;
};

void x6502_ldy(unsigned char B)
{
	x6502_Y = B;

	x6502_neg(x6502_Y&0x80);
	x6502_zero(x6502_Y);

	return;
};

unsigned char x6502_lsr(unsigned char B)
{
	if ((B&0x01) > 0x00) x6502_carry(0xFF);
	else x6502_carry(0x00);

	unsigned char C = B >> 1;

	x6502_neg(C&0x80);
	x6502_zero(C);

	return C;
};

unsigned char x6502_ora(unsigned char B)
{
	unsigned char C = x6502_A | B;

	x6502_neg(C&0x80);
	x6502_zero(C);

	return C;
};

unsigned char x6502_rol(unsigned char B)
{
	unsigned char D = x6502_F & 0x01;

	if (B >= 0x80) x6502_carry(0xFF);
	else x6502_carry(0x00);

	unsigned char C = B << 1;

	C += D;

	x6502_neg(C&0x80);
	x6502_zero(C);

	return C;
};
	
unsigned char x6502_ror(unsigned char B)
{
	unsigned char D = x6502_F & 0x80;

	if ((B&0x01) > 0x00) x6502_carry(0xFF);
	else x6502_carry(0x00);

	unsigned char C = B >> 1;

	C += D;

	x6502_neg(C&0x80);
	x6502_zero(C);

	return C;
};

unsigned char x6502_sbc(unsigned char B)
{
	unsigned char C = x6502_A - B - (0x01 - (x6502_F&0x01));

	if ((int)x6502_A - (int)B < 0) x6502_carry(0xFF);
	else x6502_carry(0x00);

	x6502_neg(C&0x80);
	x6502_zero(C);

	return C;
};

void x6502_tax()
{
	x6502_X = x6502_A;
	
	x6502_neg(x6502_A&0x80);
	x6502_zero(x6502_A);
	
	return;
};

void x6502_tay()
{
	x6502_Y = x6502_A;
	
	x6502_neg(x6502_A&0x80);
	x6502_zero(x6502_A);
	
	return;
};

void x6502_txa()
{
	x6502_A = x6502_X;
	
	x6502_neg(x6502_A&0x80);
	x6502_zero(x6502_A);
	
	return;
};

void x6502_tya()
{
	x6502_A = x6502_Y;
	
	x6502_neg(x6502_A&0x80);
	x6502_zero(x6502_A);
	
	return;
};

void x6502_tsx()
{
	x6502_X = x6502_S;
	
	x6502_neg(x6502_X&0x80);
	x6502_zero(x6502_X);
	
	return;
};

void x6502_txs()
{
	x6502_S = x6502_X;
	
	x6502_neg(x6502_X&0x80);
	x6502_zero(x6502_X);
	
	return;
};
	
int x6502_instruction()
{
	unsigned char B, C;

	unsigned char inst = x6502_read(x6502_L, x6502_H);
	x6502_next();

	switch (inst)
	{
		case 0x00: // BRK
		{
			return 0;
		}
		case 0x01: // ORA(zpx)
		{
			x6502_zpx();
			x6502_A = x6502_ora(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0x05: // ORAzp
		{
			x6502_zp();
			x6502_A = x6502_ora(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0x06: // ASLzp
		{
			x6502_zp();
			x6502_write(x6502_U, 0x00, x6502_asl(x6502_read(x6502_U, 0x00)));
			break;
		}	
		case 0x08: // PHP
		{
			x6502_push(x6502_F);
			break;
		}
		case 0x09: // ORA#
		{
			x6502_imm();
			x6502_A = x6502_ora(x6502_U);
			break;
		}
		case 0x0A: // ASLA
		{
			x6502_A = x6502_asl(x6502_A);
			break;
		}
		case 0x0D: // ORAa
		{
			x6502_abs();
			x6502_A = x6502_ora(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x0E: // ASLa
		{
			x6502_abs();
			x6502_write(x6502_U, x6502_V, x6502_asl(x6502_read(x6502_U, x6502_V)));
			break;
		}
		case 0x10: // BPL
		{
			x6502_rel();
			if ((x6502_F&0x80) == 0x00) x6502_branch(x6502_U);
			break;
		}
		case 0x11: // ORA(zp)y
		{
			x6502_zp();
			x6502_A = x6502_ora(x6502_read(x6502_read(x6502_U, 0x00) + x6502_Y, 0x00));
			break;
		}
		case 0x12: // ORA(zp)
		{
			x6502_zp();
			x6502_A = x6502_ora(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0x15: // ORAzpx
		{
			x6502_zpx();
			x6502_A = x6502_ora(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0x16: // ASLzpx
		{
			x6502_zpx();
			x6502_write(x6502_U, 0x00, x6502_asl(x6502_read(x6502_U, 0x00)));
			break;
		}	
		case 0x18: // CLC
		{
			x6502_carry(0x00);
			break;
		}
		case 0x19: // ORAay
		{
			x6502_absy();
			x6502_A = x6502_ora(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x1A: // INCA
		{
			x6502_A = x6502_inc(x6502_A);
			break;
		}
		case 0x1D: // ORAax
		{
			x6502_absx();
			x6502_A = x6502_ora(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x1E: // ASLax
		{
			x6502_absx();
			x6502_write(x6502_U, x6502_V, x6502_asl(x6502_read(x6502_U, x6502_V)));
			break;
		}
		case 0x20: // JSRa
		{
			x6502_abs();
			x6502_push(x6502_H);
			x6502_push(x6502_L);
			x6502_jump(x6502_U, x6502_V);
			break;
		}
		case 0x21: // AND(zpx)
		{
			x6502_zpx();
			x6502_A = x6502_and(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0x24: // BITzp
		{
			x6502_zp();
			x6502_bit(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0x25: // ANDzp
		{
			x6502_zp();
			x6502_A = x6502_and(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0x26: // ROLzp
		{
			x6502_zp();
			x6502_write(x6502_U, 0x00, x6502_rol(x6502_read(x6502_U, 0x00)));
			break;
		}	
		case 0x28: // PLP
		{
			x6502_F = x6502_pull();
			break;
		}
		case 0x29: // AND#
		{
			x6502_imm();
			x6502_A = x6502_and(x6502_U);
			break;
		}
		case 0x2A: // ROLA
		{
			x6502_A = x6502_rol(x6502_A);
			break;
		}
		case 0x2C: // BITa
		{
			x6502_abs();
			x6502_bit(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x2D: // ANDa
		{
			x6502_abs();
			x6502_A = x6502_and(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x2E: // ROLa
		{
			x6502_abs();	
			x6502_write(x6502_U, x6502_V, x6502_rol(x6502_read(x6502_U, x6502_V)));
			break;
		}
		case 0x30: // BMI
		{
			x6502_rel();	
			if ((x6502_F&0x80) != 0x00) x6502_branch(x6502_U);
			break;
		}
		case 0x31: // AND(zp)y
		{
			x6502_zp();
			x6502_A = x6502_and(x6502_read(x6502_read(x6502_U, 0x00) + x6502_Y, 0x00));
			break;
		}
		case 0x32: // AND(zp)
		{
			x6502_zp();
			x6502_A = x6502_and(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0x34: // BITzpx
		{
			x6502_zpx();
			x6502_bit(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0x35: // ANDzpx
		{
			x6502_zpx();
			x6502_A = x6502_and(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0x36: // ROLzpx
		{
			x6502_zpx();
			x6502_write(x6502_U, 0x00, x6502_rol(x6502_read(x6502_U, 0x00)));
			break;
		}	
		case 0x38: // SEC
		{
			x6502_carry(0xFF);
			break;
		}
		case 0x39: // ANDay
		{
			x6502_absy();
			x6502_A = x6502_and(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x3A: // DECA
		{
			x6502_A = x6502_dec(x6502_A);
			break;
		}
		case 0x3C: // BITax
		{
			x6502_absx();
			x6502_bit(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x3D: // ANDax
		{
			x6502_absx();	
			x6502_A = x6502_and(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x3E: // ROLax
		{
			x6502_absx();
			x6502_write(x6502_U, x6502_V, x6502_rol(x6502_read(x6502_U, x6502_V)));
			break;
		}
		case 0x40: // RTI
		{
			x6502_F = x6502_pull();
			x6502_L = x6502_pull();
			x6502_H = x6502_pull();
			break;
		}
		case 0x41: // EOR(zpx)
		{
			x6502_zpx();
			x6502_A = x6502_eor(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0x45: // EORzp
		{
			x6502_zp();
			x6502_A = x6502_eor(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0x46: // LSRzp
		{
			x6502_zp();
			x6502_write(x6502_U, 0x00, x6502_lsr(x6502_read(x6502_U, 0x00)));
			break;
		}	
		case 0x48: // PHA
		{
			x6502_push(x6502_A);
			break;
		}
		case 0x49: // EOR#
		{
			x6502_imm();
			x6502_A = x6502_eor(x6502_U);
			break;
		}
		case 0x4A: // LSRA
		{
			x6502_A = x6502_lsr(x6502_A);
			break;
		}
		case 0x4C: // JMPa
		{
			x6502_abs();	
			x6502_jump(x6502_U, x6502_V);
			break;
		}
		case 0x4D: // EORa
		{	
			x6502_abs();
			x6502_A = x6502_eor(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x4E: // LSRa
		{
			x6502_abs();
			x6502_write(x6502_U, x6502_V, x6502_eor(x6502_read(x6502_U, x6502_V)));
			break;
		}
		case 0x50: // BVC
		{
			x6502_rel();
			if ((x6502_F&0x40) == 0x00) x6502_branch(x6502_U);
			break;
		}
		case 0x51: // EOR(zp)y
		{
			x6502_zp();
			x6502_A = x6502_eor(x6502_read(x6502_read(x6502_U, 0x00) + x6502_Y, 0x00));
			break;
		}
		case 0x52: // EOR(zp)
		{
			x6502_zp();
			x6502_A = x6502_eor(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0x55: // EORzpx
		{
			x6502_zpx();
			x6502_A = x6502_eor(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0x56: // LSRzpx
		{
			x6502_zpx();
			x6502_write(x6502_U, 0x00, x6502_lsr(x6502_read(x6502_U, 0x00)));
			break;
		}	
		case 0x58: // CLI
		{
			x6502_F = x6502_F & 0xFB;
			break;
		}
		case 0x59: // EORay
		{
			x6502_absy();
			x6502_A = x6502_eor(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x5A: // PHY
		{
			x6502_push(x6502_Y);
			break;
		}
		case 0x5D: // EORax
		{	
			x6502_absx();
			x6502_A = x6502_eor(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x5E: // LSRax
		{
			x6502_absx();
			x6502_write(x6502_U, x6502_V, x6502_lsr(x6502_read(x6502_U, x6502_V)));
			break;
		}
		case 0x60: // RTS
		{
			x6502_L = x6502_pull();
			x6502_H = x6502_pull();
			break;
		}
		case 0x61: // ADC(zpx)
		{
			x6502_zpx();
			x6502_A = x6502_adc(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0x64: // STZzp
		{
			x6502_zp();
			x6502_write(x6502_U, 0x00, 0x00);
			break;
		}
		case 0x65: // ADCzp
		{
			x6502_zp();
			x6502_A = x6502_adc(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0x66: // RORzp
		{
			x6502_zp();
			x6502_write(x6502_U, 0x00, x6502_ror(x6502_read(x6502_U, 0x00)));
			break;
		}	
		case 0x68: // PLA
		{
			x6502_A = x6502_pull();
			break;
		}
		case 0x69: // ADC#
		{
			x6502_imm();
			x6502_A = x6502_adc(x6502_U);
			break;
		}
		case 0x6A: // RORA
		{	
			x6502_A = x6502_ror(x6502_A);
			break;
		}
		case 0x6C: // JMP(a)
		{
			x6502_abs();
			x6502_jump_indirect(x6502_U, x6502_V);
			break;
		}
		case 0x6D: // ADCa
		{	
			x6502_abs();
			x6502_A = x6502_adc(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x6E: // RORa
		{
			x6502_abs();
			x6502_write(x6502_U, x6502_V, x6502_ror(x6502_read(x6502_U, x6502_V)));
			break;
		}
		case 0x70: // BVS
		{
			x6502_rel();
			if ((x6502_F&0x40) != 0x00) x6502_branch(x6502_U);
			break;
		}
		case 0x71: // ADC(zp)y
		{
			x6502_zp();
			x6502_A = x6502_adc(x6502_read(x6502_read(x6502_U, 0x00) + x6502_Y, 0x00));
			break;
		}
		case 0x72: // ADC(zp)
		{
			x6502_zp();
			x6502_A = x6502_adc(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0x74: // STZzpx
		{
			x6502_zpx();
			x6502_write(x6502_U, 0x00, 0x00);
			break;
		}
		case 0x75: // ADCzpx
		{
			x6502_zpx();
			x6502_A = x6502_adc(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0x76: // RORzpx
		{
			x6502_zpx();
			x6502_write(x6502_U, 0x00, x6502_ror(x6502_read(x6502_U, 0x00)));
			break;
		}	
		case 0x78: // SEI
		{
			x6502_F = x6502_F | 0x04;
			break;
		}	
		case 0x79: // ADCay
		{	
			x6502_absy();
			x6502_A = x6502_adc(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x7A: // PLY
		{
			x6502_Y = x6502_pull();
			break;
		}
		case 0x7C: // JMP(ax)
		{
			x6502_absx();
			x6502_jump_indirect(x6502_U, x6502_V);
			break;
		}
		case 0x7D: // ADCax
		{
			x6502_absx();
			x6502_A = x6502_adc(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0x7E: // RORax
		{
			x6502_absx();
			x6502_write(x6502_U, x6502_V, x6502_ror(x6502_read(x6502_U, x6502_V)));
			break;
		}
		case 0x80: // BRA
		{
			x6502_rel();
			x6502_branch(x6502_U);
			break;
		}
		case 0x81: // STA(zpx)
		{
			x6502_zpx();
			x6502_write(x6502_read(x6502_read(x6502_U, 0x00), 0x00), 0x00, x6502_A);
			break;
		}
		case 0x84: // STYzp
		{
			x6502_zp();
			x6502_write(x6502_U, 0x00, x6502_Y);
			break;
		}
		case 0x85: // STAzp
		{
			x6502_zp();
			x6502_write(x6502_U, 0x00, x6502_A);
			break;
		}
		case 0x86: // STXzp
		{
			x6502_zp();
			x6502_write(x6502_U, 0x00, x6502_X);
			break;
		}	
		case 0x88: // DEY
		{
			x6502_Y = x6502_dec(x6502_Y);
			break;
		}
		case 0x89: // BIT#
		{
			x6502_imm();
			x6502_bit(x6502_U);
			break;
		}
		case 0x8A: // TXA
		{
			x6502_txa();
			break;
		}
		case 0x8C: // STYa
		{
			x6502_abs();
			x6502_write(x6502_U, x6502_V, x6502_Y);
			break;
		}
		case 0x8D: // STAa
		{	
			x6502_abs();
			x6502_write(x6502_U, x6502_V, x6502_A);
			break;
		}
		case 0x8E: // STXa
		{
			x6502_abs();
			x6502_write(x6502_U, x6502_V, x6502_X);
			break;
		}
		case 0x90: // BCC
		{
			x6502_rel();
			if ((x6502_F&0x01) == 0x00) x6502_branch(x6502_U);
			break;
		}
		case 0x91: // STA(zp)y
		{
			x6502_zp();
			x6502_write(x6502_read(x6502_read(x6502_U, 0x00) + x6502_Y, 0x00), 0x00, x6502_A);
			break;
		}
		case 0x92: // STA(zp)
		{
			x6502_zp();
			x6502_write(x6502_read(x6502_read(x6502_U, 0x00), 0x00), 0x00, x6502_A);
			break;
		}
		case 0x94: // STYzpx
		{
			x6502_zpx();
			x6502_write(x6502_U, 0x00, x6502_Y);
			break;
		}
		case 0x95: // STAzpx
		{
			x6502_zpx();
			x6502_write(x6502_U, 0x00, x6502_A);
			break;
		}
		case 0x96: // STXzpy
		{
			x6502_zpy();
			x6502_write(x6502_U, 0x00, x6502_X);
			break;
		}	
		case 0x98: // TYA
		{
			x6502_tya();
			break;
		}
		case 0x99: // STAay
		{
			x6502_absy();
			x6502_write(x6502_U, x6502_V, x6502_A);
			break;
		}
		case 0x9A: // TXS
		{
			x6502_txs();
			break;
		}
		case 0x9C: // STZa
		{
			x6502_abs();
			x6502_write(x6502_U, x6502_V, 0x00);
			break;
		}
		case 0x9D: // STAax
		{
			x6502_absx();
			x6502_write(x6502_U, x6502_V, x6502_A);
			break;
		}	
		case 0x9E: // STZax
		{
			x6502_absx();
			x6502_write(x6502_U, x6502_V, 0x00);
			break;
		}
		case 0xA0: // LDY#
		{
			x6502_imm();
			x6502_ldy(x6502_U);
			break;
		}
		case 0xA1: // LDA(zpx)
		{
			x6502_zpx();
			x6502_lda(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0xA2: // LDX#
		{
			x6502_imm();
			x6502_ldx(x6502_U);
			break;
		}
		case 0xA4: // LDYzp
		{
			x6502_zp();
			x6502_ldy(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0xA5: // LDAzp
		{
			x6502_zp();
			x6502_lda(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0xA6: // LDXzp
		{
			x6502_zp();
			x6502_ldx(x6502_read(x6502_U, 0x00));
			break;
		}	
		case 0xA8: // TAY
		{
			x6502_tay();
			break;
		}
		case 0xA9: // LDA#
		{
			x6502_imm();
			x6502_lda(x6502_U);
			break;
		}
		case 0xAA: // TAX
		{
			x6502_tax();
			break;
		}
		case 0xAC: // LDYa
		{
			x6502_abs();
			x6502_ldy(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xAD: // LDAa
		{
			x6502_abs();
			x6502_lda(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xAE: // LDXa
		{
			x6502_absx();
			x6502_ldx(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xB0: // BCS
		{
			x6502_rel();
			if ((x6502_F&0x01) > 0x00) x6502_branch(x6502_U);
			break;
		}
		case 0xB1: // LDA(zp)y
		{
			x6502_zp();
			x6502_lda(x6502_read(x6502_read(x6502_U, 0x00) + x6502_Y, 0x00));
			break;
		}
		case 0xB2: // LDA(zp)
		{
			x6502_zp();
			x6502_lda(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0xB4: // LDYzpx
		{
			x6502_zpx();
			x6502_ldy(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0xB5: // LDAzpx
		{
			x6502_zpx();
			x6502_lda(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0xB6: // LDXzpy
		{
			x6502_zpy();
			x6502_ldx(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0xB8: // CLV
		{
			x6502_over(0x00);
			break;
		}
		case 0xB9: // LDAay
		{
			x6502_absy();
			x6502_lda(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xBA: // TSX
		{
			x6502_tsx();
			break;
		}
		case 0xBC: // LDYax
		{
			x6502_absx();
			x6502_ldy(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xBD: // LDAax
		{
			x6502_absx();
			x6502_lda(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xBE: // LDXay
		{
			x6502_absy();
			x6502_ldx(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xC0: // CPY#
		{
			x6502_imm();
			x6502_cpy(x6502_U);
			break;
		}
		case 0xC1: // CMP(zpx)
		{
			x6502_zpx();
			x6502_cmp(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0xC4: // CPYzp
		{
			x6502_zp();
			x6502_cpy(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0xC5: // CMPzp
		{
			x6502_zp();
			x6502_cmp(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0xC6: // DECzp
		{
			x6502_zp();
			x6502_write(x6502_U, 0x00, x6502_dec(x6502_read(x6502_U, 0x00)));
			break;
		}
		case 0xC8: // INY
		{
			x6502_Y = x6502_inc(x6502_Y);
			break;
		}
		case 0xC9: // CMP#
		{
			x6502_imm();
			x6502_cmp(x6502_U);
			break;
		}
		case 0xCA: // DEX
		{
			x6502_X = x6502_dec(x6502_X);
			break;
		}
		case 0xCB: // WAI
		{
			return 0;
		}
		case 0xCC: // CPYa
		{
			x6502_abs();
			x6502_cpy(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xCD: // CMPa
		{
			x6502_abs();
			x6502_cmp(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xCE: // DECa
		{
			x6502_abs();
			x6502_write(x6502_U, x6502_V, x6502_dec(x6502_read(x6502_U, x6502_V)));
			break;
		}
		case 0xD0: // BNE
		{
			x6502_rel();
			if ((x6502_F&0x02) != 0x00) x6502_branch(x6502_U);
			break;
		}
		case 0xD1: // CMP(zp)y
		{
			x6502_zp();
			x6502_cmp(x6502_read(x6502_read(x6502_U, 0x00) + x6502_Y, 0x00));
			break;
		}
		case 0xD2: // CMP(zp)
		{
			x6502_zp();
			x6502_cmp(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0xD5: // CMPzpx
		{
			x6502_zpx();
			x6502_cmp(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0xD6: // DECzpx
		{
			x6502_zpx();
			x6502_write(x6502_U, 0x00, x6502_dec(x6502_read(x6502_U, 0x00)));
			break;
		}
		case 0xD8: // CLD
		{
			return;
		}
		case 0xD9: // CMPay
		{
			x6502_absy();
			x6502_cmp(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xDA: // PHX
		{
			x6502_push(x6502_X);
			break;
		}
		case 0xDB: // STP
		{
			return 0;
		}
		case 0xDD: // CMPax
		{
			x6502_absx();
			x6502_cmp(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xDE: // DECax
		{
			x6502_absx();
			x6502_write(x6502_U, x6502_V, x6502_dec(x6502_read(x6502_U, x6502_V)));
			break;
		}
		case 0xE0: // CPX#
		{
			x6502_imm();
			x6502_cpx(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xE1: // SBC(zpx)
		{
			x6502_zpx();
			x6502_A = x6502_sbc(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0xE4: // CPXzp
		{
			x6502_zp();
			x6502_cpx(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0xE5: // SBCzp
		{
			x6502_zp();
			x6502_A = x6502_sbc(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0xE6: // INCzp
		{
			x6502_zp();
			x6502_write(x6502_U, 0x00, x6502_inc(x6502_read(x6502_U, 0x00)));
			break;
		}
		case 0xE8: // INX
		{
			x6502_X = x6502_inc(x6502_X);
			break;
		}
		case 0xE9: // SBC#
		{
			x6502_imm();
			x6502_A = x6502_sbc(x6502_U);
			break;
		}
		case 0xEA: // NOP
		{
			break;
		}
		case 0xEC: // CPXa
		{
			x6502_abs();
			x6502_cpx(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xED: // SBCa
		{
			x6502_abs();
			x6502_A = x6502_sbc(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xEE: // INCa
		{
			x6502_abs();
			x6502_write(x6502_U, x6502_V, x6502_inc(x6502_read(x6502_U, x6502_V)));
			break;
		}
		case 0xF0: // BEQ
		{
			x6502_rel();
			if ((x6502_F&0x02) == 0x00) x6502_branch(x6502_U);
			break;
		}
		case 0xF1: // SBC(zp)y
		{
			x6502_zp();
			x6502_A = x6502_sbc(x6502_read(x6502_read(x6502_U, 0x00) + x6502_Y, 0x00));
			break;
		}
		case 0xF2: // SBC(zp)
		{
			x6502_zp();
			x6502_A = x6502_sbc(x6502_read(x6502_read(x6502_U, 0x00), 0x00));
			break;
		}
		case 0xF5: // SBCzpx
		{
			x6502_zpx();
			x6502_A = x6502_sbc(x6502_read(x6502_U, 0x00));
			break;
		}
		case 0xF6: // INCzpx
		{
			x6502_zpx();
			x6502_write(x6502_U, 0x00, x6502_inc(x6502_read(x6502_U, 0x00)));
			break;
		}
		case 0xF8: // SED
		{
			return 0;
		}
		case 0xF9: // SBCay
		{
			x6502_absy();
			x6502_A = x6502_sbc(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xFA: // PLX
		{
			x6502_X = x6502_pull();
			break;
		}
		case 0xFD: // SBCax
		{
			x6502_absx();
			x6502_A = x6502_sbc(x6502_read(x6502_U, x6502_V));
			break;
		}
		case 0xFE: // INCax
		{
			x6502_absx();
			x6502_write(x6502_U, x6502_V, x6502_inc(x6502_read(x6502_U, x6502_V)));
			break;
		}
		default:
		{
			return 0;
		}
	}

	return 1;
};

void x6502_reset()
{
	x6502_A = 0x00;
	x6502_X = 0x00;
	x6502_Y = 0x00;
	x6502_L = 0x00;
	x6502_H = 0x00;
	x6502_S = 0xFF;
	x6502_F = 0x00;
};

void x6502_run(unsigned char BL, unsigned char BH)
{
	x6502_L = BL;
	x6502_H = BH;

	while (x6502_instruction()) {} 

	return;
};


const unsigned char command_size = 64;
unsigned char command_string[command_size];

unsigned int editor_start = 0x5000; // change later if need be
unsigned int editor_end = 0x6000; // change later if need be
unsigned int editor_blocks = 8; // 512 blocks to be saved to SD card
unsigned int editor_total = editor_start + 256;
char editor_character = 0x00;
char editor_prompt = '\\';
char editor_prompt_caps = '|';

unsigned char monitor_addr_start_nibble[4];
unsigned char monitor_addr_stop_nibble[4];
unsigned char monitor_data_nibble[2];

unsigned char basic_variables = 0x00; // 256 byte page

const unsigned char editor_text[480] PROGMEM = 
	"BASIC Keywords:\n  IF [THEN or END], GOTO, MON\" \",\n  PRINT, INPUT, SING\nVariable Arrays: ABCDWXYZ! with ()\nMath Operators: +*-/%\nComparator Operators: =#<>\nMonitor Example:\n  MON \"0000:EE0F00DC;JJJ,0000.000F\"\nPrime Numbers Example:\n  10 PRINT 'TYPE NUMBER'\n  20 INPUT X\n  30 A = 2\n  40 PRINT A, ';'\n  50 A = A + 1\n  60 IF A > X THEN GOTO 10000\n  70 B = A - 1\n  80 IF A % B = 0 THEN GOTO 50\n  90 B = B - 1\n  100 IF B = 1 THEN GOTO 40\n  110 GOTO 80_";


unsigned char editor_break()
{
	if (editor_character == editor_prompt || editor_character == editor_prompt_caps || editor_character == 0x1B)
	{
		return 0x01; // slash (or ESC) to break
	}

	editor_character = keyboard_character();
	
	return 0x00;
};

void editor_checkmemory()
{
	// checking for RAM expansion
	x6502_write((unsigned char)(editor_start%256), (unsigned char)(editor_start/256), 0xA5);

	if (x6502_read((unsigned char)(editor_start%256), (unsigned char)(editor_start/256+0x20)) == 0xA5)
	{
		editor_end = editor_start + 0x1800; // only 8KB available
		editor_blocks = 12;
	}
	else
	{
		editor_end = editor_start + 0x3800; // full 16KB available
		editor_blocks = 28;
	}

	x6502_write((unsigned char)(editor_start%256), (unsigned char)(editor_start/256), 0x00);
};

unsigned char editor_load()
{
	unsigned char v = 0x00;

	for (int init=0; init<5; init++)
	{
		if (editor_break()) break;

		if (sdcard_initialize())
		{
			v = 0x01;

			for (int i=0; i<editor_blocks; i++)
			{
				if (editor_break()) break;

				if (!sdcard_readblock(0x00,(unsigned char)(0x02*i)))
				{
					v = 0x00;
				}
			
				for (int j=0; j<512; j++)
				{
					if (editor_break()) break;

					x6502_write((unsigned char)(j%256), (unsigned char)(((editor_start&0xFF00)>>8)+(int)(0x02*i)+(int)(j/256)), shared_memory[j]);
				}
			}
		}
	}

	for (int i=0; i<512; i++) shared_memory[i] = eeprom_read((unsigned char)(i%256), (unsigned char)(i/256));

	return v;
};

unsigned char editor_save()
{
	unsigned char v = 0x00;

	unsigned char key = 0x00;

	for (int i=0; i<512; i++) eeprom_write((unsigned char)(i%256), (unsigned char)(i/256), shared_memory[i]);

	for (int init=0; init<5; init++)
	{
		if (editor_break()) break;

		if (sdcard_initialize())
		{	
			v = 0x01;

			for (int i=0; i<editor_blocks; i++)
			{
				if (editor_break()) break;

				for (int j=0; j<512; j++)
				{
					if (editor_break()) break;

					shared_memory[j] = x6502_read((unsigned char)(j%256), (unsigned char)(((editor_start&0xFF00)>>8)+(int)(0x02*i)+(int)(j/256)));
				}

				if (!sdcard_writeblock(0x00,(unsigned char)(0x02*i)))
				{
					v = 0x00;
				}
			}
		}
	}

	for (int i=0; i<512; i++) shared_memory[i] = eeprom_read((unsigned char)(i%256), (unsigned char)(i/256));

	return v;
};
	
void monitor_execute(int start, int end)
{
	unsigned char addr_pos = 0x00;
	unsigned char addr_offset = 0x00;
	unsigned char data_pos = 0x00;

	unsigned int temp_addr;
	unsigned int temp_offset;
	unsigned char temp_data;

	//for (int i=0; i<4; i++) monitor_addr_start_nibble[i] = 0x00;
	//for (int i=0; i<4; i++) monitor_addr_stop_nibble[i] = 0x00;
	//for (int i=0; i<2; i++) monitor_data_nibble[i] = 0x00;

	unsigned char key = 0x00;

	bool printed = false;

	if (start < 0 || end > command_size) return;

	for (int i=start; i<end; i++)
	{
		if (editor_break()) break;

		if (command_string[i] == 'J') // jump
		{
			addr_offset = 0x00;

			temp_addr = (unsigned int)((monitor_addr_start_nibble[0] << 12) + (monitor_addr_start_nibble[1] << 8) + 
				(monitor_addr_start_nibble[2] << 4) + monitor_addr_start_nibble[3] + addr_offset);

			x6502_run((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));

			addr_pos = 0x00;
			addr_offset = 0x00;
			data_pos = 0x00;
		}
		else if ((command_string[i] >= 0x30 && command_string[i] <= 0x39) || 
			(command_string[i] >= 0x41 && command_string[i] <= 0x46))
		{
			if (command_string[i] >= 0x30 && command_string[i] <= 0x39)
			{
				temp_data = (unsigned char)(command_string[i] - 0x30);
			}
			else if (command_string[i] >= 0x41 && command_string[i] <= 0x46)
			{
				temp_data = (unsigned char)(command_string[i] - 0x41 + 0x0A);
			}

			if (data_pos == 0x00)
			{
				if (addr_pos < 0x04)
				{
					monitor_addr_start_nibble[addr_pos] = temp_data;
					addr_pos++;

					if (addr_pos == 0x04)
					{
						addr_pos = 0x00;
	
						keyboard_print(0x0D);

						addr_offset = 0x00;
	
						temp_addr = (unsigned int)((monitor_addr_start_nibble[0] << 12) + (monitor_addr_start_nibble[1] << 8) + 
							(monitor_addr_start_nibble[2] << 4) + monitor_addr_start_nibble[3] + addr_offset);
	
						if ((unsigned char)((temp_addr&0xF000)>>12) <= 0x09) keyboard_print(((temp_addr&0xF000)>>12) + '0');
						else if ((unsigned char)((temp_addr&0xF000)>>12) <= 0x0F) keyboard_print(((temp_addr&0xF000)>>12) + 'A' - 0x0A);
	
						if ((unsigned char)((temp_addr&0x0F00)>>8) <= 0x09) keyboard_print(((temp_addr&0x0F00)>>8) + '0');
						else if ((unsigned char)((temp_addr&0x0F00)>>8) <= 0x0F) keyboard_print(((temp_addr&0x0F00)>>8) + 'A' - 0x0A);
	
						if ((unsigned char)((temp_addr&0x00F0)>>4) <= 0x09) keyboard_print(((temp_addr&0x00F0)>>4) + '0');
						else if ((unsigned char)((temp_addr&0x00F0)>>4) <= 0x0F) keyboard_print(((temp_addr&0x00F0)>>4) + 'A' - 0x0A);
	
						if ((unsigned char)(temp_addr&0x000F) <= 0x09) keyboard_print((temp_addr&0x000F) + '0');
						else if ((unsigned char)(temp_addr&0x00F) <= 0x0F) keyboard_print((temp_addr&0x000F) + 'A' - 0x0A);
	
						keyboard_print(':');
						keyboard_print(' ');
				
						temp_data = x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
	
						if ((unsigned char)((temp_data&0xF0)>>4) <= 0x09) keyboard_print(((temp_data&0xF0)>>4) + '0');
						else if ((unsigned char)((temp_data&0xF0)>>4) <= 0x0F) keyboard_print(((temp_data&0xF0)>>4) + 'A' - 0x0A);
						
						if ((unsigned char)(temp_data&0x0F) <= 0x09) keyboard_print((temp_data&0x0F) + '0');
						else if ((unsigned char)(temp_data&0x0F) <= 0x0F) keyboard_print((temp_data&0x0F) + 'A' - 0x0A);

						keyboard_print(' ');

						printed = true;
					}
				}
				else
				{
					monitor_addr_stop_nibble[addr_pos-0x08] = temp_data;
					addr_pos++;

					if (addr_pos == 0x0C)
					{
						addr_pos = 0x08;

						addr_offset = 0x00;

						temp_addr = (unsigned int)((monitor_addr_start_nibble[0] << 12) + (monitor_addr_start_nibble[1] << 8) + 
							(monitor_addr_start_nibble[2] << 4) + monitor_addr_start_nibble[3] + addr_offset);

						temp_offset = (unsigned int)((monitor_addr_stop_nibble[0] << 12) + (monitor_addr_stop_nibble[1] << 8) + 
							(monitor_addr_stop_nibble[2] << 4) + monitor_addr_stop_nibble[3] + addr_offset);
	
						if (!printed)
						{
							keyboard_print(0x0D);

							temp_addr = (unsigned int)((monitor_addr_start_nibble[0] << 12) + (monitor_addr_start_nibble[1] << 8) + 
								(monitor_addr_start_nibble[2] << 4) + monitor_addr_start_nibble[3] + addr_offset);
	
							if ((unsigned char)((temp_addr&0xF000)>>12) <= 0x09) keyboard_print(((temp_addr&0xF000)>>12) + '0');
							else if ((unsigned char)((temp_addr&0xF000)>>12) <= 0x0F) keyboard_print(((temp_addr&0xF000)>>12) + 'A' - 0x0A);
	
							if ((unsigned char)((temp_addr&0x0F00)>>8) <= 0x09) keyboard_print(((temp_addr&0x0F00)>>8) + '0');
							else if ((unsigned char)((temp_addr&0x0F00)>>8) <= 0x0F) keyboard_print(((temp_addr&0x0F00)>>8) + 'A' - 0x0A);
	
							if ((unsigned char)((temp_addr&0x00F0)>>4) <= 0x09) keyboard_print(((temp_addr&0x00F0)>>4) + '0');
							else if ((unsigned char)((temp_addr&0x00F0)>>4) <= 0x0F) keyboard_print(((temp_addr&0x00F0)>>4) + 'A' - 0x0A);
	
							if ((unsigned char)(temp_addr&0x000F) <= 0x09) keyboard_print((temp_addr&0x000F) + '0');
							else if ((unsigned char)(temp_addr&0x00F) <= 0x0F) keyboard_print((temp_addr&0x000F) + 'A' - 0x0A);
	
							keyboard_print(':');
							keyboard_print(' ');
				
							temp_data = x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
	
							if ((unsigned char)((temp_data&0xF0)>>4) <= 0x09) keyboard_print(((temp_data&0xF0)>>4) + '0');
							else if ((unsigned char)((temp_data&0xF0)>>4) <= 0x0F) keyboard_print(((temp_data&0xF0)>>4) + 'A' - 0x0A);
						
							if ((unsigned char)(temp_data&0x0F) <= 0x09) keyboard_print((temp_data&0x0F) + '0');
							else if ((unsigned char)(temp_data&0x0F) <= 0x0F) keyboard_print((temp_data&0x0F) + 'A' - 0x0A);

							keyboard_print(' ');

							printed = true;
						}

						while (temp_addr < temp_offset)
						{
							if (editor_break()) break;

							temp_addr++;
	
							if (temp_addr % 0x08 == 0x00)
							{
								keyboard_print(0x0D);
	
								if ((unsigned char)((temp_addr&0xF000)>>12) <= 0x09) keyboard_print(((temp_addr&0xF000)>>12) + '0');
								else if ((unsigned char)((temp_addr&0xF000)>>12) <= 0x0F) keyboard_print(((temp_addr&0xF000)>>12) + 'A' - 0x0A);
	
								if ((unsigned char)((temp_addr&0x0F00)>>8) <= 0x09) keyboard_print(((temp_addr&0x0F00)>>8) + '0');
								else if ((unsigned char)((temp_addr&0x0F00)>>8) <= 0x0F) keyboard_print(((temp_addr&0x0F00)>>8) + 'A' - 0x0A);
	
								if ((unsigned char)((temp_addr&0x00F0)>>4) <= 0x09) keyboard_print(((temp_addr&0x00F0)>>4) + '0');
								else if ((unsigned char)((temp_addr&0x00F0)>>4) <= 0x0F) keyboard_print(((temp_addr&0x00F0)>>4) + 'A' - 0x0A);
	
								if ((unsigned char)(temp_addr&0x000F) <= 0x09) keyboard_print((temp_addr&0x000F) + '0');
								else if ((unsigned char)(temp_addr&0x00F) <= 0x0F) keyboard_print((temp_addr&0x000F) + 'A' - 0x0A);
	
								keyboard_print(':');
								keyboard_print(' ');
							}
				
							temp_data = x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
	
							if ((unsigned char)((temp_data&0xF0)>>4) <= 0x09) keyboard_print(((temp_data&0xF0)>>4) + '0');
							else if ((unsigned char)((temp_data&0xF0)>>4) <= 0x0F) keyboard_print(((temp_data&0xF0)>>4) + 'A' - 0x0A);
						
							if ((unsigned char)(temp_data&0x0F) <= 0x09) keyboard_print((temp_data&0x0F) + '0');
							else if ((unsigned char)(temp_data&0x0F) <= 0x0F) keyboard_print((temp_data&0x0F) + 'A' - 0x0A);

							keyboard_print(' ');

							printed = true;
						}
					}
				}
			}
			else
			{
				monitor_data_nibble[data_pos - 0x01] = temp_data;
				data_pos++;

				if (data_pos == 0x03)
				{
					data_pos = 0x01;
					
					temp_addr = (unsigned int)((monitor_addr_start_nibble[0] << 12) + (monitor_addr_start_nibble[1] << 8) + 
						(monitor_addr_start_nibble[2] << 4) + monitor_addr_start_nibble[3] + addr_offset);

					x6502_write((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8), 
						(unsigned char)((monitor_data_nibble[0] << 4) + monitor_data_nibble[1]));

					addr_offset++;
				}
			}
		}
		else if (command_string[i] == '.')
		{
			addr_pos = 0x08;
		}
		else if (command_string[i] == ',')
		{
			addr_pos = 0x00;
		}
		else if (command_string[i] == ':')
		{
			data_pos = 0x01;
		}
		else if (command_string[i] == ';')
		{
			data_pos = 0x00;
		}
		else
		{
			// ignore
		}
	}

	if (!printed)
	{
		keyboard_print(0x0D);

		addr_offset = 0x00;

		temp_addr = (unsigned int)((monitor_addr_start_nibble[0] << 12) + (monitor_addr_start_nibble[1] << 8) + 
			(monitor_addr_start_nibble[2] << 4) + monitor_addr_start_nibble[3] + addr_offset);
	
		if ((unsigned char)((temp_addr&0xF000)>>12) <= 0x09) keyboard_print(((temp_addr&0xF000)>>12) + '0');
		else if ((unsigned char)((temp_addr&0xF000)>>12) <= 0x0F) keyboard_print(((temp_addr&0xF000)>>12) + 'A' - 0x0A);
	
		if ((unsigned char)((temp_addr&0x0F00)>>8) <= 0x09) keyboard_print(((temp_addr&0x0F00)>>8) + '0');
		else if ((unsigned char)((temp_addr&0x0F00)>>8) <= 0x0F) keyboard_print(((temp_addr&0x0F00)>>8) + 'A' - 0x0A);
	
		if ((unsigned char)((temp_addr&0x00F0)>>4) <= 0x09) keyboard_print(((temp_addr&0x00F0)>>4) + '0');
		else if ((unsigned char)((temp_addr&0x00F0)>>4) <= 0x0F) keyboard_print(((temp_addr&0x00F0)>>4) + 'A' - 0x0A);
	
		if ((unsigned char)(temp_addr&0x000F) <= 0x09) keyboard_print((temp_addr&0x000F) + '0');
		else if ((unsigned char)(temp_addr&0x00F) <= 0x0F) keyboard_print((temp_addr&0x000F) + 'A' - 0x0A);
	
		keyboard_print(':');
		keyboard_print(' ');
				
		temp_data = x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
	
		if ((unsigned char)((temp_data&0xF0)>>4) <= 0x09) keyboard_print(((temp_data&0xF0)>>4) + '0');
		else if ((unsigned char)((temp_data&0xF0)>>4) <= 0x0F) keyboard_print(((temp_data&0xF0)>>4) + 'A' - 0x0A);
						
		if ((unsigned char)(temp_data&0x0F) <= 0x09) keyboard_print((temp_data&0x0F) + '0');
		else if ((unsigned char)(temp_data&0x0F) <= 0x0F) keyboard_print((temp_data&0x0F) + 'A' - 0x0A);

		keyboard_print(' ');
	}

	return;
};

int basic_number(unsigned int &addr, int &num)
{
	num = 0;

	int temp_value = 0;
	unsigned char temp_char;
	unsigned char temp_place;
	int temp_offset;
	unsigned char temp_byte;
	unsigned char temp_operation = '=';

	int key = 0;

	temp_offset = 0; // up to 16 each

	while (addr < editor_total)
	{
		if (editor_break()) { key = 1; break; }
	
		temp_char = (unsigned int)x6502_read((unsigned char)(addr&0x00FF), (unsigned char)((addr&0xFF00)>>8));

		addr++;

		if (temp_char == 'A' || temp_char == 'B' || temp_char == 'C' || temp_char == 'D' ||
			temp_char == 'W' || temp_char == 'X' || temp_char == 'Y' || temp_char == 'Z') // variable value
		{
			temp_place = temp_char;

			temp_offset = 0;

			temp_char = (unsigned int)x6502_read((unsigned char)(addr&0x00FF), (unsigned char)((addr&0xFF00)>>8));

			if (temp_char == '(' || temp_char == '[')
			{
				addr++;
	
				key = basic_number(addr, temp_offset);

				if (key == 1) break;
			}

			if (temp_place < 'W') temp_byte = x6502_read((unsigned char)((int)(temp_place-'A')*32+(int)(temp_offset%16)*2), basic_variables);
			else temp_byte = x6502_read((unsigned char)((int)(temp_place-'W')*32+128+(int)(temp_offset%16)*2), basic_variables);
	
			temp_value = (int)temp_byte;
			
			if (temp_place < 'W') temp_byte = x6502_read((unsigned char)((int)(temp_place-'A')*32+(int)(temp_offset%16)*2+1), basic_variables);
			else temp_byte = x6502_read((unsigned char)((int)(temp_place-'W')*32+128+(int)(temp_offset%16)*2+1), basic_variables);

			if (temp_byte >= 0x80) // negative
			{
				temp_value += (int)((int)(temp_byte&0x7F)*256);

				temp_value *= -1;
			}
			else
			{
				temp_value += (int)((int)temp_byte*256);
			}
		}
		else if (temp_char == '!') // random number
		{
			temp_value = random(32768);
		}
		else if (temp_char >= 0x30 && temp_char <= 0x39) // number value
		{
			temp_value *= 10;
			temp_value += (int)(temp_char - 0x30);
		}
		else if (temp_char == '=' || temp_char == '#' || temp_char == '<' || temp_char == '>' || temp_char == 'T' || 
			temp_char == '"' || temp_char == '\'' || temp_char == ',' || temp_char == '.' || temp_char == ':' || temp_char == ';')
		{
			if (temp_operation == '=')
			{
				num = temp_value;
			}
			else if (temp_operation == '+') // add
			{
				num += temp_value;
			}
			else if (temp_operation == '-') // sub
			{
				num -= temp_value;
			}
			else if (temp_operation == '*') // mul
			{
				num *= temp_value;	
			}
			else if (temp_operation == '/') // div
			{
				num /= temp_value;
			}
			else if (temp_operation == '%') // mod
			{
				num = num % temp_value;
			}

			temp_operation = 0x00;

			addr--;
	
			break;
		}
		else if (temp_char == '+' || temp_char == '-' || temp_char == '*' || temp_char == '/' || temp_char == '%') 
		{
			if (temp_operation == '=')
			{
				num = temp_value;
			}
			else if (temp_operation == '+') // add
			{
				num += temp_value;
			}
			else if (temp_operation == '-') // sub
			{
				num -= temp_value;
			}
			else if (temp_operation == '*') // mul
			{
				num *= temp_value;	
			}
			else if (temp_operation == '/') // div
			{
				num /= temp_value;
			}
			else if (temp_operation == '%') // mod
			{
				num = num % temp_value;
			}

			temp_value = 0;

			temp_operation = temp_char;
		}
		else if (temp_char == ')' || temp_char == ']')
		{
			break;
		}
		else if (temp_char == 0x10)
		{
			addr--;
	
			break;
		}
		else
		{
			// ignore
		}
	}

	if (temp_operation == '=') // equals
	{
		num = temp_value;
	}
	else if (temp_operation == '+') // add
	{
		num += temp_value;
	}
	else if (temp_operation == '-') // sub
	{
		num -= temp_value;
	}
	else if (temp_operation == '*') // mul
	{
		num *= temp_value;	
	}
	else if (temp_operation == '/') // div
	{
		num /= temp_value;
	}
	else if (temp_operation == '%') // mod
	{
		num = num % temp_value;
	}

	return key;
};

void basic_execute()
{
	unsigned int temp_addr = editor_start;
	unsigned int temp_line = 0x0000;

	int temp_place = 0;
	int temp_num = 0;
	int temp_compare = 0;

	unsigned char temp_char = 0x00;
	unsigned char temp_read = 0x00;

	int temp_offset = 0x00;

	unsigned int temp_variable = 0x00;

	unsigned char temp_last = 0x00;
	
	unsigned char temp_second;

	temp_line = (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
					
	temp_line *= (unsigned int)256;
	
	temp_line += (unsigned int)x6502_read((unsigned char)((temp_addr+1)&0x00FF), (unsigned char)(((temp_addr+1)&0xFF00)>>8));

	temp_line = temp_line % 32768;

	temp_addr += 2;

	while (temp_line != 0x0000 && temp_addr < editor_total)
	{
		if (editor_break()) break;

		temp_char = (unsigned char)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
		
		temp_addr++;

		temp_second = (unsigned char)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));

		if (temp_char == 'P' && temp_second == 'R') // print
		{
			while (temp_addr < editor_total)
			{
				if (editor_break()) break;				

				temp_char = (unsigned char)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
		
				if (temp_char == 0x10) break;
				else if (temp_char == '"' || temp_char == '\'' || (temp_char >= 0x30 && temp_char <= 0x39) || temp_char == '-' ||
					temp_char == 'A' || temp_char == 'B' || temp_char == 'C' || temp_char == 'D' ||
					temp_char == 'W' || temp_char == 'X' || temp_char == 'Y' || temp_char == 'Z')
				{		
					if (temp_char == '"' || temp_char == '\'')
					{
						temp_addr++;
	
						while (temp_addr < editor_total)
						{
							if (editor_break()) break;
	
							temp_char = (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
	
							temp_addr++;
	
							if (temp_char == '"' || temp_char == '\'')
							{
								break;
							}
							else if (temp_char == 0x10)
							{
								temp_addr--;
								
								break;
							}
							else if (temp_char == ';' || temp_char == ':')
							{
								keyboard_print(0x0D);
							}
							else
							{
								keyboard_print((char)temp_char);
							}
						}
					}
					else
					{
						if (basic_number(temp_addr, temp_num)) break;
		
						if (temp_num < 0) // negative
						{
							keyboard_print('-');
	
							temp_num *= -1;
						}
						
						temp_num = temp_num % 32768;
		
						temp_place = 0x00;
	
						if (temp_num >= 10000)
						{
							keyboard_print((char)((temp_num / 10000)+0x30));
							temp_num = temp_num % 10000;
							temp_place = 0x01;
						}
						
						if (temp_num >= 1000 || temp_place == 0x01)
						{
							keyboard_print((char)((temp_num / 1000)+0x30));
							temp_num = temp_num % 1000;
							temp_place = 0x01;
						}
	
						if (temp_num >= 100 || temp_place == 0x01)
						{
							keyboard_print((char)((temp_num / 100)+0x30));
							temp_num = temp_num % 100;
							temp_place = 0x01;
						}
	
						if (temp_num >= 10 || temp_place == 0x01)
						{
							keyboard_print((char)((temp_num / 10)+0x30));
							temp_num = temp_num % 10;
							temp_place = 0x01;
						}
		
						keyboard_print((char)(temp_num+0x30));
					}
				}
				else temp_addr++;
			}

		}
		else if (temp_char == 'I' && temp_second == 'N') // input
		{
			while (temp_addr < editor_total)
			{
				if (editor_break()) break;
	
				temp_char = (unsigned char)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
		
				if (temp_char == 0x10) break;
				else if (temp_char == 'A' || temp_char == 'B' || temp_char == 'C' || temp_char == 'D' ||
					temp_char == 'W' || temp_char == 'X' || temp_char == 'Y' || temp_char == 'Z')
				{
					temp_addr++;

					temp_place = temp_char;

					if (temp_place >= 'W')
					{
						keyboard_print(0x0D);
					}

					temp_offset = 0; // up to 16 each
	
					temp_char = (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));

					if (temp_char == '(' || temp_char == '[')
					{
						temp_addr++;
	
						if (basic_number(temp_addr, temp_offset)) break;
					}

					if (temp_place < 'W') // A,B,C,D do a single keystroke
					{
						while (editor_character == 0x00)
						{	
							if (editor_break()) break;
	
							if (editor_character >= 0x20 &&
								editor_character <= 0x7F)
							{
								keyboard_print((char)editor_character);
							}

							if (editor_character < 0x20 && editor_character != 0x0D && 
								editor_character != 0x08 && editor_character != 0x09)
							{
								editor_character = 0x00;
							}
						}

						temp_char = editor_character;

						if (temp_char >= 0x61 && temp_char <= 0x7A) temp_char = (char)(temp_char - 0x20); // only upper case

						x6502_write((unsigned char)((int)(temp_place-'A')*32+(int)((temp_offset)%16)*2), basic_variables, 
							(unsigned char)(temp_char));
			
						x6502_write((unsigned char)((int)(temp_place-'A')*32+(int)((temp_offset)%16)*2+1), basic_variables, 
							(unsigned char)(0x00));

						keyboard_print(0x0D);
					}
					else // W,X,Y,Z do a full number after return
					{
						temp_num = 0;
		
						while (editor_character != 0x0D)
						{	
							if (editor_break()) break;
	
							if (editor_character >= 0x30 &&
								editor_character <= 0x39)
							{
								temp_num *= 10;
								temp_num += (int)(editor_character - 0x30);
	
								keyboard_print((char)editor_character);
							}
							else if (editor_character == 0x08)
							{
								temp_num /= 10;
	
								keyboard_print(0x08);
								keyboard_print(' ');
								keyboard_print(0x08);
							}
							else if (editor_character != 0x0D && editor_character != editor_prompt && 
								editor_character != editor_prompt_caps && editor_character != 0x1B)
							{
								editor_character = 0x00;
							}
						}
	
						x6502_write((unsigned char)((int)(temp_place-'W')*32+128+(int)((temp_offset)%16)*2), basic_variables, 
							(unsigned char)(temp_num%256));
			
						x6502_write((unsigned char)((int)(temp_place-'W')*32+128+(int)((temp_offset)%16)*2+1), basic_variables, 
							(unsigned char)(temp_num/256));

						keyboard_print(0x0D);
					}
					
					break;
				}
				else temp_addr++;
			}
		}
		else if (temp_char == 'I' && temp_second == 'F') // if
		{
			if (basic_number(temp_addr, temp_num)) break;

			while (temp_addr < editor_total)
			{
				if (editor_break()) break;

				temp_char = (unsigned char)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
		
				temp_addr++;

				if (temp_char == '=' || temp_char == '#' || temp_char == '<' || temp_char == '>')
				{
					temp_place = temp_char;

					if (basic_number(temp_addr, temp_compare)) break;

					if ((temp_place == '=' && temp_num == temp_compare) ||
						(temp_place == '#' && temp_num != temp_compare) ||
						(temp_place == '<' && temp_num < temp_compare) ||
						(temp_place == '>' && temp_num > temp_compare))
					{
						temp_char = (unsigned char)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));

						if (temp_char == 'T') // then (optional)
						{
							while (temp_addr < editor_total)
							{
								if (editor_break()) break;
		
								temp_char = (unsigned char)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
			
								if (temp_char == 0x10) break;
								else if (temp_char == 'N')
								{
									temp_addr++;
									break;
								}
								else temp_addr++;
							}

							break;
						}
						else
						{
							break;
						}
					}
					else
					{
						temp_place = temp_char;
				
						if (temp_place == 'T') // then (optional)
						{
							while (temp_addr < editor_total)
							{
								if (editor_break()) break;
		
								temp_char = (unsigned char)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
			
								if (temp_char == 0x10) break;
								else temp_addr++;
							}
	
							break;
						}
						else
						{
							while (temp_addr < editor_total)
							{
								if (editor_break()) break;
	
								temp_char = (unsigned char)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
		
								if (temp_char == 0x10) temp_addr += 3;
								else if (temp_char == 'E') break;
								else temp_addr++;
							}

							break;
						}
					}
				}
			}
		}
		else if (temp_char == 'E') // end
		{
			while (temp_addr < editor_total)
			{
				if (editor_break()) break;

				temp_char = (unsigned char)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
		
				if (temp_char == 0x10) break;
				else temp_addr++;
			}
		}
		else if (temp_char == 'G' && temp_second == 'O') // goto
		{
			while (temp_addr < editor_total)
			{
				if (editor_break()) break;
	
				temp_char = (unsigned char)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
		
				if (temp_char == 'T')
				{
					temp_addr++;
					break;
				}
				else if (temp_char == 'A' || temp_char == 'B' || temp_char == 'C' || temp_char == 'D' ||
					temp_char == 'W' || temp_char == 'X' || temp_char == 'Y' || temp_char == 'Z' ||
					(temp_char >= 0x30 && temp_char <= 0x39))
				{
					break;
				}
				else temp_addr++;
			}

			if (basic_number(temp_addr, temp_num)) break;

			if (temp_num < 0) temp_num *= -1;

			temp_num = temp_num % 32768;

			temp_addr = editor_start;

			temp_line = (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
					
			temp_line *= (unsigned int)256;
	
			temp_line += (unsigned int)x6502_read((unsigned char)((temp_addr+1)&0x00FF), (unsigned char)(((temp_addr+1)&0xFF00)>>8));

			temp_line = temp_line % 32768;

			temp_addr += 2;

			while (temp_line < temp_num && temp_addr < editor_total)
			{
				if (editor_break()) break;

				temp_char = (unsigned char)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
		
				temp_addr++;

				if (temp_char == 0x10)
				{
					temp_line = (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
					
					temp_line *= (unsigned int)256;
	
					temp_line += (unsigned int)x6502_read((unsigned char)((temp_addr+1)&0x00FF), (unsigned char)(((temp_addr+1)&0xFF00)>>8));
	
					temp_line = temp_line % 32768;

					temp_addr += 2;
				}
			}
		}
		else if (temp_char == 'S' && temp_second == 'I') // sing
		{
			if (basic_number(temp_addr, temp_num)) break;

			if (temp_num < 0) temp_num *= -1;

			audio_note(temp_num); // beeps and boops
		}
		else if (temp_char == 'A' || temp_char == 'B' || temp_char == 'C' || temp_char == 'D' ||
			temp_char == 'W' || temp_char == 'X' || temp_char == 'Y' || temp_char == 'Z') // variables
		{
			temp_place = temp_char;

			temp_offset = 0; // up to 16 each
	
			temp_char = (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));

			if (temp_char == '(' || temp_char == '[')
			{
				temp_addr++;

				if (basic_number(temp_addr, temp_offset)) break;
			}

			while (temp_addr < editor_total)
			{
				if (editor_break()) break;
	
				temp_char = (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));

				temp_addr++;

				if (temp_char == '=') // set variable
				{
					if (basic_number(temp_addr, temp_num)) break;

					if (temp_num < 0) // negative
					{
						if (temp_place < 'W') x6502_write((unsigned char)((int)(temp_place-'A')*32+(int)(temp_offset%16)*2), basic_variables, 
							(unsigned char)((-temp_num)%256));
						else x6502_write((unsigned char)((int)(temp_place-'W')*32+128+(int)(temp_offset%16)*2), basic_variables, 
							(unsigned char)((-temp_num)%256));

						if (temp_place < 'W') x6502_write((unsigned char)((int)(temp_place-'A')*32+(int)(temp_offset%16)*2+1), basic_variables, 
							(unsigned char)((-temp_num)/256 + 0x80));
						else x6502_write((unsigned char)((int)(temp_place-'W')*32+128+(int)(temp_offset%16)*2+1), basic_variables, 
							(unsigned char)((-temp_num)/256 + 0x80));
					}
					else
					{
						if (temp_place < 'W') x6502_write((unsigned char)((int)(temp_place-'A')*32+(int)(temp_offset%16)*2), basic_variables, 
							(unsigned char)(temp_num%256));
						else x6502_write((unsigned char)((int)(temp_place-'W')*32+128+(int)(temp_offset%16)*2), basic_variables, 
							(unsigned char)(temp_num%256));

						if (temp_place < 'W') x6502_write((unsigned char)((int)(temp_place-'A')*32+(int)(temp_offset%16)*2+1), basic_variables, 
							(unsigned char)(temp_num/256));
						else x6502_write((unsigned char)((int)(temp_place-'W')*32+128+(int)(temp_offset%16)*2+1), basic_variables, 
							(unsigned char)(temp_num/256));
					}

					break;
				}
				else if (temp_char == 0x10)
				{
					temp_addr--;

					break;
				}
			}
		}
		else if (temp_char == 'M') // monitor
		{
			while (temp_addr < editor_total)
			{
				if (editor_break()) break;
	
				temp_char = (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));

				temp_addr++;

				if (temp_char == '"' || temp_char == '\'')
				{
					temp_place = 0x00;

					for (int i=0; i<command_size; i++) command_string[i] = 0x00;

					while (temp_addr < editor_total)
					{
						if (editor_break()) break;

						temp_char = (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));

						temp_addr++;

						if (temp_char == '"' || temp_char == '\'' || temp_char == 0x10)
						{
							monitor_execute(0, command_size);

							if (temp_char == 0x10) temp_addr--;

							break;
						}
						else
						{
							command_string[temp_place] = temp_char;
							temp_place++;
						}
					}

					break;
				}
			}
		}
		else if (temp_char == 0x10) // line delimiters
		{
			temp_line = (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
					
			temp_line *= (unsigned int)256;
	
			temp_line += (unsigned int)x6502_read((unsigned char)((temp_addr+1)&0x00FF), (unsigned char)(((temp_addr+1)&0xFF00)>>8));

			temp_addr += 2;
		}
	}

	return;
};


/*

#define rogue_room_px 0
#define rogue_room_py 9
#define rogue_room_sx 18
#define rogue_room_sy 27
#define rogue_room_cx 36
#define rogue_room_cy 45
#define rogue_room_vi 54

//unsigned char rogue_room_px[3][3]; // top-left corner of each room
//unsigned char rogue_room_py[3][3];
//unsigned char rogue_room_sx[3][3]; // bottom-right corner of each room
//unsigned char rogue_room_sy[3][3];
//unsigned char rogue_room_cx[3][3]; // a point inside of each room
//unsigned char rogue_room_cy[3][3];
//unsigned char rogue_room_vi[3][3];

#define rogue_enemy_t 64
#define rogue_enemy_x 72
#define rogue_enemy_y 80
#define rogue_enemy_h 88
#define rogue_enemy_r 96
#define rogue_enemy_b 104
#define rogue_item_t 112
#define rogue_item_x 120
#define rogue_item_y 128

//char rogue_enemy_t[8]; // enemy type
//unsigned char rogue_enemy_x[8]; // enemy position
//unsigned char rogue_enemy_y[8];
//char rogue_enemy_h[8]; // enemy health
//char rogue_enemy_r[8]; // value of room below enemy
//char rogue_enemy_b[8]; // enemy behavior, 0 = stand, 1 = random, 2 = chase, 3 = flee
//char rogue_item_t[8]; // item type
//unsigned char rogue_item_x[8]; // item position
//unsigned char rogue_item_y[8];

unsigned char rogue_player_x; // player position
unsigned char rogue_player_y;
char rogue_player_r; // value of room below player
char rogue_player_m; // player max health
char rogue_player_h; // player health
char rogue_player_o; // player food
char rogue_player_p; // player potions
char rogue_player_j; // player scrolls
char rogue_player_w; // player arrows
unsigned char rogue_player_c; // player step count
unsigned char rogue_player_g; // player gold
unsigned char rogue_player_a; // player attack
unsigned char rogue_player_d; // player defense
unsigned char rogue_player_f; // player floor
unsigned char rogue_player_l; // player level
int rogue_player_e; // player experience
unsigned char rogue_player_qv = 0x00; // player holding the amulet
unsigned char rogue_player_qx = 0x00; // and position of stairs hidden
unsigned char rogue_player_qy = 0x00; // until amulet is in hands
const unsigned char rogue_player_qf = 5; // floor amulet is found on 

char rogue_char_player = '@';
char rogue_char_floor = ':';
char rogue_char_door = '+';
char rogue_char_stairs = '<';
char rogue_char_hallway = '#';
char rogue_char_wall_horizontal = '-';
char rogue_char_wall_vertical = '|';
char rogue_char_enemy_fungus = 'F';
char rogue_char_enemy_bat = 'B';
char rogue_char_enemy_goblin = 'G';
char rogue_char_enemy_hobgoblin = 'H';
char rogue_char_enemy_troll = 'T';
char rogue_char_enemy_ogre = 'O';
char rogue_char_item_amulet = '*';
char rogue_char_item_gold = '$';
char rogue_char_item_food = '%';
char rogue_char_item_potion = '!';
char rogue_char_item_scroll = '?';
char rogue_char_item_arrow = '/';
char rogue_char_item_weapon = ')';
char rogue_char_item_armor = ']';

const unsigned char rogue_text_welcome[42] PROGMEM = "Grab the Amulet on Floor 5, then ascend!\n";
const unsigned char rogue_text_died[28] PROGMEM = "You died! Press \\ to exit.\n";
const unsigned char rogue_text_ascended[32] PROGMEM = "You ascended! Press \\ to exit.\n";

const unsigned char rogue_text_direction[16] PROGMEM = "What direction?\n";

const unsigned char rogue_text_levelup[16] PROGMEM = "You leveled up\n";
const unsigned char rogue_text_hungry[16] PROGMEM = "You are hungry\n";

const unsigned char rogue_text_attacked[16] PROGMEM = "You attacked \n";
const unsigned char rogue_text_missed[16] PROGMEM = "You missed \n";
const unsigned char rogue_text_defeated[16] PROGMEM = "You defeated \n";
const unsigned char rogue_text_picked[16] PROGMEM = "You picked up \n";
const unsigned char rogue_text_drank[12] PROGMEM = "You drank \n";
const unsigned char rogue_text_read[12] PROGMEM = "You read \n";
const unsigned char rogue_text_shot[12] PROGMEM = "You shot \n";
const unsigned char rogue_text_struck[12] PROGMEM = " struck you\n";

const unsigned char rogue_text_fungus[8] PROGMEM = "Fungus\n";
const unsigned char rogue_text_bat[4] PROGMEM = "Bat\n";
const unsigned char rogue_text_goblin[8] PROGMEM = "Goblin\n";
const unsigned char rogue_text_hobgoblin[10] PROGMEM = "Hobgoblin\n";
const unsigned char rogue_text_troll[8] PROGMEM = "Troll\n";
const unsigned char rogue_text_ogre[6] PROGMEM = "Ogre\n";

const unsigned char rogue_text_gold[6] PROGMEM = "Gold\n";
const unsigned char rogue_text_food[6] PROGMEM = "Food\n";
const unsigned char rogue_text_potion[8] PROGMEM = "Potion\n";
const unsigned char rogue_text_scroll[8] PROGMEM = "Scroll\n";
const unsigned char rogue_text_arrow[8] PROGMEM = "Arrow\n";
const unsigned char rogue_text_weapon[8] PROGMEM = "Weapon\n";
const unsigned char rogue_text_armor[8] PROGMEM = "Armor\n";

const unsigned char rogue_text_amulet[8] PROGMEM = "Amulet\n";

const unsigned char rogue_text_break[4] PROGMEM = ". \n";

unsigned char rogue_text_position = 0;


void rogue_visibility()
{
	char v, b;

	if (rogue_player_r == rogue_char_floor || rogue_player_r == rogue_char_stairs)
	{
		for (unsigned char i=0; i<3; i++)
		{
			for (unsigned char j=0; j<3; j++)
			{
				if (rogue_player_x >= screen_memory[rogue_room_px + i*3+j] && rogue_player_x <= screen_memory[rogue_room_sx + i*3+j] &&
					rogue_player_y >= screen_memory[rogue_room_py + i*3+j] && rogue_player_y <= screen_memory[rogue_room_sy + i*3+j])
				{
					if (screen_memory[rogue_room_vi + i*3+j] != 0x02)
					{
						screen_memory[rogue_room_vi + i*3+j] = 0x02;

						for (unsigned char x=screen_memory[rogue_room_px + i*3+j]-1; x<=screen_memory[rogue_room_sx + i*3+j]+1; x++)
						{
							for (unsigned char y=screen_memory[rogue_room_py + i*3+j]-1; y<=screen_memory[rogue_room_sy + i*3+j]+1; y++)
							{
								if (y >= 0x00 && x >= 0x00 && x < 0x40 && !(x == rogue_player_x && y == rogue_player_y))
								{
									v = display_receivecharacter(y + 0x20, x) % 128;
		
									if (v != rogue_char_hallway && v > 0x00)
									{
										display_sendcharacter(y, x, v + 128); // visibility
									}
								}
							}
						}
					}

					break;
				}
			}
		}
	
		for (char x=-2; x<=2; x++)
		{
			for (char y=-2; y<=2; y++)
			{
				if ((char)(rogue_player_y + y) >= 0x00 && (char)(rogue_player_x + x) >= 0x00 && (char)(rogue_player_x + x) < 0x40)
				{
					if (!(x < 2 && x > -2 && y < 2 && y > -2))
					{
						v = display_receivecharacter(rogue_player_y + y, rogue_player_x + x) % 128;

						if (v > 0x00)
						{
							v = display_receivecharacter(rogue_player_y + y + 0x20, rogue_player_x + x) % 128;
						
							if (v == rogue_char_hallway) display_sendcharacter(rogue_player_y + y, rogue_player_x + x, v); // visibility
						}
					}
				}
			}
		}
	}
	else
	{
		for (unsigned char i=0; i<3; i++)
		{
			for (unsigned char j=0; j<3; j++)
			{
				if (screen_memory[rogue_room_vi + i*3+j] == 0x02)
				{
					screen_memory[rogue_room_vi + i*3+j] = 0x01;

					for (unsigned char x=screen_memory[rogue_room_px + i*3+j]-1; x<=screen_memory[rogue_room_sx + i*3+j]+1; x++)
					{
						for (unsigned char y=screen_memory[rogue_room_py + i*3+j]-1; y<=screen_memory[rogue_room_sy + i*3+j]+1; y++)
						{
							if (y >= 0x00 && x >= 0x00 && x < 0x40 && !(x == rogue_player_x && y == rogue_player_y))
							{
								v = display_receivecharacter(y + 0x20, x) % 128;
		
								if (v != rogue_char_hallway)
								{
									display_sendcharacter(y, x, v); // visibility
								}
							}
						}
					}
				}
			}
		}		

		for (char x=-2; x<=2; x++)
		{
			for (char y=-2; y<=2; y++)
			{
				if ((char)(rogue_player_y + y) >= 0x00 && (char)(rogue_player_x + x) >= 0x00 && (char)(rogue_player_x + x) < 0x40)
				{
					v = display_receivecharacter(rogue_player_y + y + 0x20, rogue_player_x + x) % 128;

					if (x < 2 && x > -2 && y < 2 && y > -2)
					{
						if (rogue_player_r == rogue_char_hallway)
						{
							if ((v == rogue_char_hallway || v == rogue_char_door) && v > 0x00)
							{
								display_sendcharacter(rogue_player_y + y, rogue_player_x + x, v + 128); // visibility
							}
						}
						else if (v > 0x00)
						{
							display_sendcharacter(rogue_player_y + y, rogue_player_x + x, v + 128); // visibility
						}
					}
					else
					{
						b = 0x00;

						for (unsigned char i=0; i<8; i++)
						{
							if ((char)(rogue_player_x + x) == screen_memory[rogue_enemy_x + i] && (char)(rogue_player_y + y) == screen_memory[rogue_enemy_y + i])
							{
								v = display_receivecharacter(rogue_player_y + y, rogue_player_x + x) % 128;
								
								if (v > 0x00)
								{
									display_sendcharacter(rogue_player_y + y, rogue_player_x + x, screen_memory[rogue_enemy_r + i]); // visibility

									b = 0x01;

									break;
								}
							}
						}

						if (b == 0x00)
						{
							for (unsigned char i=0; i<8; i++)
							{
								if ((char)(rogue_player_x + x) == screen_memory[rogue_item_x + i] && (char)(rogue_player_y + y) == screen_memory[rogue_item_y + i])
								{
									v = display_receivecharacter(rogue_player_y + y, rogue_player_x + x) % 128;
									
									if (v > 0x00)
									{
										display_sendcharacter(rogue_player_y + y, rogue_player_x + x, rogue_char_floor); // visibility
	
										b = 0x01;
	
										break;
									}
								}
							}
						}

						if (b == 0x00)
						{
							v = display_receivecharacter(rogue_player_y + y, rogue_player_x + x) % 128;
						
							display_sendcharacter(rogue_player_y + y, rogue_player_x + x, v); // visibility
						}
					}
				}
			}
		}
	}
};

void rogue_setuprooms()
{
	for (unsigned long i=0; i<millis() % 10000; i++)
	{
		rogue_player_r = random(100);
	}

	for (unsigned int i=0x0000; i<0x2000; i++)
	{
		display_sendpacket(i/256, i%256, 0x00); // clears map and visibility
	}

	unsigned char tx, ty, dx, dy, w, b, qx, qy;
	
	char v;
	
	for (unsigned char i=0; i<3; i++)
	{
		for (unsigned char j=0; j<3; j++)
		{
			screen_memory[rogue_room_px + i*3+j] = random(14) + 2;
			screen_memory[rogue_room_py + i*3+j] = random(4) + 2;

			screen_memory[rogue_room_sx + i*3+j] = random(15 - screen_memory[rogue_room_px + i*3+j]) + 2;
			screen_memory[rogue_room_sy + i*3+j] = random(5 - screen_memory[rogue_room_py + i*3+j]) + 2;

			screen_memory[rogue_room_px + i*3+j] += 20 * i;
			screen_memory[rogue_room_py + i*3+j] += 10 * j;

			screen_memory[rogue_room_sx + i*3+j] += screen_memory[rogue_room_px + i*3+j];
			screen_memory[rogue_room_sy + i*3+j] += screen_memory[rogue_room_py + i*3+j];

			screen_memory[rogue_room_cx + i*3+j] = random(screen_memory[rogue_room_sx + i*3+j] - screen_memory[rogue_room_px + i*3+j]) + screen_memory[rogue_room_px + i*3+j];
			screen_memory[rogue_room_cy + i*3+j] = random(screen_memory[rogue_room_sy + i*3+j] - screen_memory[rogue_room_py + i*3+j]) + screen_memory[rogue_room_py + i*3+j];

			screen_memory[rogue_room_vi + i*3+j] = 0x00;
		}
	}

	tx = random(3);
	ty = random(3);

	qx = screen_memory[rogue_room_cx + tx*3+ty];
	qy = screen_memory[rogue_room_cy + tx*3+ty];

	do
	{
		tx = random(3);
		ty = random(3);

		rogue_player_x = screen_memory[rogue_room_cx + tx*3+ty];
		rogue_player_y = screen_memory[rogue_room_cy + tx*3+ty];
	}
	while (rogue_player_x == qx && rogue_player_y == qy);

	rogue_player_r = rogue_char_floor;

	// this is where you decide which hallways to draw, then just skip the ones not drawn below

	for (unsigned char i=0; i<3; i++)
	{
		for (unsigned char j=0; j<2; j++)
		{
			//if (path_vert[i][j] == 0) continue;

			tx = screen_memory[rogue_room_cx + i*3+j];
			ty = screen_memory[rogue_room_cy + i*3+j];

			dx = screen_memory[rogue_room_cx + i*3+j+1];
			dy = screen_memory[rogue_room_cy + i*3+j+1];
	
			w = random(screen_memory[rogue_room_py + i*3+j+1] - screen_memory[rogue_room_sy + i*3+j] - 4) + screen_memory[rogue_room_sy + i*3+j] + 2;

			b = 0x00;

			while (ty < w)
			{		
				if (b == 0x00 && ty > screen_memory[rogue_room_sy + i*3+j]) 
				{
					b = 0x01;
					display_sendcharacter(ty + 0x20, tx, rogue_char_door);
				}
				else display_sendcharacter(ty + 0x20, tx, rogue_char_hallway);	
				ty++;
			}

			while (tx < dx)
			{
				display_sendcharacter(ty + 0x20, tx, rogue_char_hallway);
				tx++;
			}

			while (tx > dx)
			{
				display_sendcharacter(ty + 0x20, tx, rogue_char_hallway);
				tx--;
			}

			while (ty < dy)
			{
				if (b == 0x01 && ty >= screen_memory[rogue_room_py + i*3+j+1]-1) 
				{
					b = 0x00;
					display_sendcharacter(ty + 0x20, tx, rogue_char_door);
				}
				else display_sendcharacter(ty + 0x20, tx, rogue_char_hallway);
				ty++;
			}
		}	
	}

	for (unsigned char i=0; i<2; i++)
	{
		for (unsigned char j=0; j<3; j++)
		{
			//if (path_horz[i][j] == 0) continue;

			tx = screen_memory[rogue_room_cx + i*3+j];
			ty = screen_memory[rogue_room_cy + i*3+j];

			dx = screen_memory[rogue_room_cx + (i+1)*3+j];
			dy = screen_memory[rogue_room_cy + (i+1)*3+j];
	
			w = random(screen_memory[rogue_room_px + (i+1)*3+j] - screen_memory[rogue_room_sx + i*3+j] - 4) + screen_memory[rogue_room_sx + i*3+j] + 2;

			b = 0x00;

			while (tx < w)
			{		
				if (b == 0x00 && tx > screen_memory[rogue_room_sx + i*3+j]) 
				{
					b = 0x01;
					display_sendcharacter(ty + 0x20, tx, rogue_char_door);
				}
				else display_sendcharacter(ty + 0x20, tx, rogue_char_hallway);	
				tx++;
			}

			while (ty < dy)
			{
				display_sendcharacter(ty + 0x20, tx, rogue_char_hallway);
				ty++;
			}

			while (ty > dy)
			{
				display_sendcharacter(ty + 0x20, tx, rogue_char_hallway);
				ty--;
			}

			while (tx < dx)
			{
				if (b == 0x01 && tx >= screen_memory[rogue_room_px + (i+1)*3+j]-1) 
				{
					b = 0x00;
					display_sendcharacter(ty + 0x20, tx, rogue_char_door);
				}
				else display_sendcharacter(ty + 0x20, tx, rogue_char_hallway);
				tx++;
			}
		}	
	}

	for (unsigned char i=0; i<3; i++)
	{
		for (unsigned char j=0; j<3; j++)
		{
			for (unsigned char x = screen_memory[rogue_room_px + i*3+j]-1; x <= screen_memory[rogue_room_sx + i*3+j]+1; x++)
			{
				for (unsigned char y = screen_memory[rogue_room_py + i*3+j]-1; y <= screen_memory[rogue_room_sy + i*3+j]+1; y++)
				{
					if ((char)x >= screen_memory[rogue_room_px + i*3+j] && x <= screen_memory[rogue_room_sx + i*3+j] &&
						(char)y >= screen_memory[rogue_room_py + i*3+j] && y <= screen_memory[rogue_room_sy + i*3+j])
					{
						display_sendcharacter(y + 0x20, x, rogue_char_floor);
					}
					else
					{
						v = display_receivecharacter(y + 0x20, x);

						if (v != rogue_char_door)
						{
							if (y < screen_memory[rogue_room_py + i*3+j] || y > screen_memory[rogue_room_sy + i*3+j])
							{
								display_sendcharacter(y + 0x20, x, rogue_char_wall_horizontal);
							}
							else
							{
								display_sendcharacter(y + 0x20, x, rogue_char_wall_vertical);
							}
						}
					}
				}
			}
		}
	}

	if (rogue_player_f >= rogue_player_qf)
	{
		rogue_player_qx = qx;
		rogue_player_qy = qy;

		display_sendcharacter(qy + 0x20, qx, rogue_char_floor);
	}
	else display_sendcharacter(qy + 0x20, qx, rogue_char_stairs);

	rogue_visibility();

	display_sendcharacter(rogue_player_y, rogue_player_x, rogue_char_player + 128);

	for (unsigned char i=0; i<8; i++)
	{
		screen_memory[rogue_enemy_x + i] = random(64);
		screen_memory[rogue_enemy_y + i] = random(30);

		b = 0x00;	

		for (unsigned char j=0; j<8; j++)
		{
			if (i == j) continue;
			else if (screen_memory[rogue_enemy_x + i] == screen_memory[rogue_enemy_x + j] && screen_memory[rogue_enemy_y + i] == screen_memory[rogue_enemy_x + j])
			{
				b = 0x01;

				break;
			}
		}
		
		v = display_receivecharacter(screen_memory[rogue_enemy_y + i] + 0x20, screen_memory[rogue_enemy_x + i]);

		if (v != rogue_char_floor || b == 0x01 || (screen_memory[rogue_enemy_x + i] == rogue_player_x && screen_memory[rogue_enemy_y + i] == rogue_player_y))
		{
			i--;
		}
		else
		{
			screen_memory[rogue_enemy_r + i] = v;

			if (rogue_player_f == 1 && rogue_player_qv == 0)
			{
				b = random(10);

				if (b < 4)
				{
					screen_memory[rogue_enemy_t + i] = rogue_char_enemy_fungus;
					screen_memory[rogue_enemy_h + i] = 3;
					screen_memory[rogue_enemy_b + i] = 0; // stand
				}
				else if (b < 8)
				{
					screen_memory[rogue_enemy_t + i] = rogue_char_enemy_bat;
					screen_memory[rogue_enemy_h + i] = 4;
					screen_memory[rogue_enemy_b + i] = 1; // random
				}
				else
				{
					screen_memory[rogue_enemy_t + i] = rogue_char_enemy_goblin;
					screen_memory[rogue_enemy_h + i] = 5;
					screen_memory[rogue_enemy_b + i] = 1; // stand
				}
			}
			else if (rogue_player_f < rogue_player_qf && rogue_player_qv == 0)
			{
				b = random(10);

				if (b < rogue_player_f - 2)
				{
					screen_memory[rogue_enemy_t + i] = rogue_char_enemy_troll;
					screen_memory[rogue_enemy_h + i] = 15;
					screen_memory[rogue_enemy_b + i] = 0; // stand
				}
				else if (b < (rogue_player_f - 1) * 2)
				{
					screen_memory[rogue_enemy_t + i] = rogue_char_enemy_hobgoblin;
					screen_memory[rogue_enemy_h + i] = 10;
					screen_memory[rogue_enemy_b + i] = 1; // random
				}
				else
				{
					screen_memory[rogue_enemy_t + i] = rogue_char_enemy_goblin;
					screen_memory[rogue_enemy_h + i] = 5;
					screen_memory[rogue_enemy_b + i] = 0; // stand
				}
			}
			else
			{
				b = random(10);

				if (b < 6)
				{
					screen_memory[rogue_enemy_t + i] = rogue_char_enemy_ogre;
					screen_memory[rogue_enemy_h + i] = 25;
					screen_memory[rogue_enemy_b + i] = 2; // chase
				}
				else
				{
					screen_memory[rogue_enemy_t + i] = rogue_char_enemy_troll;
					screen_memory[rogue_enemy_h + i] = 15;
					screen_memory[rogue_enemy_b + i] = 0; // stand
				}
			}
		}
	}

	for (unsigned char i=0; i<8; i++)
	{
		screen_memory[rogue_item_x + i] = random(64);
		screen_memory[rogue_item_y + i] = random(30);
		
		v = display_receivecharacter(screen_memory[rogue_item_y + i] + 0x20, screen_memory[rogue_item_x + i]);

		if (v != rogue_char_floor || (screen_memory[rogue_item_x + i] == rogue_player_x && screen_memory[rogue_item_y + i] == rogue_player_y) ||
			(screen_memory[rogue_item_x + i] == qx && screen_memory[rogue_item_y + i] == qy))
		{
			i--;
		}
		else
		{
			if (rogue_player_f >= rogue_player_qf && i == 7)
			{
				screen_memory[rogue_item_t + i] = rogue_char_item_amulet; // amulet
			}
			else
			{
				b = random(100);

				if (b < 30)
				{
					screen_memory[rogue_item_t + i] = rogue_char_item_gold; // gold
				}
				else if (b < 50)
				{
					screen_memory[rogue_item_t + i] = rogue_char_item_arrow; // arrow
				}
				else if (b < 70)
				{
					screen_memory[rogue_item_t + i] = rogue_char_item_food; // food
				}
				else if (b < 80)
				{
					screen_memory[rogue_item_t + i] = rogue_char_item_potion; // potion
				}
				else if (b < 90)
				{
					screen_memory[rogue_item_t + i] = rogue_char_item_scroll; // scroll
				}
				else if (b < 95)
				{
					screen_memory[rogue_item_t + i] = rogue_char_item_weapon; // weapon
				}
				else
				{
					screen_memory[rogue_item_t + i] = rogue_char_item_armor; // armor
				}
			}
		}
	}
};

unsigned char rogue_enemycheck(char n, char tx, char ty)
{
	unsigned char b = 0x01;

	for (unsigned char i=0; i<8; i++)
	{
		if (n == i) continue;

		if ((char)(screen_memory[rogue_enemy_x + n] + tx) == screen_memory[rogue_enemy_x + i] && 
			(char)(screen_memory[rogue_enemy_y + n] + ty) == screen_memory[rogue_enemy_y + i] && 
			(char)screen_memory[rogue_enemy_h + i] > 0)
		{
			b = 0x00;
			
			break;
		}
	}

	return b;
};

unsigned char rogue_randomwalk(char n, char &tx, char &ty)
{
	char v;

	unsigned char b = 0x00;

	tx = random(3) - 1; // random walk
	ty = random(3) - 1;

	if ((char)(screen_memory[rogue_enemy_x + n] + tx) == rogue_player_x && (char)(screen_memory[rogue_enemy_y + n] + ty) == rogue_player_y)
	{
		tx = 0;
		ty = 0;
				
		b = 0x02;
	}	
	else
	{
		v = display_receivecharacter(screen_memory[rogue_enemy_y + n] + ty + 0x20, screen_memory[rogue_enemy_x + n] + tx) % 128;

		if (v > 0x00 && 
			(v == rogue_char_floor || v == rogue_char_stairs || 
			v == rogue_char_hallway || v == rogue_char_door))
		{
			b = 0x01;
		}
	}

	if (b == 0x01)
	{
		b = rogue_enemycheck(n, tx, ty);
	}

	return b;
};

unsigned char rogue_chase(char n, char &tx, char &ty)
{
	char v;

	unsigned char b = 0x00;

	if (rogue_player_x < screen_memory[rogue_enemy_x + n]) tx = -1; // chase the player
	else if (rogue_player_x > screen_memory[rogue_enemy_x + n]) tx = 1;
	else tx = 0;

	if (rogue_player_y < screen_memory[rogue_enemy_y + n]) ty = -1;
	else if (rogue_player_y > screen_memory[rogue_enemy_y + n]) ty = 1;
	else ty = 0;

	if ((char)(screen_memory[rogue_enemy_x + n] + tx) == rogue_player_x && (char)(screen_memory[rogue_enemy_y + n] + ty) == rogue_player_y)
	{
		tx = 0;
		ty = 0;
				
		b = 0x02;
	}	
	else
	{
		b = rogue_enemycheck(n, tx, ty);

		v = display_receivecharacter(screen_memory[rogue_enemy_y + n] + ty + 0x20, screen_memory[rogue_enemy_x + n] + tx) % 128;

		if (v > 0x00 && b == 0x01 &&
			(v == rogue_char_floor || v == rogue_char_stairs || 
			v == rogue_char_hallway || v == rogue_char_door))
		{
			b = 0x01;
		}
		else
		{
			if (rogue_player_x < screen_memory[rogue_enemy_x + n]) tx = -1; // chase the player
			else if (rogue_player_x > screen_memory[rogue_enemy_x + n]) tx = 1;
			else tx = random(3) - 1;

			ty = 0;

			b = rogue_enemycheck(n, tx, ty);

			v = display_receivecharacter(screen_memory[rogue_enemy_y + n] + ty + 0x20, screen_memory[rogue_enemy_x + n] + tx) % 128;
						
			if (v > 0x00 && b == 0x01 &&
				(v == rogue_char_floor || v == rogue_char_stairs || 
				v == rogue_char_hallway || v == rogue_char_door))
			{
				b = 0x01;
			}
			else
			{
				tx = 0;

				if (rogue_player_y < screen_memory[rogue_enemy_y + n]) ty = -1; // chase the player
				else if (rogue_player_y > screen_memory[rogue_enemy_y + n]) ty = 1;
				else ty = random(3) - 1;

				b = rogue_enemycheck(n, tx, ty);

				v = display_receivecharacter(screen_memory[rogue_enemy_y + n] + ty + 0x20, screen_memory[rogue_enemy_x + n] + tx) % 128;

				if (v > 0x00 && b == 0x01 &&
					(v == rogue_char_floor || v == rogue_char_stairs || 
					v == rogue_char_hallway || v == rogue_char_door))
				{
					b = 0x01;
				}
				else
				{
					tx = random(3) - 1;
					ty = random(3) - 1;

					b = rogue_enemycheck(n, tx, ty);
					
					v = display_receivecharacter(screen_memory[rogue_enemy_y + n] + ty + 0x20, screen_memory[rogue_enemy_x + n] + tx) % 128;

					if (v > 0x00 && b == 0x01 &&
						(v == rogue_char_floor || v == rogue_char_stairs || 
						v == rogue_char_hallway || v == rogue_char_door))
					{
						b = 0x01;
					}
					else b = 0x00;
				}
			}
		}
	}

	return b;
};	

unsigned char rogue_flee(char n, char &tx, char &ty)
{
	char v;

	unsigned char b = 0x00;

	if (rogue_player_x < screen_memory[rogue_enemy_x + n]) tx = 1; // flee from the player
	else if (rogue_player_x > screen_memory[rogue_enemy_x + n]) tx = -1;
	else tx = 0;

	if (rogue_player_y < screen_memory[rogue_enemy_y + n]) ty = 1;
	else if (rogue_player_y > screen_memory[rogue_enemy_y + n]) ty = -1;
	else ty = 0;

	b = rogue_enemycheck(n, tx, ty);

	v = display_receivecharacter(screen_memory[rogue_enemy_y + n] + ty + 0x20, screen_memory[rogue_enemy_x + n] + tx) % 128;

	if (v > 0x00 && b == 0x01 &&
		(v == rogue_char_floor || v == rogue_char_stairs || 
		v == rogue_char_hallway || v == rogue_char_door))
	{
		b = 0x01;
	}
	else
	{
		if (rogue_player_x < screen_memory[rogue_enemy_x + n]) tx = 1; // flee from the player
		else if (rogue_player_x > screen_memory[rogue_enemy_x + n]) tx = -1;
		else tx = random(3) - 1;

		ty = random(3) - 1;

		b = rogue_enemycheck(n, tx, ty);

		v = display_receivecharacter(screen_memory[rogue_enemy_y + n] + ty + 0x20, screen_memory[rogue_enemy_x + n] + tx) % 128;
						
		if (v > 0x00 && b == 0x01 &&
			(v == rogue_char_floor || v == rogue_char_stairs || 
			v == rogue_char_hallway || v == rogue_char_door))
		{
			b = 0x01;
		}
		else
		{
			tx = random(3) - 1;

			if (rogue_player_y < screen_memory[rogue_enemy_y + n]) ty = 1; // flee from the player
			else if (rogue_player_y > screen_memory[rogue_enemy_y + n]) ty = -1;
			else ty = random(3) - 1;

			b = rogue_enemycheck(n, tx, ty);

			v = display_receivecharacter(screen_memory[rogue_enemy_y + n] + ty + 0x20, screen_memory[rogue_enemy_x + n] + tx) % 128;

			if (v > 0x00 && b == 0x01 &&
				(v == rogue_char_floor || v == rogue_char_stairs || 
				v == rogue_char_hallway || v == rogue_char_door))
			{
				b = 0x01;
			}
			else b = 0x00;			
		}
	}

	return b;
};	

void rogue_moveenemies()
{
	unsigned char v, b;
	char tx, ty;
	char ax, ay;

	for (unsigned char i=0; i<8; i++)
	{
		if ((char)screen_memory[rogue_enemy_h + i] <= 0) continue;

		tx = 0;
		ty = 0;

		b = 0x00;

		if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_fungus) // fungus
		{
			// does not move, do nothing
		}
		else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_bat) // bat
		{
			if (random(100) < 75)
			{
				b = rogue_randomwalk(i, tx, ty);
			}

			if (b == 0x02) // attack player
			{
				if (random(100) < 30 - 3 * rogue_player_d)
				{
					rogue_player_h -= 1; // get hit by enemy

					rogue_printmessage(rogue_text_bat);
					rogue_printmessage(rogue_text_struck);
					rogue_printmessage(rogue_text_break);
				}

				b = 0x00; // don't really move
			}
		}
		else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_goblin) // goblin
		{
			ax = (char)rogue_player_x - (char)screen_memory[rogue_enemy_x + i];
			if (ax < 0) ax *= -1;

			ay = (char)rogue_player_y - (char)screen_memory[rogue_enemy_y + i];
			if (ay < 0) ay *= -1;

			if (ax <= 5 && ay <= 5 && screen_memory[rogue_enemy_h + i] > rogue_player_l)
			{
				screen_memory[rogue_enemy_b + i] = 2; // chase
			}
			else if (ax <= 10 && ay <= 10 && screen_memory[rogue_enemy_h + i] <= rogue_player_l)
			{
				screen_memory[rogue_enemy_b + i] = 3; // flee
			}
			else if (ax > 10 || ay > 10)
			{
				screen_memory[rogue_enemy_b + i] = 1; // walk
			}

			if (screen_memory[rogue_enemy_b + i] == 1)
			{
				b = rogue_randomwalk(i, tx, ty);
			}
			else if (screen_memory[rogue_enemy_b + i] == 2)
			{
				b = rogue_chase(i, tx, ty);
			}
			else if (screen_memory[rogue_enemy_b + i] == 3)
			{
				b = rogue_flee(i, tx, ty);
			}

			if (b == 0x02) // attack player
			{
				if (random(100) < 65 - 3 * rogue_player_d)
				{
					rogue_player_h -= random(2) + 1; // get hit by enemy

					rogue_printmessage(rogue_text_goblin);
					rogue_printmessage(rogue_text_struck);
					rogue_printmessage(rogue_text_break);
				}
				
				b = 0x00; // don't really move
			}
		}
		else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_hobgoblin) // hobgoblin
		{
			ax = (char)rogue_player_x - (char)screen_memory[rogue_enemy_x + i];
			if (ax < 0) ax *= -1;

			ay = (char)rogue_player_y - (char)screen_memory[rogue_enemy_y + i];
			if (ay < 0) ay *= -1;

			if (ax <= 10 && ay <= 10 && screen_memory[rogue_enemy_h + i] > rogue_player_l-1)
			{
				screen_memory[rogue_enemy_b + i] = 2; // chase
			}
			else if (ax <= 10 && ay <= 10 && screen_memory[rogue_enemy_h + i] <= rogue_player_l-1)
			{
				screen_memory[rogue_enemy_b + i] = 3; // flee
			}
			else if (ax > 10 || ay > 10)
			{
				screen_memory[rogue_enemy_b + i] = 1; // walk
			}

			if (screen_memory[rogue_enemy_b + i] == 1)
			{
				b = rogue_randomwalk(i, tx, ty);
			}
			else if (screen_memory[rogue_enemy_b + i] == 2)
			{
				b = rogue_chase(i, tx, ty);
			}
			else if (screen_memory[rogue_enemy_b + i] == 3)
			{
				b = rogue_flee(i, tx, ty);
			}

			if (b == 0x02) // attack player
			{
				if (random(100) < 75 - 3 * rogue_player_d)
				{
					rogue_player_h -= random(4) + 1; // get hit by enemy

					rogue_printmessage(rogue_text_hobgoblin);
					rogue_printmessage(rogue_text_struck);
					rogue_printmessage(rogue_text_break);
				}
				
				b = 0x00; // don't really move
			}
		}
		else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_troll) // troll
		{
			ax = (char)rogue_player_x - (char)screen_memory[rogue_enemy_x + i];
			if (ax < 0) ax *= -1;

			ay = (char)rogue_player_y - (char)screen_memory[rogue_enemy_y + i];
			if (ay < 0) ay *= -1;

			if (ax <= 10 && ay <= 10)
			{
				screen_memory[rogue_enemy_b + i] = 2; // chase
			}
			else if (ax > 10 || ay > 10)
			{
				screen_memory[rogue_enemy_b + i] = 0; // stand
			}
			
			if (screen_memory[rogue_enemy_b + i] == 2)
			{
				b = rogue_chase(i, tx, ty);
			}
	
			if (b == 0x02) // attack player
			{
				if (random(100) < 85 - 3 * rogue_player_d)
				{
					rogue_player_h -= random(6) + 1; // get hit by enemy

					rogue_printmessage(rogue_text_troll);
					rogue_printmessage(rogue_text_struck);
					rogue_printmessage(rogue_text_break);
				}
				
				b = 0x00; // don't really move
			}

			if (random(100) < 25) // regenerate health
			{	
				screen_memory[rogue_enemy_h + i]++;
	
				if (screen_memory[rogue_enemy_h + i] > 15)
				{
					screen_memory[rogue_enemy_h + i] = 15;
				}
			}
		}

		else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_ogre) // ogre
		{
			if (screen_memory[rogue_enemy_b + i] == 2)
			{
				b = rogue_chase(i, tx, ty);
			}

			if (b == 0x02) // attack player
			{
				if (random(100) < 95 - 3 * rogue_player_d)
				{
					rogue_player_h -= random(10) + 1; // get hit by enemy

					rogue_printmessage(rogue_text_ogre);
					rogue_printmessage(rogue_text_struck);
					rogue_printmessage(rogue_text_break);
				}
				
				b = 0x00; // don't really move
			}
		}
	
		if (b > 0x00)
		{
			v = display_receivecharacter(screen_memory[rogue_enemy_y + i], screen_memory[rogue_enemy_x + i]);

			if ((unsigned char)v > 0x80)
			{
				display_sendcharacter(screen_memory[rogue_enemy_y + i], screen_memory[rogue_enemy_x + i], screen_memory[rogue_enemy_r + i] + 128);
			}
	
			screen_memory[rogue_enemy_x + i] += tx;
			screen_memory[rogue_enemy_y + i] += ty;

			screen_memory[rogue_enemy_r + i] = display_receivecharacter(screen_memory[rogue_enemy_y + i] + 0x20, screen_memory[rogue_enemy_x + i]);
		}
	}
};

void rogue_showenemies()
{
	unsigned char v;

	for (unsigned char i=0; i<8; i++)
	{
		if ((char)screen_memory[rogue_enemy_h + i] <= 0) continue;

		v = display_receivecharacter(screen_memory[rogue_enemy_y + i], screen_memory[rogue_enemy_x + i]);

		if (v > 0x80) // show only in light
		{
			display_sendcharacter(screen_memory[rogue_enemy_y + i], screen_memory[rogue_enemy_x + i], screen_memory[rogue_enemy_t + i] + 128);
		}
	}
};

void rogue_showitems()
{
	unsigned char v;

	for (unsigned char i=0; i<8; i++)
	{
		if (screen_memory[rogue_item_t + i] > 0)
		{
			v = display_receivecharacter(screen_memory[rogue_item_y + i], screen_memory[rogue_item_x + i]);

			if (v > 0x80) // show only in light
			{
				display_sendcharacter(screen_memory[rogue_item_y + i], screen_memory[rogue_item_x + i], screen_memory[rogue_item_t + i] + 128);
			}
		}
	}
};

void rogue_levelup()
{
	while (rogue_player_e >= 5 * rogue_player_l * rogue_player_l)
	{
		rogue_player_e -= 5 * rogue_player_l * rogue_player_l;
	
		rogue_player_l++;

		if (rogue_player_l > 9) rogue_player_l = 9;

		rogue_player_m += 10;
					
		if (rogue_player_m > 99) rogue_player_m = 99;

		rogue_player_h = rogue_player_m;
	
		rogue_printmessage(rogue_text_levelup);
		rogue_printmessage(rogue_text_break);
	}
};

void rogue_moveplayer(unsigned char tx, unsigned char ty)
{
	unsigned char v, b = 0x00;
	unsigned char ex, ey;

	v = display_receivecharacter(rogue_player_y + ty + 0x20, rogue_player_x + tx) % 128;

	if (rogue_player_r == rogue_char_floor || rogue_player_r == rogue_char_stairs)
	{
		if (v == rogue_char_floor || v == rogue_char_door || v == rogue_char_stairs)
		{
			b = 0x01;
		}
	}
	else if (rogue_player_r == rogue_char_hallway)
	{
		if (v == rogue_char_hallway || v == rogue_char_door)
		{
			b = 0x01;
		}
	}
	else if (rogue_player_r == rogue_char_door)
	{
		if (v == rogue_char_floor || v == rogue_char_hallway || v == rogue_char_door || v == rogue_char_stairs)
		{
			b = 0x01;
		}
	}

	if (b == 0x01)
	{
		ex = rogue_player_x + tx;
		ey = rogue_player_y + ty;

		for (unsigned int i=0; i<8; i++)
		{
			if (ex == screen_memory[rogue_enemy_x + i] && ey == screen_memory[rogue_enemy_y + i] && (char)screen_memory[rogue_enemy_h + i] > 0)
			{
				if (random(100) < 50 + 5 * (rogue_player_l / 2))
				{
					screen_memory[rogue_enemy_h + i] -= rogue_player_a + random(rogue_player_a+1); // hit the enemy

					if ((char)screen_memory[rogue_enemy_h + i] > 0)
					{
						rogue_printmessage(rogue_text_attacked);
						if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_fungus) rogue_printmessage(rogue_text_fungus);
						else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_bat) rogue_printmessage(rogue_text_bat);
						else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_goblin) rogue_printmessage(rogue_text_goblin);
						else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_hobgoblin) rogue_printmessage(rogue_text_hobgoblin);
						else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_troll) rogue_printmessage(rogue_text_troll);
						else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_ogre) rogue_printmessage(rogue_text_ogre);
						rogue_printmessage(rogue_text_break);
					}
				}
				else
				{
					rogue_printmessage(rogue_text_missed);
					if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_fungus) rogue_printmessage(rogue_text_fungus);
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_bat) rogue_printmessage(rogue_text_bat);
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_goblin) rogue_printmessage(rogue_text_goblin);
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_hobgoblin) rogue_printmessage(rogue_text_hobgoblin);
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_troll) rogue_printmessage(rogue_text_troll);
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_ogre) rogue_printmessage(rogue_text_ogre);
					rogue_printmessage(rogue_text_break);
				}

				if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_goblin ||
					screen_memory[rogue_enemy_t + i] == rogue_char_enemy_hobgoblin ||
					screen_memory[rogue_enemy_t + i] == rogue_char_enemy_troll ||
					screen_memory[rogue_enemy_t + i] == rogue_char_enemy_ogre)
				{
					screen_memory[rogue_enemy_b + i] = 2; // chase when hit
				}

				if ((char)screen_memory[rogue_enemy_h + i] <= 0)
				{
					rogue_printmessage(rogue_text_defeated);
					if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_fungus) rogue_printmessage(rogue_text_fungus);
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_bat) rogue_printmessage(rogue_text_bat);
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_goblin) rogue_printmessage(rogue_text_goblin);
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_hobgoblin) rogue_printmessage(rogue_text_hobgoblin);
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_troll) rogue_printmessage(rogue_text_troll);
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_ogre) rogue_printmessage(rogue_text_ogre);
					rogue_printmessage(rogue_text_break);

					if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_fungus) rogue_player_e += 1;
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_bat) rogue_player_e += 2;
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_goblin) rogue_player_e += 5;
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_hobgoblin) rogue_player_e += 10;
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_troll) rogue_player_e += 20;
					else if (screen_memory[rogue_enemy_t + i] == rogue_char_enemy_ogre) rogue_player_e += 40;

					rogue_levelup();

					v = display_receivecharacter(screen_memory[rogue_enemy_y + i], screen_memory[rogue_enemy_x + i]);

					if (v > 0x80) display_sendcharacter(screen_memory[rogue_enemy_y + i], screen_memory[rogue_enemy_x + i], screen_memory[rogue_enemy_r + i] + 128);
					else display_sendcharacter(screen_memory[rogue_enemy_y + i], screen_memory[rogue_enemy_x + i], screen_memory[rogue_enemy_r + i]);
				}
	
				b = 0x02; // don't really move
			}

			if (ex == screen_memory[rogue_item_x + i] && ey == screen_memory[rogue_item_y + i] && screen_memory[rogue_item_t + i] > 0)
			{	
				if (screen_memory[rogue_item_t + i] == rogue_char_item_amulet) // amulet
				{
					rogue_player_qv = 0x01;

					rogue_printmessage(rogue_text_picked);
					rogue_printmessage(rogue_text_amulet);
					rogue_printmessage(rogue_text_break);
				}
				else if (screen_memory[rogue_item_t + i] == rogue_char_item_gold) // gold
				{
					rogue_player_g++;

					rogue_printmessage(rogue_text_picked);
					rogue_printmessage(rogue_text_gold);
					rogue_printmessage(rogue_text_break);
				}
				else if (screen_memory[rogue_item_t + i] == rogue_char_item_food) // food
				{
					rogue_player_o += 10;
					if (rogue_player_o > 99) rogue_player_o = 99;

					rogue_printmessage(rogue_text_picked);
					rogue_printmessage(rogue_text_food);
					rogue_printmessage(rogue_text_break);
				}
				else if (screen_memory[rogue_item_t + i] == rogue_char_item_potion) // potion
				{
					rogue_player_p++;
					if (rogue_player_p > 9) rogue_player_p = 9;

					rogue_printmessage(rogue_text_picked);
					rogue_printmessage(rogue_text_potion);
					rogue_printmessage(rogue_text_break);
				}
				else if (screen_memory[rogue_item_t + i] == rogue_char_item_scroll) // scroll
				{
					rogue_player_j++;
					if (rogue_player_j > 9) rogue_player_j = 9;

					rogue_printmessage(rogue_text_picked);
					rogue_printmessage(rogue_text_scroll);
					rogue_printmessage(rogue_text_break);
				}
				else if (screen_memory[rogue_item_t + i] == rogue_char_item_arrow) // arrow
				{
					rogue_player_w++;
					if (rogue_player_w > 99) rogue_player_w = 9;

					rogue_printmessage(rogue_text_picked);
					rogue_printmessage(rogue_text_arrow);
					rogue_printmessage(rogue_text_break);
				}
				else if (screen_memory[rogue_item_t + i] == rogue_char_item_weapon) // weapon
				{
					rogue_player_a++;
					if (rogue_player_a > 9) rogue_player_a = 9;

					rogue_printmessage(rogue_text_picked);
					rogue_printmessage(rogue_text_weapon);
					rogue_printmessage(rogue_text_break);
				}
				else if (screen_memory[rogue_item_t + i] == rogue_char_item_armor) // armor
				{
					rogue_player_d++;
					if (rogue_player_d > 9) rogue_player_d = 9;

					rogue_printmessage(rogue_text_picked);
					rogue_printmessage(rogue_text_armor);
					rogue_printmessage(rogue_text_break);
				}

				screen_memory[rogue_item_t + i] = 0;
			}
		}
	}

	if (b > 0x00)
	{
		display_sendcharacter(rogue_player_y, rogue_player_x, rogue_player_r + 128);

		if (b == 0x01)
		{
			rogue_player_x += tx;
			rogue_player_y += ty;
			rogue_player_r = v;
		}

		rogue_player_c++;

		if (rogue_player_c > 16) // adjust as needed
		{
			rogue_player_c = 0;

			if (rogue_player_h < rogue_player_m) rogue_player_h++;
		
			rogue_player_o--;

			if (rogue_player_o <= 0)
			{
				rogue_player_o = 0;

				rogue_player_h -= 2;

				rogue_printmessage(rogue_text_hungry);
				rogue_printmessage(rogue_text_break);
			}
		}

		rogue_moveenemies();

		rogue_visibility();
	
		display_sendcharacter(rogue_player_y, rogue_player_x, rogue_char_player + 128);

		rogue_showitems();

		rogue_showenemies();
	}
};

void rogue_shootarrow(const char tx, const char ty)
{
	rogue_printmessage(rogue_text_shot);
	rogue_printmessage(rogue_text_arrow);
	rogue_printmessage(rogue_text_break);

	char px = rogue_player_x;
	char py = rogue_player_y;

	unsigned char v;

	for (unsigned char i=0; i<=10; i++) // range
	{
		px += tx;
		py += ty;

		for (unsigned int j=0; j<8; j++)
		{
			if (px == screen_memory[rogue_enemy_x + j] && py == screen_memory[rogue_enemy_y + j] && (char)screen_memory[rogue_enemy_h + j] > 0)
			{
				if (random(100) < 50 + 5 * (rogue_player_l / 2))
				{
					screen_memory[rogue_enemy_h + j] -=  rogue_player_a + random(rogue_player_a+1); // damage

					if ((char)screen_memory[rogue_enemy_h + j] > 0)
					{
						rogue_printmessage(rogue_text_attacked);
						if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_fungus) rogue_printmessage(rogue_text_fungus);
						else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_bat) rogue_printmessage(rogue_text_bat);
						else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_goblin) rogue_printmessage(rogue_text_goblin);
						else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_hobgoblin) rogue_printmessage(rogue_text_hobgoblin);
						else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_troll) rogue_printmessage(rogue_text_troll);
						else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_ogre) rogue_printmessage(rogue_text_ogre);
						rogue_printmessage(rogue_text_break);
					}
				}
				else
				{
					rogue_printmessage(rogue_text_missed);
					if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_fungus) rogue_printmessage(rogue_text_fungus);
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_bat) rogue_printmessage(rogue_text_bat);
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_goblin) rogue_printmessage(rogue_text_goblin);
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_hobgoblin) rogue_printmessage(rogue_text_hobgoblin);
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_troll) rogue_printmessage(rogue_text_troll);
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_ogre) rogue_printmessage(rogue_text_ogre);
					rogue_printmessage(rogue_text_break);
				}

				if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_goblin ||
					screen_memory[rogue_enemy_t + j] == rogue_char_enemy_hobgoblin ||
					screen_memory[rogue_enemy_t + j] == rogue_char_enemy_troll ||
					screen_memory[rogue_enemy_t + j] == rogue_char_enemy_ogre)
				{
					screen_memory[rogue_enemy_b + j] = 2; // chase when hit
				}

				if ((char)screen_memory[rogue_enemy_h + j] <= 0)
				{
					rogue_printmessage(rogue_text_defeated);
					if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_fungus) rogue_printmessage(rogue_text_fungus);
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_bat) rogue_printmessage(rogue_text_bat);
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_goblin) rogue_printmessage(rogue_text_goblin);
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_hobgoblin) rogue_printmessage(rogue_text_hobgoblin);
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_troll) rogue_printmessage(rogue_text_troll);
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_ogre) rogue_printmessage(rogue_text_ogre);
					rogue_printmessage(rogue_text_break);

					if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_fungus) rogue_player_e += 1;
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_bat) rogue_player_e += 2;
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_goblin) rogue_player_e += 5;
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_hobgoblin) rogue_player_e += 10;
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_troll) rogue_player_e += 20;
					else if (screen_memory[rogue_enemy_t + j] == rogue_char_enemy_ogre) rogue_player_e += 40;

					rogue_levelup();

					v = (unsigned char)display_receivecharacter(screen_memory[rogue_enemy_y + j], screen_memory[rogue_enemy_x + j]);

					if (v > 0x00)
					{
						if (v > 0x80) display_sendcharacter(screen_memory[rogue_enemy_y + j], screen_memory[rogue_enemy_x + j], screen_memory[rogue_enemy_r + j] + 128);
						else display_sendcharacter(screen_memory[rogue_enemy_y + j], screen_memory[rogue_enemy_x + j], screen_memory[rogue_enemy_r + j]);
					}
				}

				break;
			}
		}
	}
};

void rogue_clearmessage()
{
	for (int i=0; i<64; i++)
	{
		display_sendcharacter(0x00, i, 0x00);
	}

	rogue_text_position = 0;
};

void rogue_printmessage(const unsigned int place)
{
	for (int i=0; i<64; i++)
	{
		if (rogue_text_position >= 64) break;

		if ((char)pgm_read_byte_near(place + i) == 0 || (char)pgm_read_byte_near(place + i) == '\n') break;
		else
		{
			display_sendcharacter(0x00, rogue_text_position, (char)pgm_read_byte_near(place + i));

			rogue_text_position++;
		}
	}
};

void rogue_printstats()
{
	display_sendcharacter(0x1D, 0x00, 'F');
	display_sendcharacter(0x1D, 0x01, 'l');
	display_sendcharacter(0x1D, 0x02, 'r');
	display_sendcharacter(0x1D, 0x03, ':');
	
	display_sendcharacter(0x1D, 0x04, (char)(rogue_player_f/10 + 0x30));
	display_sendcharacter(0x1D, 0x05, (char)(rogue_player_f%10 + 0x30));

	display_sendcharacter(0x1D, 0x06, ',');
	display_sendcharacter(0x1D, 0x07, ' ');

	display_sendcharacter(0x1D, 0x08, 'H');
	display_sendcharacter(0x1D, 0x09, 'l');
	display_sendcharacter(0x1D, 0x0A, 't');
	display_sendcharacter(0x1D, 0x0B, 'h');
	display_sendcharacter(0x1D, 0x0C, ':');
	
	if (rogue_player_h >= 0)
	{
		display_sendcharacter(0x1D, 0x0D, (char)(rogue_player_h/10 + 0x30));
		display_sendcharacter(0x1D, 0x0E, (char)(rogue_player_h%10 + 0x30));
	}
	else
	{
		display_sendcharacter(0x1D, 0x0D, '0');
		display_sendcharacter(0x1D, 0x0E, '0');
	}

	display_sendcharacter(0x1D, 0x0F, '/');
	display_sendcharacter(0x1D, 0x10, (char)(rogue_player_m/10 + 0x30));
	display_sendcharacter(0x1D, 0x11, (char)(rogue_player_m%10 + 0x30));

	display_sendcharacter(0x1D, 0x12, ',');
	display_sendcharacter(0x1D, 0x13, ' ');

	display_sendcharacter(0x1D, 0x14, 'A');
	display_sendcharacter(0x1D, 0x15, 't');
	display_sendcharacter(0x1D, 0x16, 'k');
	display_sendcharacter(0x1D, 0x17, '/');
	display_sendcharacter(0x1D, 0x18, 'D');
	display_sendcharacter(0x1D, 0x19, 'e');
	display_sendcharacter(0x1D, 0x1A, 'f');
	display_sendcharacter(0x1D, 0x1B, ':');
	
	display_sendcharacter(0x1D, 0x1C, (char)(rogue_player_a%10 + 0x30));
	display_sendcharacter(0x1D, 0x1D, '/');
	display_sendcharacter(0x1D, 0x1E, (char)(rogue_player_d%10 + 0x30));

	display_sendcharacter(0x1D, 0x1F, ',');
	display_sendcharacter(0x1D, 0x20, ' ');

	display_sendcharacter(0x1D, 0x21, rogue_char_item_gold);
	
	display_sendcharacter(0x1D, 0x22, (char)(rogue_player_g/10 + 0x30));
	display_sendcharacter(0x1D, 0x23, (char)(rogue_player_g%10 + 0x30));

	display_sendcharacter(0x1D, 0x24, ' ');

	display_sendcharacter(0x1D, 0x25, rogue_char_item_food);
	
	if (rogue_player_o >= 0)
	{
		display_sendcharacter(0x1D, 0x26, (char)(rogue_player_o/10 + 0x30));
		display_sendcharacter(0x1D, 0x27, (char)(rogue_player_o%10 + 0x30));
	}
	else
	{
		display_sendcharacter(0x1D, 0x26, '0');
		display_sendcharacter(0x1D, 0x27, '0');
	}

	display_sendcharacter(0x1D, 0x28, ' ');

	display_sendcharacter(0x1D, 0x29, rogue_char_item_potion);
	
	display_sendcharacter(0x1D, 0x2A, (char)(rogue_player_p%10 + 0x30));

	display_sendcharacter(0x1D, 0x2B, ' ');

	display_sendcharacter(0x1D, 0x2C, rogue_char_item_scroll);
	
	display_sendcharacter(0x1D, 0x2D, (char)(rogue_player_j%10 + 0x30));

	display_sendcharacter(0x1D, 0x2E, ' ');

	display_sendcharacter(0x1D, 0x2F, rogue_char_item_arrow);
	
	display_sendcharacter(0x1D, 0x30, (char)(rogue_player_w%10 + 0x30));
	
	if (rogue_player_qv > 0x00)
	{
		display_sendcharacter(0x1D, 0x3F, rogue_char_item_amulet);
	}
};


void rogue_mainloop()
{
	unsigned char k;

	char v, b;

	rogue_player_m = 10;
	rogue_player_h = 10;
	rogue_player_o = 50;
	rogue_player_p = 1;
	rogue_player_j = 1;
	rogue_player_w = 3;
	rogue_player_g = 0;
	rogue_player_a = 1;
	rogue_player_d = 1;
	rogue_player_f = 1;
	rogue_player_l = 1;
	rogue_player_e = 0;

	rogue_player_qv = 0x00;
	rogue_player_qx = 0x00;
	rogue_player_qy = 0x00;

	rogue_setuprooms();

	rogue_showitems();

	rogue_showenemies();

	rogue_printstats();

	rogue_printmessage(rogue_text_welcome);

	while (true)
	{
		k = keyboard_character();

		if (k != 0x00)
		{
			rogue_clearmessage();

			if (k == '\\' || k == '|' || k == 0x1B) break;
			else if (k == 'h' || k == 'H' || k == 0x13 || k == 0x34 || k == 0xB4) rogue_moveplayer(-1,0);
			else if (k == 'j' || k == 'J' || k == 0x12 || k == 0x32 || k == 0xB2) rogue_moveplayer(0,1);
			else if (k == 'k' || k == 'K' || k == 0x11 || k == 0x38 || k == 0xB8) rogue_moveplayer(0,-1);
			else if (k == 'l' || k == 'L' || k == 0x14 || k == 0x36 || k == 0xB6) rogue_moveplayer(1,0);
			else if (k == 'y' || k == 'Y' || k == 0x37 || k == 0xB7) rogue_moveplayer(-1,-1);
			else if (k == 'b' || k == 'B' || k == 0x31 || k == 0xB1) rogue_moveplayer(-1,1);
			else if (k == 'u' || k == 'U' || k == 0x39 || k == 0xB9) rogue_moveplayer(1,-1);
			else if (k == 'n' || k == 'N' || k == 0x33 || k == 0xB3) rogue_moveplayer(1,1);
			else if (k == '.' || k == '>' || k == 0x35 || k == 0xB5 || k == 0x7F) rogue_moveplayer(0,0);			
			else if (k == 'p' || k == 'P' || k == 'q' || k == 'Q')
			{
				if (rogue_player_p > 0)
				{
					rogue_player_h += (char)(rogue_player_m / 3) + 1;
					if (rogue_player_h > rogue_player_m) rogue_player_h = rogue_player_m;

					rogue_player_p--;

					rogue_printmessage(rogue_text_drank);
					rogue_printmessage(rogue_text_potion);
					rogue_printmessage(rogue_text_break);
				}
			
				rogue_moveplayer(0,0);
			}
			else if (k == 'r' || k == 'R' || k == 'z' || k == 'Z')
			{
				if (rogue_player_j > 0)
				{
					v = display_receivecharacter(rogue_player_y + 0x20, rogue_player_x) % 128;

					for (char x=0; x<64; x++)
					{
						for (char y=1; y<29; y++)
						{
							v = display_receivecharacter(y + 0x20, x) % 128;
		
							b = display_receivecharacter(y, x) % 128;

							if (b > 0) display_sendcharacter(y, x, v);
						}
					}

					for (unsigned char i=0; i<3; i++)
					{
						for (unsigned char j=0; j<3; j++)
						{
							if (screen_memory[rogue_room_vi + i*3+j] == 0x02)
							{
								screen_memory[rogue_room_vi + i*3+j] = 0x01;
							}
						}
					}

					while (true)
					{
						rogue_player_x = random(64);
						rogue_player_y = random(30);

						v = display_receivecharacter(rogue_player_y + 0x20, rogue_player_x) % 128;

						rogue_player_r = v;

						if (v == rogue_char_floor || v == rogue_char_hallway)
						{
							v = 0x00;

							for (unsigned char i=0; i<8; i++)
							{
								if (rogue_player_x == screen_memory[rogue_enemy_x + i] && rogue_player_y == screen_memory[rogue_enemy_y + i] && 
									(char)screen_memory[rogue_enemy_h + i] > 0)
								{
									v = 0x01;
									break;
								}
							}
		
							if (v == 0x00)
							{
								for (unsigned char i=0; i<8; i++)
								{
									if (rogue_player_x == screen_memory[rogue_item_x + i] && rogue_player_y == screen_memory[rogue_item_y + i] && 
										(char)screen_memory[rogue_enemy_t + i] > 0)
									{
										v = 0x01;
										break;
									}
								}
							}

							if (v == 0x00) break;
						}
					}

					rogue_player_j--;

					rogue_printmessage(rogue_text_read);
					rogue_printmessage(rogue_text_scroll);
					rogue_printmessage(rogue_text_break);
				}
			
				rogue_moveplayer(0,0);
			}
			else if (k == 't' || k == 'T' || k == 'c' || k == 'C')
			{
				if (rogue_player_w > 0)
				{
					rogue_printmessage(rogue_text_direction);

					while (true)
					{
						k = keyboard_character();

						if (k != 0x00)
						{
							rogue_clearmessage();

							if (k == 'h' || k == 'H' || k == 0x13 || k == 0x34 || k == 0xB4) rogue_shootarrow(-1,0);
							else if (k == 'j' || k == 'J' || k == 0x12 || k == 0x32 || k == 0xB2) rogue_shootarrow(0,1);
							else if (k == 'k' || k == 'K' || k == 0x11 || k == 0x38 || k == 0xB8) rogue_shootarrow(0,-1);
							else if (k == 'l' || k == 'L' || k == 0x14 || k == 0x36 || k == 0xB6) rogue_shootarrow(1,0);
							else if (k == 'y' || k == 'Y' || k == 0x37 || k == 0xB7) rogue_shootarrow(-1,-1);
							else if (k == 'b' || k == 'B' || k == 0x31 || k == 0xB1) rogue_shootarrow(-1,1);
							else if (k == 'u' || k == 'U' || k == 0x39 || k == 0xB9) rogue_shootarrow(1,-1);
							else if (k == 'n' || k == 'N' || k == 0x33 || k == 0xB3) rogue_shootarrow(1,1);
							else k = 0x00;

							break;
						}
					}

					if (k != 0x00)
					{
						rogue_player_w--;
					}
				}
			
				rogue_moveplayer(0,0);
			}
			else if ((k == ',' || k == '<' || k == 0x20 || k == 0xA0) && rogue_player_r == rogue_char_stairs)
			{
				if (rogue_player_qv == 0x00)
				{
					rogue_player_f++;
				}
				else
				{
					rogue_player_f--;

					if (rogue_player_f == 0) // won the game!
					{
						rogue_clearmessage();

						rogue_printmessage(rogue_text_ascended);				

						while (true)
						{
							k = keyboard_character();

							if (k != 0x00)
							{
								if (k == '\\' || k == '|' || k == 0x1B) break;
							}
						}

						break;
					}
				}

				rogue_setuprooms();

				rogue_showitems();

				rogue_showenemies();
			}

			if (rogue_player_qv == 0x01)
			{
				rogue_player_qv = 0x02;

				v = display_receivecharacter(rogue_player_qy, rogue_player_qx);
				
				if (v > 0x00)
				{	
					if ((char)v > 128)
					{			
						display_sendcharacter(rogue_player_qy, rogue_player_qx, rogue_char_stairs + 0x80);
					}
					else display_sendcharacter(rogue_player_qy, rogue_player_qx, rogue_char_stairs);
				}

				display_sendcharacter(rogue_player_qy + 0x20, rogue_player_qx, rogue_char_stairs);
			}

			rogue_printstats();

			if (rogue_player_h <= 0) // game over
			{
				rogue_clearmessage();

				rogue_printmessage(rogue_text_died);				

				while (true)
				{
					k = keyboard_character();

					if (k != 0x00)
					{
						if (k == '\\' || k == '|' || k == 0x1B) break;
					}
				}

				break;
			}			
		}
	} 
};

*/


unsigned char editor_execute()
{
	unsigned char temp_place = 0x00;
	unsigned char temp_char = 0x00;
	unsigned char temp_quote = 0x00;

	unsigned int temp_num[3];
	unsigned int temp_addr = 0x0000;
	unsigned int temp_last[2];

	temp_num[0] = 0x0000;

	for (int i=0; i<command_size; i++)
	{
		if (editor_break()) break;
 
		if (command_string[i] == 'Q' && temp_place == 0x00) // quit
		{
			return 0x00;
		}
		//else if (command_string[i] == 'G' && temp_place == 0x00) // roguelike game
		//{
		//	rogue_mainloop();		
		//
		//	keyboard_clearscreen();
		//	keyboard_menu();
		//
		//	return 0x01;
		//}
		else if (command_string[i] == 'H' && temp_place == 0x00) // help
		{
			keyboard_mode = 0x00;
	
			keyboard_print(0x0D); // carriage return

			for (int j=0; j<512; j++)
			{
				if (editor_break()) break;

				temp_char = (char)pgm_read_byte_near(editor_text + j);

				if (temp_char == '_') break;

				if (temp_char == '\n') temp_char = 0x0D;

				keyboard_print(temp_char);
			}

			keyboard_mode = 0x01;

			return 0x01;
		}
		else if (command_string[i] == 'A' && temp_place == 0x00) // align
		{
			if (display_width == 40 && display_left == 0)
			{
				display_width = 40; // 40 or 64
				display_height = 24; // 24 or 15
				display_left = 12; // 12 or 0
				display_top = 1; // 1 or 5
			}
			else if (display_width == 40 && display_left == 12)
			{
				display_width = 64; // 40 or 64
				display_height = 15; // 24 or 15
				display_left = 0; // 12 or 0
				display_top = 1; // 1 or 5
			}
			else if (display_width == 64 && display_top == 1)
			{
				display_width = 64; // 40 or 64
				display_height = 15; // 24 or 15
				display_left = 0; // 12 or 0
				display_top = 5; // 1 or 5
			}
			else
			{
				display_width = 40; // 40 or 64
				display_height = 24; // 24 or 15
				display_left = 0; // 12 or 0
				display_top = 1; // 1 or 5
			}

			keyboard_clearscreen();
			keyboard_menu();

			return 0x01;
		}
		else if (command_string[i] == 'I' && temp_place == 0x00) // invert (colors)
		{
			if (keyboard_invert == 0x80) keyboard_invert = 0x00;
			else keyboard_invert = 0x80;

			keyboard_clearscreen();
			keyboard_menu();

			return 0x01;
		}
		else if (command_string[i] == 'M' && temp_place == 0x00) // slow mon, needs quote marks
		{
			for (int j=i+1; j<command_size; j++)
			{
				if (command_string[j] == '"' || command_string[j] == '\'')
				{
					for (int k=j+1; k<command_size; k++)
					{
						if (command_string[k] == '"' || command_string[k] == '\'')
						{
							monitor_execute(j, k);

							break;
						}
					}

					break;
				}
			}
	
			return 0x01;
		}
		else if (command_string[i] == '$' && temp_place == 0x00) // quick mon, does not need quote marks
		{
			monitor_execute(i, command_size);
		
			return 0x01;
		}
		else if (command_string[i] == 'N' && temp_place == 0x00) // new
		{
			for (unsigned int j=editor_start; j<editor_total; j++)
			{
				x6502_write((unsigned char)(j&0x00FF), (unsigned char)((j&0xFF00)>>8), 0x00);
			}

			editor_total = editor_start + 256;

			return 0x01;
		}
		else if (command_string[i] == 'C' && temp_place == 0x00) // clear
		{
			keyboard_clearscreen();

			keyboard_menu();

			return 0x01;
		}
		else if (command_string[i] == 'D' && temp_place == 0x00) // dir
		{
			temp_place = 0x00;

			temp_num[0] = 0x0000;
			temp_num[1] = 0x0000;

			for (int j=i+1; j<command_size; j++)
			{
				if (command_string[j] == ',')
				{
					temp_place = 0x01;
				}
				else if (command_string[j] == '.')
				{
					temp_place = 0x00;
				}
				else if (command_string[j] >= 0x30 && command_string[j] <= 0x39)
				{
					if (temp_place == 0x00)
					{
						temp_num[0] *= 10;
						temp_num[0] += (unsigned int)(command_string[j] - 0x30);
					}
					else if (temp_place == 0x01)
					{
						temp_num[1] *= 10;
						temp_num[1] += (unsigned int)(command_string[j] - 0x30);
					}
				}
			}

			if (temp_num[1] == 0x0000) temp_num[1] = 0xFFFF;

			temp_quote = 0x00;

			temp_num[2] = 0x0000;

			for (int j=editor_start; j<editor_total; j++)
			{
				if (editor_break()) break;

				if (j != editor_start)
				{
					temp_char = (unsigned char)x6502_read((unsigned char)(j&0x00FF), (unsigned char)((j&0xFF00)>>8));

					if (!(temp_char == 0x10) && temp_num[2] >= temp_num[0] && temp_num[2] <= temp_num[1]) keyboard_print((char)temp_char);
	
					if (temp_char == '"' || temp_char == '\'') temp_quote = 0x01 - temp_quote;

					if (temp_char == 0x10) temp_quote = 0x00;
				}

				if (temp_quote == 0x00 && (j == editor_start || temp_char == 0x10))
				{
					temp_place = 0x00;

					for (int k=j+1; k<editor_total; k++)
					{
						temp_char = (unsigned char)x6502_read((unsigned char)(k&0x00FF), (unsigned char)((k&0xFF00)>>8));

						if (temp_char == 0x10)
						{
							temp_place = 0x01;

							break;
						}
					}

					if (temp_place == 0x00) break;

					if (j != editor_start) j++;

					if (j >= editor_total) break;

					temp_addr = (unsigned int)x6502_read((unsigned char)(j&0x00FF), (unsigned char)((j&0xFF00)>>8));
					
					temp_addr *= (unsigned int)256;

					j++;

					if (j >= editor_total) break;
	
					temp_addr += (unsigned int)x6502_read((unsigned char)(j&0x00FF), (unsigned char)((j&0xFF00)>>8));

					temp_num[2] = temp_addr;

					if (temp_num[2] >= temp_num[0] && temp_num[2] <= temp_num[1]) keyboard_print(0x0D);

					temp_place = 0x00;
					
					if (temp_addr >= 10000)
					{
						temp_place = 0x01;
						if (temp_num[2] >= temp_num[0] && temp_num[2] <= temp_num[1]) keyboard_print((char)((unsigned char)(temp_addr/10000)+0x30));
						temp_addr = temp_addr % 10000;
					}
				
					if (temp_addr >= 1000 || temp_place == 0x01)
					{
						temp_place = 0x01;
						if (temp_num[2] >= temp_num[0] && temp_num[2] <= temp_num[1]) keyboard_print((char)((unsigned char)(temp_addr/1000)+0x30));
						temp_addr = temp_addr % 1000;
					}

					if (temp_addr >= 100 || temp_place == 0x01)
					{
						temp_place = 0x01;
						if (temp_num[2] >= temp_num[0] && temp_num[2] <= temp_num[1]) keyboard_print((char)((unsigned char)(temp_addr/100)+0x30));
						temp_addr = temp_addr % 100;
					}

					if (temp_addr >= 10 || temp_place == 0x01)
					{
						temp_place = 0x01;
						if (temp_num[2] >= temp_num[0] && temp_num[2] <= temp_num[1]) keyboard_print((char)((unsigned char)(temp_addr/10)+0x30));
						temp_addr = temp_addr % 10;
					}

					if (temp_num[2] >= temp_num[0] && temp_num[2] <= temp_num[1]) keyboard_print((char)((unsigned char)(temp_addr/1)+0x30));
				}
				
			}

			return 0x01;
		}
		else if (command_string[i] == 'R' && temp_place == 0x00) // run (basic)
		{
			keyboard_print(0x0D);

			basic_execute();

			return 0x01;
		}
		else if (command_string[i] == 'L') // load
		{
			if (!editor_load())
			{
				keyboard_print(0x0D);
				keyboard_print('?');
			}

			return 0x01;
		}
		else if (command_string[i] == 'S') // save
		{
			if (!editor_save())
			{
				keyboard_print(0x0D);
				keyboard_print('?');
			}

			return 0x01;
		}
		else if (command_string[i] == '"' || command_string[i] == '\'') // skip unused quotes
		{
			for (int j=i+1; j<command_size; j++)
			{
				if (command_string[j] == '"' || command_string[j] == '\'')
				{
					i = j+1;
			
					break;
				}
			}
		}
		else if (command_string[i] >= 0x30 && command_string[i] <= 0x39 && temp_place <= 0x01) // line number
		{
			temp_place = 0x01;

			temp_num[0] *= 10;
			temp_num[0] += (unsigned int)(command_string[i] - 0x30);
			temp_num[0] = temp_num[0] % 32768;
		}
		else if (command_string[i] <= 0x20 || command_string[i] >= 0x7F) // space/blank characters
		{
			// ignore
		}
		else
		{
			// ignore
		}

		if ((command_string[i] < 0x30 || command_string[i] > 0x39) && temp_place == 0x01)
		{
			if (temp_num[0] == 0x0000) return 0x01;

			temp_place = 0x02; // done with line number
			temp_quote = 0x00;

			temp_num[1] = 0x0000;
			temp_num[2] = 0xFFFF;

			temp_addr = editor_start; // starting address
			temp_last[0] = temp_addr;
			temp_last[1] = temp_last[0];

			temp_num[1] = (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
					
			temp_num[1] *= (unsigned int)256;

			temp_addr++;

			temp_num[1] += (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));

			temp_num[1] = temp_num[1] % 32768;

			if (temp_num[1] != 0x0000 && temp_num[1] <= temp_num[0])
			{
				while (temp_addr < editor_total) // ending address
				{
					if (editor_break()) break;

					temp_char = x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));

					if (temp_char == '"' || temp_char == '\'') temp_quote = 0x01 - temp_quote;	

					if (temp_char == 0x10) temp_quote = 0x00;

					if (temp_quote == 0x00 && (temp_char == 0x10)) // delimiter characters
					{
						temp_addr++;

						temp_last[1] = temp_last[0];
						temp_last[0] = temp_addr;
	
						temp_num[2] = temp_num[1];
	
						temp_num[1] = (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));
						
						temp_num[1] *= 256;
	
						temp_addr++;
	
						temp_num[1] += (unsigned int)x6502_read((unsigned char)(temp_addr&0x00FF), (unsigned char)((temp_addr&0xFF00)>>8));

						temp_num[1] = temp_num[1] % 32768;
	
						if (temp_num[1] > temp_num[0])
						{
							break;
						}
					}
					else temp_addr++;
				}
			}		
	
			if (temp_num[2] == temp_num[0]) // delete old one first
			{
				temp_addr = temp_last[0] - temp_last[1];

				for (unsigned int j=temp_last[1]; j<editor_total-temp_addr; j++)
				{
					if (editor_break()) break;

					x6502_write((unsigned char)((j)&0x00FF), (unsigned char)(((j)&0xFF00)>>8),
						x6502_read((unsigned char)((j+temp_addr)&0x00FF), (unsigned char)(((j+temp_addr)&0xFF00)>>8)));
				}

				for (unsigned int j=editor_total-temp_addr; j<editor_total; j++)
				{
					if (editor_break()) break;

					x6502_write((unsigned char)((j)&0x00FF), (unsigned char)(((j)&0xFF00)>>8), 0x00);

					editor_total--;
	
					if (editor_total < editor_start + 256) editor_total = editor_start + 256;
				}

				temp_last[0] = temp_last[1];
			}

			temp_addr = 0x0002;
	
			temp_place = 0x00;
			temp_quote = 0x00;

			for (int j=i; j<command_size; j++)
			{
				if (command_string[j] != 0x00)
				{
					temp_addr++;
	
					if (command_string[j] == '"' || command_string[j] == '\'') temp_quote = 0x01 - temp_quote;

					if (temp_char == 0x10) temp_quote = 0x00;

					if (temp_quote == 0x00 && (command_string[j] == 0x10))
					{
						temp_place = 0x01;

						break;
					}
				}
			}

			if (temp_addr == 0x0002) break;

			if (temp_place == 0x00)
			{
				temp_addr++;
			}

			temp_quote = 0x00;

			for (unsigned int j=editor_total-temp_addr-1; j>=temp_last[0]; j--)
			{
				if (editor_break()) break;
	
				x6502_write((unsigned char)((j+temp_addr)&0x00FF), (unsigned char)(((j+temp_addr)&0xFF00)>>8),
					x6502_read((unsigned char)((j)&0x00FF), (unsigned char)(((j)&0xFF00)>>8)));
			}
	
			x6502_write((unsigned char)((temp_last[0]+0x0000)&0x00FF), (unsigned char)(((temp_last[0]+0x0000)&0xFF00)>>8),
				(unsigned char)(temp_num[0]/256));
	
			x6502_write((unsigned char)((temp_last[0]+0x0001)&0x00FF), (unsigned char)(((temp_last[0]+0x0001)&0xFF00)>>8),
				(unsigned char)(temp_num[0]%256));

			temp_place = (unsigned char)(i);
			temp_char = 0x00;
	
			for (int j=i; j<command_size; j++)
			{
				if (command_string[j] != 0x00)
				{
					if (command_string[j] == 0x10)
					{
						temp_char = 0x01;
					}
						
					x6502_write((unsigned char)((temp_place-i+temp_last[0]+0x0002)&0x00FF), (unsigned char)(((temp_place-i+temp_last[0]+0x0002)&0xFF00)>>8), 
						command_string[j]);

					editor_total++;

					if (editor_total > editor_end) editor_total = editor_end;

					temp_place++;

					if (command_string[j] == '"' || command_string[j] == '\'') temp_quote = 0x01 - temp_quote;

					if (temp_char == 0x10) temp_quote = 0x00;

					if (temp_quote == 0x00 && (command_string[j] == 0x10))
					{
						break;
					}
				}
			}

			if (temp_char == 0x00)
			{
				x6502_write((unsigned char)((temp_place-i+temp_last[0]+0x0002)&0x00FF), (unsigned char)(((temp_place-i+temp_last[0]+0x0002)&0xFF00)>>8), 0x10);

				editor_total++;

				if (editor_total > editor_end) editor_total = editor_end;
			}

			break;
		}
	}

	return 0x01;
};

unsigned char editor_loop()
{
	keyboard_mode = 0x01;

	keyboard_print(0x0D);
	keyboard_print((char)editor_prompt);

	unsigned char key = 0x00;
	unsigned char pos = 0x00;

	while (true)
	{
		key = (unsigned char)keyboard_character();

		if (key == editor_prompt || editor_character == editor_prompt_caps || key == 0x1B || key == editor_prompt+0x80 || key == 0x9B) // slash (or ESC) to break
		{
			for (int i=0; i<command_size; i++) command_string[i] = 0x00;
			pos = 0x00;

			keyboard_print(0x0D);
			keyboard_print((char)editor_prompt);
		}
		else if (key == 0x0D || key == 0x8D) // carriage return
		{
			editor_character = 0x00;

			if (!editor_execute())
			{
				keyboard_mode = 0x00;

				keyboard_print(0x0D);

				return 0x00;
			}

			for (int i=0; i<command_size; i++) command_string[i] = 0x00;
			pos = 0x00;

			keyboard_print(0x0D);
			keyboard_print((char)editor_prompt);
		}
		else if (key == 0x08 || key == 0x88) // backspace
		{
			if (pos > 0x00)
			{
				pos--;

				keyboard_print(0x08);
			}
		}
		else if (key == 0x09 || key == 0x89) // tab
		{
			pos++;
	
			keyboard_print(0x09);
		}
		else if (key != 0x00)
		{
			if (key > 0x60 && key <= 0x7A) key = key - 0x20; // only uppercase

			if (key >= 0x20 && key < 0x7F) // only real characters go here
			{
				command_string[pos%command_size] = key;
				pos++;
			}

			keyboard_print(key);
		}	
	}
};


void setup()
{
	// put your setup code here, to run once:

	Serial.begin(9600);

	if (serial_output)
	{
		Serial.println("");
	}

	for (int i=0; i<512; i++) shared_memory[i] = 0x00;

	display_initialize();
	display_clearmemory();



	// TEMPORARY BELOW!!!



	// END OF TEMPORARY!!!


	editor_checkmemory();

	keyboard_initialize();
	keyboard_clearscreen();
	keyboard_menu();
}

unsigned char loop_key = 0x00;

void loop()
{
	// put your main code here, to run repeatedly:

	if (loop_key == '\\') // editor
	{
		while (keyboard_character()) {}

		for (int i=0; i<command_size; i++) command_string[i] = 0x00;

		loop_key = editor_loop();
	}
	else
	{
		loop_key = keyboard_character();

		if (loop_key != '\\')
		{
			keyboard_print(loop_key); // scratchpad mode
		}
	}	
}




