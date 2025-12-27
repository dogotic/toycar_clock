#ifndef __TM1637__
#define __TM1637__

#include <stdbool.h>
#include <inttypes.h>

#define SEG_A 0b00000001
#define SEG_B 0b00000010
#define SEG_C 0b00000100
#define SEG_D 0b00001000
#define SEG_E 0b00010000
#define SEG_F 0b00100000
#define SEG_G 0b01000000
#define SEG_DP 0b10000000

#define DEFAULT_BIT_DELAY 100

void TM1637_Init(uint8_t pinClk,
                 uint8_t pinDIO,
                 unsigned int bitDelay);
void TM1637_setBrightness(uint8_t brightness, bool on);
void TM1637_setSegments(const uint8_t *segments,
                        uint8_t length,
                        uint8_t pos);
void TM1637_clear();
void TM1637_showNumberDec(int num,
                          bool leading_zero,
                          uint8_t length,
                          uint8_t pos);
void TM1637_showNumberDecEx(int num,
                            uint8_t dots,
                            bool leading_zero,
                            uint8_t length,
                            uint8_t pos);
void TM1637_showNumberHexEx(uint16_t num,
                            uint8_t dots,
                            bool leading_zero,
                            uint8_t length,
                            uint8_t pos);

uint8_t encodeDigit(uint8_t digit);
void bitDelay();
void start();
void stop();
bool writeByte(uint8_t b);
void showDots(uint8_t dots, uint8_t *digits);
void showNumberBaseEx(int8_t base,
                             uint16_t num,
                             uint8_t dots,
                             bool leading_zero,
                             uint8_t length,
                             uint8_t pos);

#endif