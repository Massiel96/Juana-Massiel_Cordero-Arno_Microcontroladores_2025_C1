#include <stdio.h>
#include <stdbool.h>
#include "driver/gpio.h"

// Definición de pines
#define PIN_BOTON_ABRIR  GPIO_NUM_1
#define PIN_BOTON_CERRAR GPIO_NUM_2
#define PIN_BOTON_PARO   GPIO_NUM_3
#define PIN_FOTO_CELDA   GPIO_NUM_4
#define PIN_LED_ESTADO   GPIO_NUM_5
#define PIN_LED_FALLA    GPIO_NUM_6

// Estados del portón
typedef enum {
    CERRADO,
    ABRIENDO,
    ABIERTO,
    CERRANDO,
    PARADO,
    ERROR,
    BLOQUEADO
} EstadoPorton;

// Variables globales
EstadoPorton estadoActual = CERRADO;
bool hayObstaculo = false;
bool fallaDetectada = false;
const int PARPADEO_LENTO = 10;

// Configuración de GPIO
void configurarGPIO() {
    gpio_config_t io_conf = {};

    // Configuración de botones
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_BOTON_ABRIR) | (1ULL << PIN_BOTON_CERRAR) | (1ULL << PIN_BOTON_PARO) | (1ULL << PIN_FOTO_CELDA);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    // Configuración de LEDs
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_LED_ESTADO) | (1ULL << PIN_LED_FALLA);
    gpio_config(&io_conf);
}

// Leer entradas y actualizar el estado
void leerEntradas() {
    static bool estadoAnteriorAbrir = true;
    static bool estadoAnteriorCerrar = true;
    static bool estadoAnteriorParo = true;
    static bool estadoAnteriorFotoCelda = true;

    bool estadoActualAbrir = gpio_get_level(PIN_BOTON_ABRIR);
    bool estadoActualCerrar = gpio_get_level(PIN_BOTON_CERRAR);
    bool estadoActualParo = gpio_get_level(PIN_BOTON_PARO);
    bool estadoActualFotoCelda = gpio_get_level(PIN_FOTO_CELDA);

    if (!estadoActualAbrir && estadoAnteriorAbrir) {
        printf("Botón 'Abrir' presionado.\n");
        if (estadoActual == CERRADO || estadoActual == CERRANDO) {
            estadoActual = ABRIENDO;
        }
    }
    if (!estadoActualCerrar && estadoAnteriorCerrar) {
        printf("Botón 'Cerrar' presionado.\n");
        if (estadoActual == ABIERTO || estadoActual == ABRIENDO) {
            estadoActual = CERRANDO;
        }
    }
    if (!estadoActualParo && estadoAnteriorParo) {
        printf("Botón 'Paro' presionado.\n");
        estadoActual = PARADO;
    }
    if (!estadoActualFotoCelda && estadoAnteriorFotoCelda) {
        printf("Obstáculo detectado.\n");
        hayObstaculo = true;
        estadoActual = ERROR;
    }
    if (estadoActualFotoCelda && !estadoAnteriorFotoCelda) {
        printf("Obstáculo retirado.\n");
        hayObstaculo = false;
        estadoActual = PARADO;
    }

    estadoAnteriorAbrir = estadoActualAbrir;
    estadoAnteriorCerrar = estadoActualCerrar;
    estadoAnteriorParo = estadoActualParo;
    estadoAnteriorFotoCelda = estadoActualFotoCelda;
}

// Manejo de fallas
void gestionarFalla() {
    printf("\u00a1FALLA DETECTADA! Bombillo parpadeando...\n");
    fallaDetectada = true;
}

// Parpadeo del LED de falla
void temporizador50ms() {
    static unsigned int cnt_led_lamp = 0;
    cnt_led_lamp++;
    if (fallaDetectada) {
        if (cnt_led_lamp >= PARPADEO_LENTO) {
            cnt_led_lamp = 0;
            int estadoLed = gpio_get_level(PIN_LED_FALLA);
            gpio_set_level(PIN_LED_FALLA, !estadoLed);
        }
    }
}

int main() {
    configurarGPIO();
    while (1) {
        leerEntradas();
        if (hayObstaculo) {
            gestionarFalla();
        }
        temporizador50ms();
    }
    return 0;
}
