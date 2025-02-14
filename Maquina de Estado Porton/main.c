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

// Definición de estados
typedef enum {
    REPOSO,
    ABRIENDO,
    CERRANDO,
    PARADA
} EstadoPorton;

// Variables globales
EstadoPorton estadoActual = REPOSO;
bool hayObstaculo = false;
bool fallaDetectada = false;
const int PARPADEO_LENTO = 10; // 500ms*10=5

// Configuración de GPIO
void configurarGPIO() {
    gpio_config_t io_conf;

    // Configuración de botones
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_BOTON_ABRIR) | (1ULL << PIN_BOTON_CERRAR) | (1ULL << PIN_BOTON_PARO) | (1ULL << PIN_FOTO_CELDA);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    // Configuración de LEDs
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_LED_ESTADO) | (1ULL << PIN_LED_FALLA);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
}

// Función para leer los botones y detectar obstáculos
void leerEntradas() {
    static bool estadoAnteriorAbrir = true;
    static bool estadoAnteriorCerrar = true;
    static bool estadoAnteriorParo = true;
    static bool estadoAnteriorFotoCelda = true;

    bool estadoActualAbrir = gpio_get_level(PIN_BOTON_ABRIR);
    bool estadoActualCerrar = gpio_get_level(PIN_BOTON_CERRAR);
    bool estadoActualParo = gpio_get_level(PIN_BOTON_PARO);
    bool estadoActualFotoCelda = gpio_get_level(PIN_FOTO_CELDA);

    if (estadoActualAbrir == false && estadoAnteriorAbrir == true) {
        printf("Botón 'Abrir' presionado.\n");
        estadoActual = ABRIENDO;
    }
    if (estadoActualCerrar == false && estadoAnteriorCerrar == true) {
        printf("Botón 'Cerrar' presionado.\n");
        estadoActual = CERRANDO;
    }
    if (estadoActualParo == false && estadoAnteriorParo == true) {
        printf("Botón 'Paro' presionado. Activando paro de emergencia...\n");
        estadoActual = PARADA;
    }
    if (estadoActualFotoCelda == false && estadoAnteriorFotoCelda == true) {
        printf("Obstáculo detectado por foto celda...\n");
        hayObstaculo = true;
    }
    if (estadoActualFotoCelda == true && estadoAnteriorFotoCelda == false) {
        printf("Obstáculo quitado por foto celda...\n");
        hayObstaculo = false;
    }

    estadoAnteriorAbrir = estadoActualAbrir;
    estadoAnteriorCerrar = estadoActualCerrar;
    estadoAnteriorParo = estadoActualParo;
    estadoAnteriorFotoCelda = estadoActualFotoCelda;
}

// Función para manejar el estado de falla
void gestionarFalla() {
    printf("¡FALLA DETECTADA! Bombillo parpadeando...\n");
    fallaDetectada = true; // Activar el parpadeo del LED
}

// Función para el parpadeo del bombillo
void temporizador50ms() {
    static unsigned int cnt_led_lamp = 0; // Contador para el parpadeo
    cnt_led_lamp++;

    // Verificar si el LED debe parpadear
    if (fallaDetectada) { // Equivalente a "io.led == true"
        if (cnt_led_lamp >= PARPADEO_LENTO) {
            cnt_led_lamp = 0; // Reiniciar el contador
            int estadoLed = gpio_get_level(PIN_LED_FALLA); // Obtener el estado actual del LED
            gpio_set_level(PIN_LED_FALLA, !estadoLed); // Cambiar el estado del LED
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
