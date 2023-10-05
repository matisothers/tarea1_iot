import socket
import random
import struct
from enum import Enum
from modelos import Configuration, Datos, Logs, Loss
from peewee import DoesNotExist
from packet_parser import * 

from datetime import datetime
import uuid
from packet_parser import pack as msg_pack
from modelos import create_tables


create_tables()

# from pyinput import keyboard

HOST = '0.0.0.0'  # Escucha en todas las interfaces disponibles
PORT = 1234       # Puerto en el que se escucha

address = (HOST, PORT)

# TODO:
# 3. Paquetes recibidos por la Raspberry Pi deben ser descompuestos y almacenados en la base de datos
# 4. En cada envío de datos, deberá verificarse la configuración de la base de datos.
# 5. Se deberá ajustar el protocolo de envío o el tipo de conexión.

# TL: Transport Layer
class TL(Enum):
    TCP = 0
    UDP = 1
    
        
        

def create_instance(self,datos:dict):
    Datos.create(**datos)


class Config:
    def __init__(self, transport_layer:int=1, id_protocol:int=0):
        self.transport_layer = transport_layer
        self.id_protocol = id_protocol
        self.row = None
        try:
            self.row = Configuration.get_by_id(1)
        except DoesNotExist:
            default_row = {"id": 1, "id_protocol": self.id_protocol, "transport_layer": self.transport_layer}
            Configuration.create(**default_row)
            self.row = Configuration.get_by_id(1)

    def get(self):
        data = Configuration.get_by_id(1)
        return {
            "transport_layer": data.transport_layer,
            "id_protocol": data.id_protocol
        }
    
    def set(self, transport_layer:int, id_protocol:int):
        self.transport_layer = transport_layer
        self.id_protocol = id_protocol
        self.row.transport_layer = transport_layer
        self.row.id_protocol = id_protocol
        self.row.save()

