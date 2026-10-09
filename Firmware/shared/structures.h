#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <Arduino.h>

struct data
{
    uint16_t axis_X;
    uint16_t axis_Y;
    uint16_t axis_RX;
    uint16_t axis_RY;
    uint32_t switches;
};

struct axis 
{
    uint16_t min;
    uint16_t center;
    uint16_t max;
};

struct calibration 
{
    axis X;
    axis Y;
    axis RX;
    axis RY;
};

#endif