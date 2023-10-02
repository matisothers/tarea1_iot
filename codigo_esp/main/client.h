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
#include <stdio.h>

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

typedef char byte;
struct THPC_Data
{
    int temp;
    int hum;
    int pres;
    float co;
};
typedef struct THPC_Data thpc;
struct kpi_data{
    float ampx;
    float freqx;
    float ampy;
    float freqy;
    float ampz;
    float freqz;
    float rms;
};

typedef struct kpi_data kpi;
struct Info{
    int transport_layer;
    int id_protocol;
};
extern struct Client{
    int transport_layer;
    int id_protocol;
    int packet_id;
    char * MAC;
    int socket;


    // void (*Client__init)(struct Client* self);
    // int  (*Client__get_transport_layer)(struct Client* self);
    // int (*Client__get_id_protocol)(struct Client* self);
    // void (*Client__set_config)(struct Client* self, int transport_layer, int id_protocol);
    // void (*Client__tcp)(struct Client* self);
    // void (*Client__udp)(struct Client* self);
    // void (*Client__handle)(struct Client* self);
};
typedef struct Client client;

extern void Client__init(struct Client* self);
int Client__get_transport_layer(struct Client* self);
int client__get_id_protocol(struct Client* self);
void Client__set_config(struct Client* self, int transport_layer, int id_protocol);
void Client__tcp(struct Client* self);
void Client__udp(struct Client* self);
extern void Client__handle(struct Client* self);
byte* Client__handle_msg(struct Client* self);
void acc_sensor(float* data);
uint8_t batt_level();
struct kpi_data generate_kpi_data();
struct THPC_Data generate_THPC_Data();
byte* pack(int packet_id, char* mac, int transport_layer, int id_protocol, char * msg);
void set_header_to_msg(struct Client* self,byte* buffer, int body_lenght);
struct Info unpack(byte * packet);
