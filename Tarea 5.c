#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

// Definimos los pines del LED y del bot�n
#define LED_PIN 2       // LED en el pin 2
#define BUTTON_PIN 6    // Bot�n en el pin 6

// Timers
TimerHandle_t xTimerBlink;   // Timer para el parpadeo del LED
TickType_t buttonPressTime = 0; // Tiempo que el bot�n estuvo presionado
TaskHandle_t xTaskHandle = NULL; // Para manejar la tarea del LED

// **Interrupci�n del bot�n**: detecta cu�ndo se presiona y se suelta
void IRAM_ATTR buttonISR(void *arg) {
    static TickType_t startTime = 0;

    if (gpio_get_level(BUTTON_PIN) == 1) {
        // Si el bot�n se presiona, guardamos el tiempo y encendemos el LED
        startTime = xTaskGetTickCount();
        gpio_set_level(LED_PIN, 1);
    } else {
        // Si el bot�n se suelta, calculamos cu�nto tiempo estuvo presionado
        buttonPressTime = xTaskGetTickCount() - startTime;
        gpio_set_level(LED_PIN, 0); // Apagar LED

        // Enviar el tiempo a la tarea para que haga parpadear el LED
        xTaskNotifyFromISR(xTaskHandle, buttonPressTime, eSetValueWithOverwrite, NULL);
    }
}

// **Callback del timer**: hace parpadear el LED
void vTimerBlinkCallback(TimerHandle_t xTimer) {
    static bool ledState = false;
    ledState = !ledState; // Alternar LED (encender/apagar)
    gpio_set_level(LED_PIN, ledState);
}

// **Tarea del LED**: recibe el tiempo del bot�n y hace parpadear el LED
void vTaskLedBlink(void *pvParameters) {
    TickType_t receivedTime;
    while (1) {
        if (xTaskNotifyWait(0, 0, &receivedTime, portMAX_DELAY) == pdTRUE) {
            xTimerChangePeriod(xTimerBlink, pdMS_TO_TICKS(200), 0); // Parpadeo cada 200ms
            xTimerStart(xTimerBlink, 0); // Iniciar el timer
            vTaskDelay(receivedTime); // Esperar el tiempo que el bot�n estuvo presionado
            xTimerStop(xTimerBlink, 0); // Detener el parpadeo
            gpio_set_level(LED_PIN, 0); // Asegurar que el LED quede apagado
        }
    }
}

// **Funci�n principal**: configura los pines y crea las tareas/timers
void app_main() {
    // Configurar LED y bot�n
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN) | (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf);

    // Crear el timer para el parpadeo
    xTimerBlink = xTimerCreate("BlinkTimer", pdMS_TO_TICKS(200), pdTRUE, NULL, vTimerBlinkCallback);

    // Crear la tarea que manejar� el parpadeo
    xTaskCreate(vTaskLedBlink, "TaskLedBlink", 2048, NULL, 5, &xTaskHandle);

    // Configurar la interrupci�n del bot�n
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, buttonISR, NULL);
}




