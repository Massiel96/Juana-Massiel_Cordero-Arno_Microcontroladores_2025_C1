#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define DEBUG 1  // Activar (1) o desactivar (0) los mensajes de depuración

// Definición de pines GPIO
#define PIN_BOTON_ABRIR     GPIO_NUM_32
#define PIN_BOTON_CERRAR    GPIO_NUM_33
#define PIN_BOTON_PARO      GPIO_NUM_34
#define PIN_BOTON_OBSTACULO GPIO_NUM_35
#define PIN_LED_ESTADO      GPIO_NUM_25
#define PIN_LED_FALLA       GPIO_NUM_26

// Estados de la máquina de estados
typedef enum {
    ESPERA,
    ABRIENDO,
    ABIERTA,
    CERRANDO,
    CERRADA,
    FALLA,
    OBSTACULO,
    PARO
} Estado;

// Variables globales
Estado estadoActual = ESPERA;
bool hayObstaculo = false;
bool fallaDetectada = false;
uint64_t startTime = 0;

// Funciones prototipo
void setupPines();
void leerBotones();
void manejarEstado();
void manejarFalla();
void manejarObstaculo();
void parpadearBombillo();

void app_main() {
    setupPines();

    while (1) {
        leerBotones();
        manejarEstado();
        vTaskDelay(100 / portTICK_PERIOD_MS); // Pequeña pausa entre iteraciones
    }
}

void setupPines() {
    gpio_config_t io_conf;

    // Configuración de botones como entrada con pull-up
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    io_conf.pin_bit_mask = (1ULL << PIN_BOTON_ABRIR) | (1ULL << PIN_BOTON_CERRAR) | (1ULL << PIN_BOTON_PARO) | (1ULL << PIN_BOTON_OBSTACULO);
    gpio_config(&io_conf);

    // Configuración de LEDs como salida
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    io_conf.pin_bit_mask = (1ULL << PIN_LED_ESTADO) | (1ULL << PIN_LED_FALLA);
    gpio_config(&io_conf);
}

void leerBotones() {
    static bool lastAbrirState = true;
    static bool lastCerrarState = true;
    static bool lastParoState = true;
    static bool lastObstaculoState = true;

    bool currentAbrirState = gpio_get_level(PIN_BOTON_ABRIR);
    bool currentCerrarState = gpio_get_level(PIN_BOTON_CERRAR);
    bool currentParoState = gpio_get_level(PIN_BOTON_PARO);
    bool currentObstaculoState = gpio_get_level(PIN_BOTON_OBSTACULO);

    if (currentAbrirState == false && lastAbrirState == true) {
        #if DEBUG
        printf("Botón 'Abrir' presionado.\n");
        #endif
        estadoActual = ABRIENDO;
        startTime = esp_timer_get_time();
    }

    if (currentCerrarState == false && lastCerrarState == true) {
        #if DEBUG
        printf("Botón 'Cerrar' presionado.\n");
        #endif
        estadoActual = CERRANDO;
        startTime = esp_timer_get_time();
    }

    if (currentParoState == false && lastParoState == true) {
        #if DEBUG
        printf("Botón 'Paro' presionado. Sistema detenido.\n");
        #endif
        estadoActual = PARO;
    }

    if (currentObstaculoState == false && lastObstaculoState == true) {
        #if DEBUG
        printf("Obstáculo detectado!\n");
        #endif
        estadoActual = OBSTACULO;
    }

    lastAbrirState = currentAbrirState;
    lastCerrarState = currentCerrarState;
    lastParoState = currentParoState;
    lastObstaculoState = currentObstaculoState;
}

void manejarEstado() {
    switch (estadoActual) {
        case ESPERA:
            #if DEBUG
            printf("Estado: ESPERA\n");
            #endif
            gpio_set_level(PIN_LED_ESTADO, 0);
            break;

        case ABRIENDO:
            #if DEBUG
            printf("Estado: ABRIENDO\n");
            #endif
            gpio_set_level(PIN_LED_ESTADO, 1);
            // Simulación de apertura
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            estadoActual = ABIERTA;
            break;

        case ABIERTA:
            #if DEBUG
            printf("Estado: ABIERTA\n");
            #endif
            gpio_set_level(PIN_LED_ESTADO, 1);
            break;

        case CERRANDO:
            #if DEBUG
            printf("Estado: CERRANDO\n");
            #endif
            gpio_set_level(PIN_LED_ESTADO, 1);
            // Simulación de cierre
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            estadoActual = CERRADA;
            break;

        case CERRADA:
            #if DEBUG
            printf("Estado: CERRADA\n");
            #endif
            gpio_set_level(PIN_LED_ESTADO, 1);
            break;

        case FALLA:
            #if DEBUG
            printf("Estado: FALLA\n");
            #endif
            gpio_set_level(PIN_LED_FALLA, 1);
            manejarFalla();
            break;

        case OBSTACULO:
            manejarObstaculo();
            break;

        case PARO:
            #if DEBUG
            printf("Estado: PARO\n");
            #endif
            gpio_set_level(PIN_LED_ESTADO, 0);
            break;

        default:
            break;
    }
}

void manejarFalla() {
    #if DEBUG
    printf("¡FALLA DETECTADA! Bombillo parpadeando...\n");
    #endif
    parpadearBombillo();
    fallaDetectada = false;
    estadoActual = ESPERA;
}

void manejarObstaculo() {
    #if DEBUG
    printf("¡Obstáculo en la puerta! Deteniendo movimiento.\n");
    #endif
    estadoActual = PARO;
}

void parpadearBombillo() {
    for (int i = 0; i < 5; i++) {
        gpio_set_level(PIN_LED_FALLA, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(PIN_LED_FALLA, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