# TODO: revisar si es necesario limitar la cantidad de conexiones (self.socket.listen(conns))
class Server:
    def __init__(self, transport_layer:int = 0, host:str=HOST, port:int=PORT, buff_size:int=1024, config=Config()) -> None:
        self.transport_layer = transport_layer
        self.host = host
        self.port_TCP = port
        self.port_UDP = port+1
        self.socket_TCP = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket_TCP.bind((self.host,self.port_TCP))
        
        self.socket_UDP = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket_UDP.bind((self.host,self.port_UDP))
        
        self.buff_size = buff_size
        
        self.config = config

    # Cerrar el socket actual para reemplazarlo por uno que utilice el nuevo protocolo
    # def set_socket(self) -> None:
    #     print("Changing socket protocol")
    #     if self.socket:
    #         self.socket.close()
    #         self.socket = None
    #     if self.transport_layer == 0:
    #         self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #         self.socket.bind(self.address)
    #         self.socket.listen(10)
    #     else:
    #         self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    #         self.socket.bind(self.address)
    #         self.socket.listen(10)
    #     print(f"{self.transport_layer} socket waiting for connections at port {self.port}")
    
    # Si el protocolo cambia, también lo hace el socket
    # def set_protocol(self, transport_layer:int) -> None:
    #     if self.transport_layer != transport_layer:
    #         self.transport_layer = transport_layer
    #         self.set_socket()

    def run(self):
        while True:
            config = self.config.get()
            transport_layer = config['transport_layer']
            if transport_layer == 0:
                self.tcp_handle()
            else:
                self.udp_handle()

    def parse_header(self) -> bytes:
        id = random.randint(0,99) #int de 2 bytes
        mac = str(uuid.getnode()).encode('utf-8') #string de 6 bytes
        config = self.config.get()
        transport_layer = config['transport_layer']  # int encodeado 1 byte, TCP = 0, UDP = 1
        id_protocol = config['id_protocol']
        length = 12

        return struct.pack('<H6sBBH', id, mac, transport_layer, id_protocol, length)

    def unpack_msg(self, packet:bytes):
        id, mac, transport_layer, id_protocol, length = struct.unpack('<H6sBBH', packet[:12])
        header = {
            'id': id,
            'mac': mac,
            'transport_layer': transport_layer,
            'id_protocol': id_protocol,
            'length': length
        }
        body = packet[12:] # struct.unpack('<{}s'.format(length), packet[12:])[0].decode('utf-8')
        return [header, body]

    def parse_body(self, msg:bytes) -> dict:
        
        header, body = self.unpack_msg(msg)
        id_protocol = header["id_protocol"]
        data = ['batt_level', 'timestamp', 'temp', 'press', 'hum', 'co', 'rms', 'amp_x', 'frec_x','amp_y', 'frec_y','amp_z', 'frec_z']
        p4 = ['batt_level', 'timestamp', 'temp', 'press', 'hum', 'co', 'acc_x', 'acc_y', 'acc_z', 'rgyr_x', 'rgyr_y', 'rgyr_z'] 
        d = {}
        if id_protocol == 0:
            parsed_data = struct.unpack('<B', body)
            # Estructura del protocolo 0
            # HEADERS + Batt_level
            pass
        elif id_protocol == 1:
            parsed_data = struct.unpack('<BL', body)
            # Estructura del protocolo 1
            # HEADERS + Batt_level + Timestamp 
            pass
        elif id_protocol == 2:
            parsed_data = struct.unpack('<BLBiBi', body)
            # Estructura del protocolo 1
            # HEADERS + Batt_level + Timestamp + Temp + Press + Hum +Co
            pass
        elif id_protocol == 3:
            parsed_data = struct.unpack('<BLBiBifffffff', body)
        else:
            parsed_data = struct.unpack('<BLBiBi2000f2000f2000f2000f2000f2000f', body)
            l = len(parsed_data)
            for k in range(l):
                d[p4[k]] = parsed_data[k]
            return d

        l = len(parsed_data)
        for k in range(l):
            d[data[k]] = parsed_data[k]
        return d


    def save_to_database(self, msg: bytes):
        header, body = self.unpack_msg(msg)
        table_data = self.parse_body(body,header["id_protocol"])
        # subir datos a la tabla Datos
        Datos.create(**table_data)

        # subir datos a la tabla Logs
        id_device = header['mac']
        transport_layer = header['transport_layer']
        id_protocol = header['id_protocol']
        now = datetime.now()
        timestamp = datetime.timestamp(now)

        # subir datos a la tabla Loss

    def create_log(self, device):
        timestamp = datetime.now()
        config = self.config.get()
        print(config)
        log = {
            'id_device': device,
            'transport_layer':config['transport_layer'],
            'id_protocol': config['id_protocol'],
            'timestamp': timestamp
        }
        print(log)
        Logs.create(**log)
        print("[SERVER] CREADO LOG", log)

    #def save_to_loss(self,timestamp:int,packet_loss:bytes):


    
    def tcp_handle(self):
    
        #with self.connection: # no deberia cerrar socket
        
       
        connection, address = self.socket_TCP.accept() # hace conexion TCP
        with connection:
            print(f'{address} has connected')
            # TODO: Logear la conexión
            # Logs.create(...)
            header = self.parse_header()
            # Enviar la configuración al microcontrolador
            print("sending header")
            connection.send(header)
            print("Header sent")

            # Esperar respuesta del mensaje
            print("esperando recibir respuesta")
            data = connection.recv(self.buff_size)
            print("respuesta recibida: ", data.decode('utf-8'))
            
            # TODO: Guardar mensaje en la base datos con la data recibida
            # table_data = self.parse_body(data)
            # Datos.create(**table_data) # insertar datos en la base de datos
            
            connection.close()


    def udp_handle(self):
        connected_clients = []
        while True:
            # Recibimos mensaje
            data, addr = self.socket_UDP.recvfrom(self.buff_size)
            print(data)
            mac = addr[0]
            if mac not in connected_clients:
                print(f'[SERVER] SE CONECTO {addr}')
                connected_clients.append(mac)
                self.create_log(device=mac)
                # Enviamos header con la configuración
                header = self.parse_header()
                self.socket_UDP.sendto(header, addr)
            else:
                # El cliente ya recibió el header, por lo que 'datos' contiene la información del protocolo actual
                # table_data = self.parse_body(data)
                #Datos.create(**table_data) # insertar datos en la base de datos
                # print(table_data)
                config = self.config.get()
                header = self.parse_header()
                # Enviar la configuración
                self.socket_UDP.sendto(header, addr)
                if config['transport_layer'] != 1:
                    break

        config = self.config.get()
        tl = config['transport_layer']
        self.set_protocol(tl)
        self.run()

s = Server()
s.run()

    

"""
# Servidor echo (UDP)
def udp_connection():
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.bind(address)
        buff_size = 1024
        while True:
            # Recibir mensaje
            msg, addr = s.recvfrom(buff_size)
            print(f"ESP message: {msg.decode()}")
            
            # Enviar respuesta
            s.sendto(msg, addr)

# Crea un socket para IPv4 y conexión TCP
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind(address)
    s.listen()

    print("El servidor está esperando conexiones en el puerto", PORT)

    while True:
        conn, addr = s.accept()  # Espera una conexión
        with conn:
            print('Conectado por', addr)
            data = conn.recv(1024)  # Recibe hasta 1024 bytes del cliente
            if data:
                print("Recibido: ", data.decode('utf-8'))
                respuesta = "tu mensaje es: " + data.decode('utf-8')
                conn.sendall(respuesta.encode('utf-8'))  # Envía la respuesta al cliente



"""
