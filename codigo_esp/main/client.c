#include <stdio.h>


#include "client.h"
#include "esp_sleep.h"
// #include "esp_event.h"
// #include "esp_log.h"
// #include "esp_system.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/event_groups.h"
// #include "lwip/err.h"
// #include "lwip/sys.h"
// #include "nvs_flash.h"
// #include "lwip/sockets.h" // Para sockets

#define MAC_ADDR_SIZE 6

struct Message {
    uint16_t id;
    uint8_t MAC[6];
    uint8_t transport_layer;
    uint8_t id_protocol;
    uint16_t length;
    byte* body;
};
//usar memcpy para agregar el body


uint8_t get_mac_address() {
    uint8_t mac[MAC_ADDR_SIZE];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    ESP_LOGI("MAC address", "MAC address: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return mac;
}

byte* Client__create_body(struct Client* self, int* length){
    // crear body según el id_protocol
    int id_protocol = Client__get_id_protocol(self);
    int arr[5] = {1, 4, 10, 7*4, 12000*sizeof(float)};
    int body_size = 0;
    for(int i = 0; i <= id_protocol;i++){
        body_size += arr[i];
    }

    byte* message = (byte*) malloc(body_size * sizeof(byte));

    int batt = batt_level();
    // uint8_t batt = 51;
    int bytes_acc = arr[0];
    memcpy(message, &batt, 1);
    if (id_protocol >= 1){
        time_t timestamp;
        time(&timestamp);
        timestamp = 1696627643;

        // ESP_LOGI("offset","protocol 1: %d", bytes_acc);
        // ESP_LOGI("create_body","timestamp: %lld", time(NULL));
    
        memcpy(message + bytes_acc, &timestamp, 4);
        bytes_acc += arr[1];
    } if (id_protocol >= 2){
        struct THPC_Data tdata = generate_THPC_Data();
        ESP_LOGI("offset","protocol 2: %d", bytes_acc);
        memcpy(message + bytes_acc, &tdata.temp, 1);
        memcpy(message + bytes_acc + 1, &tdata.pres, 4);
        memcpy(message + bytes_acc + 5, &tdata.hum, 1);
        memcpy(message + bytes_acc + 6, &tdata.co, 4);
        int temp;
        memcpy(&temp, message + bytes_acc,1);
        ESP_LOGI("AAA","temp: %d",temp);
        bytes_acc += arr[2];
    } if (id_protocol == 3){
        struct kpi_data kdata = generate_kpi_data();
        memcpy(message + bytes_acc, &kdata.rms, 4);
        memcpy(message + bytes_acc + 4*1, &kdata.ampx, 4);
        memcpy(message + bytes_acc + 4*2, &kdata.freqx, 4);
        memcpy(message + bytes_acc + 4*3, &kdata.ampy, 4);
        memcpy(message + bytes_acc + 4*4, &kdata.freqy, 4);
        memcpy(message + bytes_acc + 4*5, &kdata.ampz, 4);
        memcpy(message + bytes_acc + 4*6, &kdata.freqz, 4);
        bytes_acc += arr[3];
    } if (id_protocol == 4){
        float *fdata = malloc(12000 * sizeof(float));
        acc_sensor(fdata);
        memcpy(message + bytes_acc, (byte *)fdata, 12000 * sizeof(float));
    }
    *length = body_size;
    
    return message;
}

struct Message Client__create_msg(struct Client* self, byte* body, int body_length){
    struct Message msg;
    msg.id = self->packet_id;
    self->packet_id++;
    ESP_LOGI("Create msg", "MAC address: %s", self->MAC);
    memcpy(msg.MAC, &self->MAC, 6);
    msg.transport_layer = self->transport_layer;
    msg.id_protocol = self->id_protocol;
    msg.length = 12 + body_length;
    msg.body = (byte*) malloc(body_length);
    memcpy(msg.body, body, body_length);
    ESP_LOGI("Create msg","batt level= %d", *(msg.body));
    return msg;
}

byte* pack_struct(struct Message* msg) {
    int length_msg = msg->length;
    ESP_LOGI("pack_struct","Body length= %d", length_msg);
    // int length_packet = 12 + length_msg;
    byte * packet = malloc(length_msg);

    //headers
    memcpy(packet, &msg->id, 2);
    memcpy(packet + 2, &msg->MAC, 6);
    memcpy(packet + 8, &msg->transport_layer, 1);
    memcpy(packet + 9, &msg->id_protocol, 1);
    memcpy(packet + 10, &msg->length, 2);

    //body
    memcpy(packet + 12, msg->body, length_msg-12);
    ESP_LOGI("pack_struct","batt level= %d", *(packet + 12));
    ESP_LOGI(TAG,"Headers puestos");
    return packet;
}


void Client__init(struct Client* self){
    esp_sleep_enable_timer_wakeup(6000000000);  // setea el sleep en 60 segundos = 60000000
    self->transport_layer = 0;
    self->id_protocol = 0;
    self->packet_id = 0;
    uint8_t* mac = malloc(MAC_ADDR_SIZE);
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    ESP_LOGI("MAC address", "MAC address: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    self->MAC = mac;
    //Tendremos dos sockets
    self->socket_tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    self->socket_udp = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (self->socket_tcp < 0) {
        ESP_LOGE(TAG,"Error al crear el socket TCP");
        return;
    };
    if (self->socket_udp < 0){
        ESP_LOGE(TAG,"Error al crear el socket UDP");
        return;
    }
    else{
        ESP_LOGI(TAG,"Sockets creados con exito");
        Client__tcp_connect(self);
        return;
    }
}

int Client__get_transport_layer(struct Client* self){
    return self->transport_layer;
}

int Client__get_id_protocol(struct Client* self){
    return self->id_protocol;
}

void Client__set_config(struct Client* self, int transport_layer, int id_protocol){
    self->id_protocol = id_protocol;
    self->transport_layer = transport_layer;
}

void Client__recv(struct Client* self, char *buffer, int buff_size){
    
    if (recv(self->socket_tcp, buffer, buff_size, 0) == -1) {
        ESP_LOGE(TAG, "Error al recibir datos");
        exit(EXIT_FAILURE);
    }
    ESP_LOGI(TAG, "Datos recibidos: %s", buffer);
}

void Client__tcp_connect(struct Client* self) {
    ESP_LOGI(TAG,"Conectando SOCKET TCP");
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT_TCP);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr);

    int code = connect(self->socket_tcp, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (code < 0) {
        ESP_LOGE(TAG, "Error al conectar socket: %d", code);
        close(self->socket_tcp);
        return;
    };
    
    ESP_LOGI(TAG,"Conectando con exito");
}

/*Manda un paquete y hace deep sleep 60s luego repite el proceso*/
void Client__tcp(struct Client* self){
    ESP_LOGI(TAG,"Comenzando TCP");
    int length;
    byte* body = Client__create_body(self, &length);
    struct Message msg = Client__create_msg(self, body, length); 
    byte* packet = pack_struct(&msg); 
    int pres = *(packet + 18);
    ESP_LOGI("thpc_re","pres: %d", pres);
    //byte* msg = Client__handle_msg(self);
    
    int msg_length = length+12;
    ESP_LOGI(TAG, "EL LARGO DEL MENSAJE ES %i", msg_length);
    ESP_LOGI(TAG,"Comenzando a enviar datos");
    send(self->socket_tcp, packet, msg_length, 0);
    // entrar en modo Deep Sleep por 60 segundo
    ESP_LOGI(TAG, "Mensaje enviado, procediendo a mimir");
    
    esp_deep_sleep_start();

 
    return;
}

/*Manda paquetes de manera continua hasta que el valor de Transport Layer cambie */
void Client__udp(struct Client* self){
    // Crear socket
    struct sockaddr_in server_addr;
    socklen_t server_struct_length = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT_UDP);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr);

    int sock = self->socket_udp;


    // enviar mensaje
    int length;
    byte* body = Client__create_body(self, &length);
    struct Message msg = Client__create_msg(self, body, length);
    byte* packet = pack_struct(&msg); 
    //byte* msg = Client__handle_msg(self);
    sendto(sock, packet, msg.length, 0, (struct sockaddr*)&server_addr, server_struct_length);

    // Receive the server's response:
    char server_message[128];
    if(recvfrom(sock, server_message, sizeof(server_message), 0,
         (struct sockaddr*)&server_addr, &server_struct_length) < 0){
        printf("Error while receiving server's msg\n");
        return;
    }
    
    printf("Server's response: %s\n", server_message);
    struct Info info = unpack(server_message);
    Client__set_config(self,info.transport_layer, info.id_protocol);
    ESP_LOGI(TAG, "Config actualizada!");
    return;
}

