#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Joystick.h>
#include "structures.h"

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

data current_state;

const int ADDR_BOOT_COUNT = 0;
const int ADDR_JOY_CONFIG = 2;

calibration cal;

uint16_t filtered_X = 512;
uint16_t filtered_Y = 512;
uint16_t filtered_RX = 512;
uint16_t filtered_RY = 512;

uint16_t calibrated_X = 512;
uint16_t calibrated_Y = 512;
uint16_t calibrated_RX = 512;
uint16_t calibrated_RY = 512;

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

  Serial.print(", Cal RX_min: ");
  Serial.print(cal.RX.min);
  Serial.print(", Cal RX_max: ");
  Serial.print(cal.RX.max);
  Serial.println();
}

void calibrate()
{
  delay(1000);
  digitalWrite(17, LOW);
  delay(20);
  digitalWrite(17, HIGH);
  delay(100);
  digitalWrite(17, LOW);
  delay(20);
  digitalWrite(17, HIGH);
  delay(100);
  digitalWrite(17, LOW);
  delay(20);
  digitalWrite(17, HIGH);
  Serial.println("Entering calibration mode...");

  cal.X.min = 1023; cal.X.max = 0;
  cal.Y.min = 1023; cal.Y.max = 0;
  cal.RX.min = 1023; cal.RX.max = 0;
  cal.RY.min = 1023; cal.RY.max = 0;

  unsigned long startTime = millis();
  unsigned long calDuration = 10000;

  while (millis() - startTime <= calDuration)
  {
    receiveData();

    if (current_state.axis_RX < cal.RX.min) cal.RX.min = current_state.axis_RX;
    if (current_state.axis_RX > cal.RX.max) cal.RX.max = current_state.axis_RX;

    if (current_state.axis_RY < cal.RY.min) cal.RY.min = current_state.axis_RY;
    if (current_state.axis_RY > cal.RY.max) cal.RY.max = current_state.axis_RY;
  }

  cal.RX.center = uint16_t((cal.RX.min + cal.RX.max) / 2);
  cal.RY.center = uint16_t((cal.RY.min + cal.RY.max) / 2);

  EEPROM.put(ADDR_JOY_CONFIG, cal);

  digitalWrite(17, LOW);
  delay(20);
  digitalWrite(17, HIGH);
  delay(100);
  digitalWrite(17, LOW);
  delay(20);
  digitalWrite(17, HIGH);
  delay(100);
  digitalWrite(17, LOW);
  delay(20);
  digitalWrite(17, HIGH);
  Serial.println("Calibration data saved");
}

void axis_correction()
{
  int temp_RX = map(current_state.axis_RX, cal.RX.min, cal.RX.max, 0, 1023);
  int temp_RY = map(current_state.axis_RY, cal.RY.min, cal.RY.max, 0, 1023);

  calibrated_RX = constrain(temp_RX, 0, 1023);
  calibrated_RY = constrain(temp_RY, 0, 1023);
}

void filter()
{
  ema(filtered_X, current_state.axis_X);
  ema(filtered_Y, current_state.axis_Y);

  ema(filtered_RX, calibrated_RX);
  ema(filtered_RY, calibrated_RY);
}

void setup() 
{
  pinMode(17, OUTPUT);

  byte bootCount = EEPROM.read(ADDR_BOOT_COUNT);
  bootCount++;
  EEPROM.write(ADDR_BOOT_COUNT, bootCount);

  delay(100);

  Serial.begin(115200);
  delay(1000);
  Serial.println("Serial started");

  Serial.print("Boot count = ");
  Serial.println(bootCount);

  Serial.println("Waiting for I2C...");
  Wire.begin();
  Serial.println("I2C started");

  Serial.println("Starting Joystick...");
  Joystick.begin(false);
  Serial.println("Joystick started");

  if (bootCount >= 3)
  {
    EEPROM.write(ADDR_BOOT_COUNT, 0);

    calibrate();
  }

  digitalWrite(17, LOW);
  delay(250);
  digitalWrite(17, HIGH);
  delay(1000);
  digitalWrite(17, LOW);
  delay(250);
  digitalWrite(17, HIGH);
  delay(1000);
  digitalWrite(17, LOW);
  delay(250);
  digitalWrite(17, HIGH);
  delay(1000);
  digitalWrite(17, LOW);
  delay(250);
  digitalWrite(17, HIGH);
  delay(1000);

  EEPROM.write(ADDR_BOOT_COUNT, 0);
  Serial.println("Boot counter reset");

  EEPROM.get(ADDR_JOY_CONFIG, cal);
  Serial.println("Calibration data loaded");
}

void loop() 
{
  static uint32_t last_frame_time = 0;
  uint32_t time = millis();

  if (time - last_frame_time >= 20)
  {
    last_frame_time = time;

    receiveData();

    axis_correction();
    filter();

    joystick_update();

    //serial_debug();
  }
}
