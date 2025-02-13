#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "driver/gpio.h"

// Definir DEBUG para activar logs
#define DEBUG 1

// Estados del portón
typedef enum {
    CERRADO,
    ABRIENDO,
    ABIERTO,
    CERRANDO,
    PARADO,
    ERROR
} EstadoPorton;

static EstadoPorton estadoActual = CERRADO;

// Pines de control
#define PIN_MOTOR1   5
#define PIN_MOTOR2   18
#define PIN_SENSOR_A 34  // Sensor de apertura
#define PIN_SENSOR_C 35  // Sensor de cierre
#define PIN_BUZZER   25
#define PIN_LUZ      26

// Temporizador
static TimerHandle_t xTimer;

//  controlar el motor Fun
void controlMotor(bool dir, bool encender) {
    gpio_set_level(PIN_MOTOR1, encender ? dir : 0);
    gpio_set_level(PIN_MOTOR2, encender ? !dir : 0);
}

// Máquina de estados
void actualizarEstado(TimerHandle_t xTimer) {
    switch (estadoActual) {
        case CERRADO:
            if (gpio_get_level(PIN_SENSOR_A) == 1) {
                estadoActual = ABRIENDO;
            }
            break;

        case ABRIENDO:
            controlMotor(true, true);
            if (gpio_get_level(PIN_SENSOR_A) == 0) {
                estadoActual = ABIERTO;
                controlMotor(true, false);
            }
            break;

        case ABIERTO:
            if (gpio_get_level(PIN_SENSOR_C) == 1) {
                estadoActual = CERRANDO;
            }
            break;

        case CERRANDO:
            controlMotor(false, true);
            if (gpio_get_level(PIN_SENSOR_C) == 0) {
                estadoActual = CERRADO;
                controlMotor(false, false);
            }
            break;

        case PARADO:
            controlMotor(true, false);
            break;

        case ERROR:
            gpio_set_level(PIN_BUZZER, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(PIN_BUZZER, 0);
            estadoActual = CERRADO;
            break;
    }

#if DEBUG
    ESP_LOGI("PORTON", "Estado: %d", estadoActual);
#endif
}

// Configuración inicial
void setup() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_MOTOR1) | (1ULL << PIN_MOTOR2) |
                        (1ULL << PIN_BUZZER) | (1ULL << PIN_LUZ),
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&io_conf);

    gpio_config_t sensor_conf = {
        .pin_bit_mask = (1ULL << PIN_SENSOR_A) | (1ULL << PIN_SENSOR_C),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&sensor_conf);

    // temporizador de 50ms
    xTimer = xTimerCreate("TimerPorton", pdMS_TO_TICKS(50), pdTRUE, 0, actualizarEstado);
    if (xTimer != NULL) {
        xTimerStart(xTimer, 0);
    }
}

// principal (necesaria para ESP-IDF)
void app_main() {
    setup();
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Solo para mantener el loop activo
    }
}
