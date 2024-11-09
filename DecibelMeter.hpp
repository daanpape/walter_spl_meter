#ifndef DECIBEL_METER_HPP
#define DECIBEL_METER_HPP

#include <Wire.h>
#include <Arduino.h>

#define DBM_ADDR              0x48

#define DBM_REG_VERSION       0x00
#define DBM_REG_ID3           0x01
#define DBM_REG_ID2           0x02
#define DBM_REG_ID1           0x03
#define DBM_REG_ID0           0x04
#define DBM_REG_SCRATCH       0x05
#define DBM_REG_CONTROL       0x06
#define DBM_REG_TAVG_HIGH     0x07
#define DBM_REG_TAVG_LOW      0x08
#define DBM_REG_RESET         0x09
#define DBM_REG_DECIBEL       0x0A
#define DBM_REG_MIN           0x0B
#define DBM_REG_MAX           0x0C
#define DBM_REG_THR_MIN       0x0D
#define DBM_REG_THR_MAX       0x0E
#define DBM_REG_HISTORY_0     0x14
#define DBM_REG_HISTORY_99    0x77

class DecibelMeter {
public:
    DecibelMeter(uint8_t sda, uint8_t scl, uint32_t frequency);
    bool begin();
    uint8_t getVersion();
    void getID(uint8_t *id);
    uint8_t readDecibel();
    uint8_t readMinDecibel();
    uint8_t readMaxDecibel();
    void resetMinMax();
    void setAveragingInterval(uint16_t intervalMs);

private:
    uint8_t readRegister(uint8_t regaddr);
    void writeRegister(uint8_t regaddr, uint8_t value);
    TwoWire _wire;
    uint8_t _sda;
    uint8_t _scl;
    uint32_t _frequency;
};

#endif