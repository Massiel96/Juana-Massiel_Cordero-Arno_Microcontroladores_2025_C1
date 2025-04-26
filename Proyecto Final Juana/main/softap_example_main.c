/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#define WIFI_SSID "JUANA-Proyecto"
#define WIFI_PASS "123456789m"
#define MAX_CONN 4

#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_GPIO       18
#define LEDC_DUTY_RES   LEDC_TIMER_13_BIT

#define LED_ASTABLE_GPIO    21
#define LED_MONOSTABLE_GPIO 22

static const char index_html[] =
"<!DOCTYPE html><html><head><title>NE555 ESP32</title>"
"<style>"
"body { font-family: sans-serif; text-align: center; padding: 30px; background-color:rgb(32, 186, 224); }"
"input { margin: 5px; padding: 8px; }"
".resultado { margin-top: 20px; border: 1px solid #ccc; padding: 15px; background: #fff; display: inline-block; }"
"</style>"
"<script>"
"function enviarFormulario(formId, endpoint, resultadoId) {"
"  const form = document.getElementById(formId);"
"  const datos = new URLSearchParams(new FormData(form)).toString();"
"  fetch(endpoint + '?' + datos)"
"    .then(r => r.text()).then(html => document.getElementById(resultadoId).innerHTML = html);"
"  return false;"
"}"
"function detenerPWM() {"
"  fetch('/detener_pwm').then(r => r.text()).then(alert);"
"}"
"</script></head><body>"
"<h2> Esta página emula el temporizador NE555 en modos Astable y Monostable usando ESP32.</h2>"
"<p>Juana M. C. Arno - Emulador NE555 con ESP32</p>"

"<form id='form_astable' onsubmit='return enviarFormulario(\"form_astable\", \"/calcular_astable\", \"resultado_astable\")'>"
"<h3>Modo Astable</h3>"
"R1 (Ohm): <input name='r1' type='number' step='any' min='0' required><br>"
"R2 (Ohm): <input name='r2' type='number' step='any' min='0' required><br>"
"C (uF): <input name='c' type='number' step='any' min='0' required><br>"
"<input type='submit' value='Calcular Astable'>"
"</form>"
"<div id='resultado_astable' class='resultado'></div>"
"<button onclick='detenerPWM()'>Detener Astable</button><hr>"

"<form id='form_monostable' onsubmit='return enviarFormulario(\"form_monostable\", \"/calcular_monostable\", \"resultado_monostable\")'>"
"<h3>Modo Monostable</h3>"
"R (Ohm): <input name='r' type='number' step='any' min='0' required><br>"
"C (uF): <input name='c' type='number' step='any' min='0' required><br>"
"<input type='submit' value='Calcular Monostable'>"
"</form>"
"<div id='resultado_monostable' class='resultado'></div>"
"<button onclick='detenerPWM()'>Detener Monostable</button>"
"</body></html>";

void configurar_pwm(float frecuencia, float duty_cycle) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = (uint32_t)frecuencia,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_GPIO,
        .duty           = (uint32_t)((duty_cycle / 100.0f) * ((1 << LEDC_DUTY_RES) - 1)),
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t astable_handler(httpd_req_t *req) {
    char buf[100], param[32];
    float r1 = 0, r2 = 0, c = 0;

    httpd_req_get_url_query_str(req, buf, sizeof(buf));
    if (httpd_query_key_value(buf, "r1", param, sizeof(param)) == ESP_OK) r1 = atof(param);
    if (httpd_query_key_value(buf, "r2", param, sizeof(param)) == ESP_OK) r2 = atof(param);
    if (httpd_query_key_value(buf, "c", param, sizeof(param)) == ESP_OK) c = atof(param);

    float tHigh = 0.693 * (r1 + r2) * c * 1e-6;
    float tLow = 0.693 * r2 * c * 1e-6;
    float periodo = tHigh + tLow;

    if (periodo <= 0.0f) {
        httpd_resp_sendstr(req, "<h3>Error: Valores inválidos.</h3>");
        return ESP_OK;
    }

    float frecuencia = 1.0f / periodo;
    float duty = (tHigh / periodo) * 100.0f;

    configurar_pwm(frecuencia, duty);
    gpio_set_level(LED_ASTABLE_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(LED_ASTABLE_GPIO, 0);

    char resp[512];
    snprintf(resp, sizeof(resp),
        "<h3>Resultado Modo Astable</h3>"
        "<p><strong>Frecuencia:</strong> %.2f Hz</p>"
        "<p><strong>Ciclo de trabajo:</strong> %.2f%%</p>"
        "<p><strong></strong></p>", frecuencia, duty);

    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

static esp_err_t monostable_handler(httpd_req_t *req) {
    char buf[100], param[32];
    float r = 0, c = 0;

    httpd_req_get_url_query_str(req, buf, sizeof(buf));
    if (httpd_query_key_value(buf, "r", param, sizeof(param)) == ESP_OK) r = atof(param);
    if (httpd_query_key_value(buf, "c", param, sizeof(param)) == ESP_OK) c = atof(param);

    float tiempo = 1.1 * r * c * 1e-6;

    gpio_set_level(LED_MONOSTABLE_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS((int)(tiempo * 1000)));
    gpio_set_level(LED_MONOSTABLE_GPIO, 0);

    char resp[512];
    snprintf(resp, sizeof(resp),
        "<h3>Resultado Modo Monostable</h3>"
        "<p><strong>Tiempo de pulso:</strong> %.6f s</p>"
        "<p><strong></strong></p>", tiempo);

    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

static esp_err_t detener_pwm_handler(httpd_req_t *req) {
    ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL, 0);
    httpd_resp_sendstr(req, "PWM detenido.");
    return ESP_OK;
}

void start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &(httpd_uri_t){.uri="/", .method=HTTP_GET, .handler=index_handler});
        httpd_register_uri_handler(server, &(httpd_uri_t){.uri="/calcular_astable", .method=HTTP_GET, .handler=astable_handler});
        httpd_register_uri_handler(server, &(httpd_uri_t){.uri="/calcular_monostable", .method=HTTP_GET, .handler=monostable_handler});
        httpd_register_uri_handler(server, &(httpd_uri_t){.uri="/detener_pwm", .method=HTTP_GET, .handler=detener_pwm_handler});
    }
}

void wifi_init_softap() {
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = MAX_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
}

void app_main(void) {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    gpio_set_direction(LED_ASTABLE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_MONOSTABLE_GPIO, GPIO_MODE_OUTPUT);

    wifi_init_softap();
    start_webserver();
}