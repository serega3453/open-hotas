#include <Arduino.h>
#include <Wire.h>
#include <Joystick.h>

Joystick_ Joystick
(
  JOYSTICK_DEFAULT_REPORT_ID,
  JOYSTICK_TYPE_JOYSTICK,
  20, 0,                 // 20 switches, 0 hat
  true, true, false,     // X, Y, Z
  true, true, false,  // Rx, Ry, Rz
  false, false, false,  // rudder, throttle, accelerator
  false, false           // brake, steering
);

struct data
{
    uint16_t axis_X;
    uint16_t axis_Y;
    uint16_t axis_RX;
    uint16_t axis_RY;
    uint32_t switches;
};

data current_state;

uint16_t filtered_X = 512;
uint16_t filtered_Y = 512;
uint16_t filtered_RX = 512;
uint16_t filtered_RY = 512;

void receiveData()
{
  Wire.requestFrom(0x08, sizeof(current_state));

  if (Wire.available() == sizeof(current_state))
  {
    Wire.readBytes((uint8_t*)&current_state, sizeof(current_state));
  }
}

static void ema(uint16_t &f, uint16_t raw)
{
  int16_t d = raw - f;
  f += (d + (d > 0 ? 2 : -2)) / 3;
}

void filter(uint16_t raw_X, uint16_t raw_Y)
{
  ema(filtered_X, raw_X);
  ema(filtered_Y, raw_Y);
}

void filterR(uint16_t raw_X, uint16_t raw_Y)
{
  ema(filtered_RX, raw_X);
  ema(filtered_RY, raw_Y);
}

void joystick_update()
{
  Joystick.setXAxis(filtered_X);
  Joystick.setYAxis(filtered_Y);
  Joystick.setRxAxis(filtered_RX);
  Joystick.setRyAxis(filtered_RY);

  static uint32_t last_switches = 0xFFFFFFFF;

  if (current_state.switches != last_switches)
  {
    for (int i = 0; i < 20; i++)
      Joystick.setButton(i, bitRead(current_state.switches, i));

    last_switches = current_state.switches;
  }

  Joystick.sendState();
}

void serial_debug()
{
  Serial.print("X: ");
  Serial.print(filtered_X);
  Serial.print(", Y: ");
  Serial.print(filtered_Y);
  Serial.print(", RX: ");
  Serial.print(filtered_RX);
  Serial.print(", RY: ");
  Serial.print(filtered_RY);
  Serial.print(", Switches: ");
  for (int i = 31; i >= 0; i--)
    Serial.print((current_state.switches >> i) & 1);

  Serial.println();
}

void setup() 
{
  Serial.begin(115200);
  //while(!Serial);
  Serial.println("Serial started");
  Serial.println("Waiting for I2C...");
  Wire.begin();
  Serial.println("I2C started");
  Serial.println("Starting Joystick...");
  Joystick.begin(false);
  Serial.println("Joystick started");
}

void loop() 
{
  static uint32_t last_frame_time = 0;
  uint32_t time = millis();

  if (time - last_frame_time >= 20)
  {
    last_frame_time = time;

    receiveData();

    filter(current_state.axis_X, current_state.axis_Y);
    filterR(current_state.axis_RX, current_state.axis_RY);

    joystick_update();

    //serial_debug();
  }
}