void set_header_to_msg(struct Client* self, byte* buffer, int body_lenght){
    int size = 12 + body_lenght;
    memcpy(buffer, &(self->packet_id), 2);
    memcpy(buffer+2, self->MAC + 2, 6);
    memcpy(buffer + 8, &(self->transport_layer), 1);
    memcpy(buffer + 9, &(self->id_protocol), 1);
    memcpy(buffer + 10, &(size), 2);
    self->packet_id++;
}

void Client__handle(struct Client* self){ 
    char buffer[1024];
    memset(buffer, 0, 1024);
    ESP_LOGI(TAG,"Esperando recibir mensaje");
    if (recv(self->socket_tcp, buffer, 1024, 0) < 0) {
        ESP_LOGE(TAG, "Error al recibir datos.");
        exit(EXIT_FAILURE);
    }
    ESP_LOGI(TAG, "Datos recibidos: %s", buffer);
    //Client__recv(self, header, sizeof(header));
    struct Info unpacked_header = unpack(buffer);
    int transport_layer = unpacked_header.transport_layer;
    int id_protocol = unpacked_header.id_protocol;
    
    Client__set_config(self, transport_layer, id_protocol);
    ESP_LOGI(TAG,"protocolo y transport layer actualizados, ahora a enviar datitos");
        
    while (1){
        //Client__set_socket(self,Client__get_transport_layer(self));
        if (transport_layer == 0){
            // Utilizar TCP
            Client__tcp(self);
            //
        }
        else {
            // Utilizar UDP
            Client__udp(self);
        }
        
    }
}



