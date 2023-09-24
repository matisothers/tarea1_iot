#include <stdio.h>

// #include "freertos/FreeRTOS.h"

struct Client{
    char* transport_layer;
    int id_protocol;


    void (*Client__init)(struct Client*, char* transport_layer, int id_protocol);
    char  (*Client__get_transport_layer)(struct Client* self);
    int (*Client__get_id_protocol)(struct Client* self);
    void (*Client__set_config)(struct Client* self, char* transport_layer, int id_protocol);
    void (*Client__tcp)(struct Client* self);
    void (*Client__udp)(struct Client* self);
    void (*Client__handle)(struct Client* self);
} client;

void Client__init(struct Client* self, char* transport_layer, int id_protocol){
    self->id_protocol = id_protocol;
    self->transport_layer = transport_layer;
    esp_deep_sleep_enable_timer_wakeup(60000000); // 60 segundos
}

char* Client__get_transport_layer(struct Client* self){
    return self->transport_layer;
}

int Client__get_id_protocol(struct Client* self){
    return self->id_protocol;
}

void Client__set_config(struct Client* self, char* transport_layer, int id_protocol){
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
    switch (id_protocol){
        case 0:
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
    // ...
    // Teniendo la configuración, utilizar el socket correspondiente (TCP, UDP) y enviar los datos según el protocolo (0, 1, 2, 3, 4)
    while (1){
        if (Client__get_transport_layer(self) == "TCP"){
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