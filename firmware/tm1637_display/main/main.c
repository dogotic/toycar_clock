#include <stdio.h>
#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "tm1637.h"

static const char *TAG = "TM1637";

void app_main(void)
{
    TM1637_Init(GPIO_NUM_13,GPIO_NUM_12,DEFAULT_BIT_DELAY);
    TM1637_setBrightness(0x03,true);
    TM1637_showNumberHexEx(0xBABA,0,false,4,0);

    while (1) {
        ESP_LOGI(TAG, "TIK TAK");
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
