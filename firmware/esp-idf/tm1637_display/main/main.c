#include <stdio.h>
#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "driver/gpio.h"

#include "tm1637.h"

/* ===== TYPES ===== */
typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} clock_time_t;

/* ===== GLOBALS ===== */
QueueHandle_t clockQueue = NULL;
TimerHandle_t clockTimer = NULL;

/* ===== TIMER CALLBACK ===== */
static void ClockTimerCallback(TimerHandle_t xTimer)
{
    static clock_time_t time = { 12, 0, 0};

    time.seconds++;

    if (time.seconds >= 60) {
        time.seconds = 0;
        time.minutes++;
    }
    if (time.minutes >= 60) {
        time.minutes = 0;
        time.hours++;
    }
    if (time.hours >= 24) {
        time.hours = 0;
    }

    xQueueOverwrite(clockQueue, &time);
}

/* ===== DISPLAY TASK ===== */
void DisplayTask(void *pvParameters)
{
    clock_time_t time;
    uint8_t colon = 0x1;

    while (1) {
        if (xQueueReceive(clockQueue, &time, portMAX_DELAY)) {
            // Formatiraj broj za prikaz HHMM
            int v = time.hours * 100 + time.minutes;

            // Blink dvotočke svake sekunde
            colon ^= 0x1;
            uint8_t colonMask = colon ? 0x80 : 0x00;  // 0x80 pali kolon
            ESP_LOGI("DISPLAY TASK","colon %d colonMask = %d",colon,colonMask);

            // Pokaži broj s dvotočkom
            TM1637_showNumberDecEx(v, colonMask, true, 4, 0);
        }
    }
}


/* ===== ALARM TASK ===== */
void AlarmTask(void *pvParameters)
{
    clock_time_t time;

    while (1) {
        if (xQueueReceive(clockQueue, &time, portMAX_DELAY)) {
            if (time.hours == 12 &&
                time.minutes == 1 &&
                time.seconds == 0) {
                ESP_LOGW("ALARM", "⏰ ALARM!");
            }
        }
    }
}

/* ===== MAIN ===== */
void app_main(void)
{
    TM1637_Init(GPIO_NUM_13, GPIO_NUM_12, DEFAULT_BIT_DELAY);
    TM1637_setBrightness(0x03, true);
    clockQueue = xQueueCreate(1, sizeof(clock_time_t));
    configASSERT(clockQueue);

    clockTimer = xTimerCreate(
        "ClockTimer",
        pdMS_TO_TICKS(1000),
        pdTRUE,
        NULL,
        ClockTimerCallback
    );
    configASSERT(clockTimer);

    xTimerStart(clockTimer, 0);

    xTaskCreate(DisplayTask, "DisplayTask", 4096, NULL, 1, NULL);
    xTaskCreate(AlarmTask,   "AlarmTask",   4096, NULL, 1, NULL);
}
