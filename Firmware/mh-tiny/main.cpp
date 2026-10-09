#include "Arduino.h"
#include "Wire.h"

struct data
{
	uint16_t axis_X;
	uint16_t axis_Y;
	uint16_t axis_RX;
	uint16_t axis_RY;
	uint32_t switches;	//0..15 - matrix buttons, 16..19 - switches, 20..31 - reserved
};

const int RX_LO = 340, RX_HI = 670;
const int RY_LO = 390, RY_HI = 600;

data current_state = {};

const int JOY_RX = A0;
const int JOY_RY = A1;

const int  JOY_X = A6;
const int  JOY_Y = A7;
const int  PIN_LED = 0;

// 3..10 - keyboard matrix, 11 - joystick switch, 12 - tumbler, 13 - slider1, 14 - slider2
const int switch_pin_count = 4;
const int matrix_rows_count = 4;
const int matrix_cols_count = 4;

const int matrix_count = matrix_rows_count * matrix_cols_count;

const int switch_pins[switch_pin_count] = {PIN_PB3, PIN_PB4, PIN_PB5, PIN_PB7};
const int matrix_cols[matrix_cols_count] = {3, 4, 5, 6};
const int matrix_rows[matrix_rows_count] = {7, 8, 9, 10};

void scan_inputs()
{
  data temp_state = {};

	for (int i = 0; i < matrix_rows_count; i++)
	{
		digitalWrite(matrix_rows[i], LOW);
		delayMicroseconds(5);
		for (int j = 0; j < matrix_cols_count; j++)
		{
			temp_state.switches = (temp_state.switches & ~(0x01UL << (j + i * matrix_cols_count)) | ((uint32_t)!digitalRead(matrix_cols[j]) << (j + i * matrix_cols_count)));
		}
		digitalWrite(matrix_rows[i], HIGH);
		delayMicroseconds(5);
	}

	for (int i = 0; i < switch_pin_count; i++)
	{
		temp_state.switches = (temp_state.switches & ~(0x01UL << (i + matrix_count))) | ((uint32_t)!digitalRead(switch_pins[i]) << (i + matrix_count));
	}

	temp_state.axis_X = 1023 - analogRead(JOY_X);
	temp_state.axis_Y = analogRead(JOY_Y);

	int rx = 1023 - analogRead(JOY_RX);
	rx = constrain(rx, RX_LO, RX_HI);
	temp_state.axis_RX = (uint32_t)(rx - RX_LO) * 1023 / (RX_HI - RX_LO);
	//temp_state.axis_RX = rx;

	int ry = analogRead(JOY_RY);
	ry = constrain(ry, RY_LO, RY_HI);
	temp_state.axis_RY = (uint32_t)(ry - RY_LO) * 1023 / (RY_HI - RY_LO);
	//temp_state.axis_RY = ry;
	current_state = temp_state;
}

void sendData()
{
  Wire.write((uint8_t*)&current_state, sizeof(current_state));
}

void setup()
{
  Wire.begin(0x08);
  Wire.onRequest(sendData);

	for (int i = 0; i < switch_pin_count; i++)
	{
		pinMode(switch_pins[i], INPUT_PULLUP);
	}

	for (int i = 0; i < matrix_rows_count; i++)
	{
		pinMode(matrix_rows[i], OUTPUT);
		digitalWrite(matrix_rows[i], HIGH);
	}

	for (int i = 0; i < matrix_cols_count; i++)
	{
		pinMode(matrix_cols[i], INPUT_PULLUP);
	}

  pinMode(PIN_LED, OUTPUT);

  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);
  pinMode(JOY_RX, INPUT);
  pinMode(JOY_RY, INPUT);
}

void loop()
{
  scan_inputs();

  if (current_state.switches != 0)
  {
    digitalWrite(PIN_LED, HIGH);
  }
  else
  {
    digitalWrite(PIN_LED, LOW);
  }

  delay(1);
}