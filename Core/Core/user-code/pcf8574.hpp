#ifndef __PCF8574_HPP
#define __PCF8574_HPP

#include "prj_header.hpp"

extern "C"
{
    uint8_t PCF8574_Init(void);
    uint8_t PCF8574_ReadOneByte(void);
    void PCF8574_WriteOneByte(uint8_t DataToWrite);
    void PCF8574_WriteBit(uint8_t bit, uint8_t sta);
    uint8_t PCF8574_ReadBit(uint8_t bit);
}

#endif