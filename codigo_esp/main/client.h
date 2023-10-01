#include <stdio.h>

struct THPC_Data
{
    int temp;
    int hum;
    int pres;
    float co;
} thpc;
struct kpi_data{
    float ampx;
    float freqx;
    float ampy;
    float freqy;
    float ampz;
    float freqz;
    float rms;
} kpi;
struct info;
struct Client{
    int transport_layer;
    int id_protocol;
    char * MAC;


    void (*Client__init)(struct Client*);
    int  (*Client__get_transport_layer)(struct Client* self);
    int (*Client__get_id_protocol)(struct Client* self);
    void (*Client__set_config)(struct Client* self, int transport_layer, int id_protocol);
    void (*Client__tcp)(struct Client* self);
    void (*Client__udp)(struct Client* self);
    void (*Client__handle)(struct Client* self);
} client;

void Client__init(struct Client*);
int Client__get_transport_layer(struct Client* self);
int client__get_id_protocol(struct Client* self);
void Client__set_config(struct Client* self, int transport_layer, int id_protocol);
void Client__tcp(struct Client* self);
void Client__udp(struct Client* self);
void Client__handle(struct Client* self);
void acc_sensor(float* data);
uint8_t batt_level()
struct kpi_data generate_kpi_data()
THPC_Data generate_THPC_Data();