#include <stdio.h>

#include "lwip/sockets.h" // Para sockets

#include "client.h"
#include "freertos/FreeRTOS.h"



void Client__init(struct Client* self){
    esp_deep_sleep_enable_timer_wakeup(60000000); // setea el sleep en 60 segundos
    self->transport_layer = 0;
    
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

/*Manda un paquete y hace deep sleep 60s luego repite el proceso*/
void Client__tcp(struct Client* self){
    // Crear el mensaje según el protocolo
    // char* msg = Client__handle_msg(self)
    
    // Enviar el mensaje
    // ...
    
    // entrar en modo Deep Sleep por 60 segundo
    esp_deep_sleep_start();

    // Consultar por la configuración nuevamente (?)

    
    return;
}

/*Manda paquetes de manera continua hasta que el valor de Transport Layer cambie */
void Client__udp(struct Client* self){
    return;
}

// función que retorna el mensaje (headers+body) empaquetado
char* Client__handle_msg(struct Client* self){
    
    // crear body según el id_protocol
    int id_protocol = Client__get_id_protocol(self);
    int response;
    switch (id_protocol){
        case 0:
            char batt = batt_level();
            response = Client__send(self, &batt);
            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
    }

    // Crear headers
    // ...

    // retornar headers + body
}

void Client__handle(struct Client* self){ 
    // Se inicia la conexión con socket TCP para obtener transport_layer e id_protocol

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


    // Recibir respuesta

    char rx_buffer[128];
    int rx_len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (rx_len < 0) {
        ESP_LOGE(TAG, "Error al recibir datos");
        return;
    }
    ESP_LOGI(TAG, "Datos recibidos: %s", rx_buffer);

    // ver respuesta y cambiar protocol y transport layer

    struct Info info = unpack(rx_buffer);

    
   
    Client__set_config(self,info.transport_layer, info.id_protocol);
    
    

    // Close the socket:
    close(sock);
    // Teniendo la configuración, utilizar el socket correspondiente (TCP, UDP) y enviar los datos según el protocolo (0, 1, 2, 3, 4)
    while (1){
        if (Client__get_transport_layer(self) == 0){
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

void Client__send(client *self, unsigned char *buffer, size_t size){
    if (transport_layer){ //TCP
        send(self.socket, buffer, size, 0);
    } 
    else{ //UDP
        sento(self.socket, buffer, size, 0, self.to_socketaddr, self.socketaddr_size); //to_socketaddr is (struct sockaddr*) 
    }
}


// PACK UNPACK


unsigned char * pack(int packet_id, char* mac, int transport_layer, int id_protocol, char * msg) {
    int length_msg = strlen(msg);
    int length_packet = 12 + length_msg;
    unsigned char * packet = malloc(length_packet);
    
   
    memcpy(packet, &packet_id, 2);
    memcpy(packet + 2, MAC, 6);
    memcpy(packet + 8, &transport_layer, 1);
    memcpy(packet + 9, &id_protocol, 1);

    memcpy(packet + 10, &length_packet, 2);
    memcpy(packet + 12, text, length_msg);
    return packet;
}



struct Info{
    int transport_layer;
    int id_protocol;
};


struct Info unpack(char * packet) {
    int packet_id;
    char* MAC;
    int transport_layer;
    int protocol;

    int length_msg;
    char * text;

    memcpy(&packet_id, packet, 2);
    memcpy(MAC, packet + 2, 6);
    memcpy(&transport_layer, packet + 8, 1);
    memcpy(&protocol,packet + 9, 1);
    memcpy(&length_msg, packet + 10, 2);

    struct Info res;
    res.transport_layer = transport_layer;
    res.id_protocol = id_protocol;
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