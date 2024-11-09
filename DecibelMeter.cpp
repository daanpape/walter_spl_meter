#include "DecibelMeter.hpp"

DecibelMeter::DecibelMeter(uint8_t sda, uint8_t scl, uint32_t frequency)
    : _wire(0), _sda(sda), _scl(scl), _frequency(frequency) {}

bool DecibelMeter::begin() {
    _wire.begin(_sda, _scl, _frequency);
    return true;
}

uint8_t DecibelMeter::getVersion() {
    return readRegister(DBM_REG_VERSION);
}

void DecibelMeter::getID(uint8_t *id) {
    id[0] = readRegister(DBM_REG_ID3);
    id[1] = readRegister(DBM_REG_ID2);
    id[2] = readRegister(DBM_REG_ID1);
    id[3] = readRegister(DBM_REG_ID0);
}

uint8_t DecibelMeter::readDecibel() {
    return readRegister(DBM_REG_DECIBEL);
}

uint8_t DecibelMeter::readMinDecibel() {
    return readRegister(DBM_REG_MIN);
}

uint8_t DecibelMeter::readMaxDecibel() {
    return readRegister(DBM_REG_MAX);
}

void DecibelMeter::setAveragingInterval(uint16_t intervalMs) {
    uint8_t highByte = (intervalMs >> 8) & 0xFF;
    uint8_t lowByte = intervalMs & 0xFF;
    writeRegister(DBM_REG_TAVG_HIGH, highByte);
    writeRegister(DBM_REG_TAVG_LOW, lowByte);
}

void DecibelMeter::resetMinMax() {
    writeRegister(DBM_REG_RESET, 0b00000010);  // Clear MIN/MAX
}

uint8_t DecibelMeter::readRegister(uint8_t regaddr) {
    _wire.beginTransmission(DBM_ADDR);
    _wire.write(regaddr);
    _wire.endTransmission();
    _wire.requestFrom(DBM_ADDR, 1);
    delay(10);
    return _wire.read();
}

void DecibelMeter::writeRegister(uint8_t regaddr, uint8_t value) {
    _wire.beginTransmission(DBM_ADDR);
    _wire.write(regaddr);
    _wire.write(value);
    _wire.endTransmission();
}