// PACK UNPACK

// unused function
// byte* pack(int packet_id, char* mac, int transport_layer, int id_protocol, char * msg) {
//     int length_msg = strlen(msg);
//     int length_packet = 12 + length_msg;
//     byte * packet = malloc(length_packet);
   
//     memcpy(packet, &packet_id, 2);
//     memcpy(packet + 2, mac, 6);
//     memcpy(packet + 8, &transport_layer, 1);
//     memcpy(packet + 9, &id_protocol, 1);

//     memcpy(packet + 10, &length_packet, 2);
//     memcpy(packet + 12, msg, length_msg);
    
//     ESP_LOGI("PACK","packeted_message: %s", packet);
//     return packet;
// }



struct Info unpack(byte * packet) {
    int packet_id;
    int transport_layer;
    int id_protocol;

    int length_msg;
    ESP_LOGI(TAG,"Comenzando unpack");

    memcpy(&packet_id, packet, 2);
    memcpy(&transport_layer, packet + 8, 1);
    memcpy(&id_protocol, packet + 9, 1);
    memcpy(&length_msg, packet + 10, 2);

    ESP_LOGI("unpack","packet_id: %d", packet_id);
    ESP_LOGI("unpack","transport_layer: %d", transport_layer);
    ESP_LOGI("unpack","id_protocol: %d", id_protocol);
    ESP_LOGI("unpack","length_msg: %d", length_msg);
    
    struct Info res;
    res.transport_layer = transport_layer;
    res.id_protocol = id_protocol;

    ESP_LOGI(TAG,"unpack listoco uwu");
    return res;


    // text = malloc(length_msg + 1 - 12); // +1 for the null-terminator - 12 for the header
    // if (text == NULL) {
    //     // Handle memory allocation failure
    //     return;
    // }
    
    // memcpy(text, packet + 12, length_msg);
    // text[length_msg] = '\0'; // Null-terminate the string

    // printf("Packet ID: %d\n", packet_id);
    // printf("Float Value: %f\n", value_float);
    // printf("Text: %s\n", text);

    // free(text); 
}




// FUNCIONES PARA GENERAR DATOS

void acc_sensor(float* data){ // 12.000
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


struct kpi_data generate_kpi_data(){
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


thpc generate_THPC_Data(){
    struct THPC_Data res;
    res.temp = 5 + rand()%25;
    res.temp = 10;
    res.hum = 30 + rand()%50;
    res.pres = 1000 + rand()%200;
    res.co = 30 + ((float) rand() / (float) (RAND_MAX)) * 170;
    ESP_LOGI("thpc","temp: %d", res.temp);
    ESP_LOGI("thpc","hum: %d", res.hum);
    ESP_LOGI("thpc","pres: %d", res.pres);
    ESP_LOGI("thpc","length_msg: %f", res.co);
    return res;
}
