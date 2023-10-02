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




void Client__init(struct Client* self){
    esp_sleep_enable_timer_wakeup(60000000); // setea el sleep en 60 segundos
    self->transport_layer = 0;
    self->id_protocol = 0;
    self->packet_id = 0;
    self->MAC = "123456";
    // por defecto TCP
    self->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (self->socket == -1) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
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
    memset(buffer, 0, buff_size);
    if (recv(self->socket, buffer, buff_size, 0) == -1) {
        ESP_LOGE(TAG, "Error al recibir datos");
        exit(EXIT_FAILURE);
    }
    ESP_LOGI(TAG, "Datos recibidos: %s", buffer);
}

void Client__close_socket(struct Client* self){
    close(self->socket);
}

void Client__set_socket(struct Client* self, int transport_layer){
    if (transport_layer == 0) {
        self->socket = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        self->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    
}

void Client__change_socket(struct Client* self){
    Client__close_socket(&self);
    int transport_layer = Client__get_transport_layer(&self);
    Client__set_socket(&self, transport_layer);
}

void Client__tcp_connect(struct Client* self) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr);

    if (connect(self->socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        ESP_LOGE(TAG, "Error al conectar");
        close(self->socket);
        return;
    }

    
}

/*Manda un paquete y hace deep sleep 60s luego repite el proceso*/
void Client__tcp(struct Client* self){
    // Crear el mensaje según el protocolo
    // char* msg = Client__handle_msg(self)
    
    // Enviar el mensaje
    

    // Crear un socket
    //int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //if (sock < 0) {
    //    ESP_LOGE(TAG, "Error al crear el socket");
    //    return;
    //}

    // Conectar al servidor
    Client__tcp_connect(&self);
    

    char header[1024];
    Client__recv(&self, header, sizeof(header));
    struct Info unpacked_header = unpack(header);

    int transport_layer = unpacked_header.transport_layer;
    int id_protocol = unpacked_header.id_protocol;
    Client__set_config(&self, transport_layer, id_protocol);

    while (1) {
        // enviar mensaje
        byte* msg = Client__handle_msg(self);
        send(self->socket, msg, strlen(msg), 0);
        // entrar en modo Deep Sleep por 60 segundo
        esp_deep_sleep_start();

        char header[1024];
        Client__recv(&self, header, sizeof(header));
        struct Info unpacked_header = unpack(header);
        Client__set_config(&self, transport_layer, id_protocol);
        if (self->transport_layer != 0) {
            break;
        }
    }
    Client__change_socket(&self);
    return;
}

/*Manda paquetes de manera continua hasta que el valor de Transport Layer cambie */
void Client__udp(struct Client* self){
    // Crear socket
    struct sockaddr_in server_addr;
    socklen_t server_struct_length = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(sock < 0){
        printf("Error while creating socket\n");
        return;
    }
    printf("Socket created successfully\n");

    byte* msg = Client__handle_msg(self);
    sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&server_addr, server_struct_length);

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


// función que retorna el mensaje (headers+body) empaquetado
byte* Client__handle_msg(struct Client* self){
    
    byte* message;
    // crear body según el id_protocol
    int id_protocol = Client__get_id_protocol(self);
    int body_size = 0;
    switch (id_protocol){
        case 0:
            body_size = 1;
            break;
        case 1:
            body_size = 5;
            break;
        case 2:
            body_size = 15;
            break;
        case 3:
            body_size = 15 + 7*4;
            break;
        case 4:
            body_size = 15 + 12000 * sizeof(float);
            break;
    }

    // Crear headers
    
    message = (byte*) malloc(12 + body_size * sizeof(byte));
    set_header_to_msg(self, message, body_size);
    // ...

    char batt = batt_level();
    memcpy(message + 12, &batt, 1);
    if (id_protocol > 0){
        unsigned long timestamp = (unsigned long)time(NULL);
        memcpy(message + 13, &timestamp, 4);
    }
    if (id_protocol > 1){
        struct THPC_Data tdata = generate_THPC_Data();
        memcpy(message + 18, &tdata, 10);
    }
    if (id_protocol == 3){
        struct kpi_data kdata = generate_kpi_data();
        memcpy(message + 27, &kdata, 7*4);
    }
    if (id_protocol == 4){
        float *fdata = malloc(12000 * sizeof(float));
        acc_sensor(fdata);
        memcpy(message + 27, (byte *)fdata, 12000 * sizeof(float));
    }
    // retornar headers + body
    
    return message;
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
        ESP_LOGE(TAG, "Error al conectar primer socket");
        close(sock);
        return;
    }

    // Recibir respuesta

    byte rx_buffer[128];
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



// PACK UNPACK


byte* pack(int packet_id, char* mac, int transport_layer, int id_protocol, char * msg) {
    int length_msg = strlen(msg);
    int length_packet = 12 + length_msg;
    byte * packet = malloc(length_packet);
    
   
    memcpy(packet, &packet_id, 2);
    memcpy(packet + 2, mac, 6);
    memcpy(packet + 8, &transport_layer, 1);
    memcpy(packet + 9, &id_protocol, 1);

    memcpy(packet + 10, &length_packet, 2);
    memcpy(packet + 12, msg, length_msg);
    return packet;
}



struct Info unpack(byte * packet) {
    int packet_id;
    int transport_layer;
    int id_protocol;

    int length_msg;

    memcpy(&packet_id, packet, 2);
    memcpy(&transport_layer, packet + 8, 1);
    memcpy(&id_protocol, packet + 9, 1);
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
    res.temp = 5 + (rand()/RAND_MAX)*25;
    res.hum = 30 + (rand()/RAND_MAX)*50;
    res.pres = 1000 + (rand()/RAND_MAX)*200;
    res.co = 30 + ((float) rand() / (float) (RAND_MAX)) * 170;
    return res;
}
