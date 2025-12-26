#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_rom_sys.h"

#include "tm1637.h"

#define TM1637_I2C_COMM1 0x40
#define TM1637_I2C_COMM2 0xC0
#define TM1637_I2C_COMM3 0x80

#define CLK_LOW()   gpio_set_level(m_pinClk, 0)
#define CLK_HIGH()  gpio_set_level(m_pinClk, 1)

#define DIO_LOW()   gpio_set_level(m_pinDIO, 0)
#define DIO_HIGH()  gpio_set_level(m_pinDIO, 1)

#define DIO_READ()  gpio_get_level(m_pinDIO)

static uint8_t m_pinClk;
static uint8_t m_pinDIO;
static uint8_t m_brightness;
static  unsigned int m_bitDelay;

const uint8_t digitToSegment[] = {
    // XGFEDCBA
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
    0b01110111, // A
    0b01111100, // b
    0b00111001, // C
    0b01011110, // d
    0b01111001, // E
    0b01110001  // F
};

static const uint8_t minusSegments = 0b01000000;

void TM1637_Init(uint8_t pinClk,
                 uint8_t pinDIO,
                 unsigned int bitDelay)
{
    // Copy the pin numbers
    m_pinClk = pinClk;
    m_pinDIO = pinDIO;
    m_bitDelay = bitDelay;

    gpio_set_direction(m_pinClk, GPIO_MODE_OUTPUT);
    gpio_set_direction(m_pinDIO, GPIO_MODE_OUTPUT);

    gpio_set_level(m_pinClk, 0);
    gpio_set_level(m_pinDIO, 0);
}

void TM1637_setBrightness(uint8_t brightness, bool on)
{
    m_brightness = (brightness & 0x7) | (on ? 0x08 : 0x00);
}

void TM1637_setSegments(const uint8_t *segments,
                        uint8_t length,
                        uint8_t pos)
{
    // Write COMM1
    start();
    writeByte(TM1637_I2C_COMM1);
    stop();

    // Write COMM2 + first digit address
    start();
    writeByte(TM1637_I2C_COMM2 + (pos & 0x03));

    // Write the data bytes
    for (uint8_t k = 0; k < length; k++)
        writeByte(segments[k]);

    stop();

    // Write COMM3 + brightness
    start();
    writeByte(TM1637_I2C_COMM3 + (m_brightness & 0x0f));
    stop();
}

void TM1637_clear()
{
    uint8_t data[] = {0, 0, 0, 0};
    TM1637_setSegments(data,4,0);
}

void TM1637_showNumberDec(int num,
                          bool leading_zero,
                          uint8_t length,
                          uint8_t pos)
{
    TM1637_showNumberDecEx(num, 0, leading_zero, length, pos);
}

void TM1637_showNumberDecEx(int num,
                            uint8_t dots,
                            bool leading_zero,
                            uint8_t length,
                            uint8_t pos)
{
    showNumberBaseEx(num < 0 ? -10 : 10, num < 0 ? -num : num, dots, leading_zero, length, pos);
}

void TM1637_showNumberHexEx(uint16_t num,
                            uint8_t dots,
                            bool leading_zero,
                            uint8_t length,
                            uint8_t pos)
{
    showNumberBaseEx(16, num, dots, leading_zero, length, pos);
}

void bitDelay()
{
    esp_rom_delay_us(m_bitDelay);
}

void start()
{
    DIO_HIGH();
    CLK_HIGH();
    bitDelay();
    DIO_LOW();
    bitDelay();
    CLK_LOW();
}

void stop()
{
    CLK_LOW();
    bitDelay();
    DIO_LOW();
    bitDelay();
    CLK_HIGH();
    bitDelay();
    DIO_HIGH();
}

bool writeByte(uint8_t b)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        CLK_LOW();
        bitDelay();

        if (b & 0x01)
            DIO_HIGH();
        else
            DIO_LOW();

        bitDelay();
        CLK_HIGH();
        bitDelay();

        b >>= 1;
    }

    // ACK
    CLK_LOW();
    DIO_HIGH();   // pusti DIO
    bitDelay();

    CLK_HIGH();
    bitDelay();
    uint8_t ack = DIO_READ();

    CLK_LOW();
    bitDelay();

    return ack == 0;
}


void showDots(uint8_t dots, uint8_t *digits)
{
    for (int i = 0; i < 4; ++i)
    {
        digits[i] |= (dots & 0x80);
        dots <<= 1;
    }
}

uint8_t encodeDigit(uint8_t digit)
{
    return digitToSegment[digit & 0x0f];
}

void showNumberBaseEx(int8_t base,
                             uint16_t num,
                             uint8_t dots,
                             bool leading_zero,
                             uint8_t length,
                             uint8_t pos)
{
    bool negative = false;
    if (base < 0)
    {
        base = -base;
        negative = true;
    }

    uint8_t digits[4];

    if (num == 0 && !leading_zero)
    {
        // Singular case - take care separately
        for (uint8_t i = 0; i < (length - 1); i++)
            digits[i] = 0;
        digits[length - 1] = encodeDigit(0);
    }
    else
    {
        // uint8_t i = length-1;
        // if (negative) {
        //	// Negative number, show the minus sign
        //     digits[i] = minusSegments;
        //	i--;
        // }

        for (int i = length - 1; i >= 0; --i)
        {
            uint8_t digit = num % base;

            if (digit == 0 && num == 0 && leading_zero == false)
                // Leading zero is blank
                digits[i] = 0;
            else
                digits[i] = encodeDigit(digit);

            if (digit == 0 && num == 0 && negative)
            {
                digits[i] = minusSegments;
                negative = false;
            }

            num /= base;
        }
    }

    if (dots != 0)
    {
        showDots(dots, digits);
    }

    TM1637_setSegments(digits, length, pos);
}
