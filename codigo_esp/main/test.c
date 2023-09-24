#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include "lwip/sockets.h" // Para sockets

//Credenciales de WiFi

#define WIFI_SSID "testIot"
#define WIFI_PASSWORD "IotTeam2023"
#define SERVER_IP     "192.168.4.1" // IP del servidor
#define SERVER_PORT   1234

// Variables de WiFi
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
static const char* TAG = "WIFI";
static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;


void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 10) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(char* ssid, char* password) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));

    // Set the specific fields
    strcpy((char*)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, WIFI_PASSWORD);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", ssid,
                 password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", ssid,
                 password);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
        IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
        WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

void nvs_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}


void socket_tcp(char* msg){
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr);

    // Crear un socket
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Error al crear el socket");
        return;
    }

    // Conectar al servidor
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        ESP_LOGE(TAG, "Error al conectar");
        close(sock);
        return;
    }

    // Enviar mensaje "Hola Mundo"
    send(sock, msg, strlen(msg), 0);

    // Recibir respuesta

    char rx_buffer[128];
    int rx_len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (rx_len < 0) {
        ESP_LOGE(TAG, "Error al recibir datos");
        return;
    }
    ESP_LOGI(TAG, "Datos recibidos: %s", rx_buffer);
    
    // Cerrar el socket
    close(sock);
}



void socket_udp(char* msg){
    int socket_desc;
    struct sockaddr_in server_addr;
    int server_struct_length = sizeof(server_addr);
    
    
    // Create socket:
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Send the message to server:
    if(sendto(socket_desc, msg, strlen(msg), 0,
         (struct sockaddr*)&server_addr, server_struct_length) < 0){
        printf("Unable to send message\n");
        return;
    }
    
    // Receive the server's response:
    char server_message[128];
    if(recvfrom(socket_desc, server_message, sizeof(server_message), 0,
         (struct sockaddr*)&server_addr, &server_struct_length) < 0){
        printf("Error while receiving server's msg\n");
        return;
    }
    
    printf("Server's response: %s\n", server_message);
    
    // Close the socket:
    close(socket_desc);
}





// TODO:
// Crear la función socket_udp

// Crear funcion para empaquetar datos


void acc_sensor(float* data){
    // funcion que genera los valores acc x,y,z y Regyr x,y,z en ese orden

    float a = 16.0;
    for(int i = 0; i < 6000; i++){
        data[i] = ((float) rand() / (float) (RAND_MAX)) * a * 2 - a;
    }

    a = 1000;
    for(int i = 6000; i < 12000; i++){
        data[i] = ((float) rand() / (float) (RAND_MAX)) * a * 2 - a;
    }
}

uint8_t batt_level(){
    return (rand() % 100) + 1;
}


struct kpi_data{
    float ampx;
    float freqx;
    float ampy;
    float freqy;
    float ampz;
    float freqz;
    float rms;
};

kpi_data generate_kpi_data(){
    struct kpi_data res;
    
    res.ampx = 0.0059 + ((float) rand() / (float) RAND_MAX) * (0.12-0.0059);
    res.freqx = 29.0 + ((float) rand() / (float) RAND_MAX) * (2.0);
    res.ampy = 0.0041 + ((float) rand() / (float) RAND_MAX) * (0.11-0.0041);
    res.freqy = 59.0 + ((float) rand() / (float) RAND_MAX) * (2.0);
    res.ampz = 0.008 + ((float) rand() / (float) RAND_MAX) * (0.15-0.008);
    res.freqz = 89.0 + ((float) rand() / (float) RAND_MAX) * (2.0);
    res.rms = sqrt(res.ampx*res.ampx + res.ampy*res.ampy + res.ampz * res.ampz);

    return res;
}

struct THPC_Data
{
    int temp;
    int hum;
    int pres;
    float co;
};

THPC_Data generate_THPC_Data(){
    struct THPC_Data res;
    res.temp = 5 + (rand()/RAND_MAX)*25;
    res.hum = 30 + (rand()/RAND_MAX)*50;
    res.pres = 1000 + (rand()/RAND_MAX)*200;
    res.co = 30 + ((float) rand() / (float) (RAND_MAX)) * 170;
    return res;
}


void app_main(void){
    /* TODO: Crear el flujo por parte de la ESP
    1. Conectarse como TCP y preguntar por el tipo de protocolo (1, 2, 3, 4) y el transport_layer (TCP, UDP)
    * Se recibe TCP:
        - Enviar paquete
        - Entrar en Deep Sleep por 60 seg
        - Repetir
    * Se recibe UDP:
        - Enviar datos de forma continua
        - Consultar si cambió el transport_layer
        - 
    3. Paquetes recibidos por la Raspberry Pi deben ser descompuestos y almacenados en la base de datos
    4. En cada envío de datos, deberá verificarse la configuración de la base de datos.
    5. Se deberá ajustar el protocolo de envío o el tipo de conexión.
    */

    



    nvs_init();
    wifi_init_sta(WIFI_SSID, WIFI_PASSWORD);
    ESP_LOGI(TAG,"Conectado a WiFi!\n");
    socket_tcp();
}
