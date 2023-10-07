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
from socket import error as SocketError
import binascii

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
    
        

def prettify(mac_string):
    return ':'.join('%02x' % ord(b) for b in mac_string)
           

def create_instance(self,datos:dict):
    Datos.create(**datos)


class Config:
    def __init__(self, transport_layer:int=0, id_protocol:int=0):
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
        self.socket_TCP.listen()
        
        self.socket_UDP = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket_UDP.bind((self.host,self.port_UDP))
        
        self.buff_size = buff_size
        
        self.config = config

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
        print(struct.unpack('<H6BBBH', packet[:12]))
        id, mac1,mac2,mac3,mac4,mac5,mac6, transport_layer, id_protocol, length = struct.unpack('<H6BBBH', packet[:12])
        mac = f"{mac1}{mac2}:{mac3}{mac4}:{mac5}{mac6}"
  
        header = {
            'header_id': id,
            'header_mac': str(mac),
            'transport_layer': transport_layer,
            'id_protocol': id_protocol,
            'length': length
        }
        body_packet = packet[12:] # struct.unpack('<{}s'.format(length), packet[12:])[0].decode('utf-8')
        print("body bytes: ", body_packet)
        body = self.parse_body(body_packet, id_protocol)
        return dict(header, **body) # retorna los datos usados por la tabla Datos

    def parse_body(self, body:bytes, id_protocol:int) -> dict:
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
            print(len(body))
            parsed_data = struct.unpack('<BLBiBf', body)
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

    def create_data_row(self, data:dict):
        Datos.create(**data)
        print("[SERVER] Creada fila de la tabla Datos:", data)
    
    def create_log_row(self, id_device):
        timestamp = datetime.now()
        config = self.config.get()
        print(config)
        log = {
            'id_device': id_device,
            'transport_layer':config['transport_layer'],
            'id_protocol': config['id_protocol'],
            'timestamp': timestamp
        }
        print(log)
        Logs.create(**log)
        print("[SERVER] Creada fila de la tabla Logs:", log)


    
    def tcp_handle(self):
        try:
            connection, address = self.socket_TCP.accept() # hace conexion TCP
            with connection:
                connection.settimeout(10)
                
                print(f'{address} has connected')
                # Logear la conexión
                self.create_log_row(id_device=address[0])
                header = self.parse_header()
                # Enviar la configuración al microcontrolador
                print("[SERVER] Sending header")
                connection.send(header)
                print("[SERVER] Header sent")
                
                # Esperar respuesta del mensaje
                print("[SERVER] Waiting for a message from client...")
                try:
                    data = connection.recv(self.buff_size)
                    print("[SERVER] Data received: ", data)
                    # Cliente hace DEEP SLEEP -> se cierra la conexión
                except SocketError as e:
                    print(e)
                    print("[SERVER] Timed out receiving data")

                else:
                    unpacked_message = self.unpack_msg(data)
                    print("[SERVER] Data unpacked: ", unpacked_message)
                    # Guardar mensaje en la base datos con la data recibida
                    self.create_data_row(unpacked_message)
                    
                connection.close()
                return
        except SocketError as e:
            print(e)
            return


    def udp_handle(self):
        connected_clients = []
        self.socket_TCP.settimeout(1)
        self.socket_UDP.settimeout(1)
    
        while True:
            try:
                connection, address = self.socket_TCP.accept() # hace conexion TCP
                with connection:
                    print(f'{address} has connected')
                    # Logear la conexión
                    self.create_log_row(id_device=address[0])
                    header = self.parse_header()
                    # Enviar la configuración al microcontrolador
                    print("[SERVER] Sending header")
                    connection.send(header)
                    print("[SERVER] Header sent")
            except:
                pass

            # Recibimos mensaje
            try:
                data, addr = self.socket_UDP.recvfrom(self.buff_size) 
                mac = addr[0]
                if mac not in connected_clients:
                    print(f'[SERVER] SE CONECTO {addr}')
                    connected_clients.append(mac)
                    # self.create_log(device=mac)
                    print(f"[SERVER] Cliente {connected_clients.index(mac)} envió mensaje")
                    print(data)
                    
                    unpacked_message = self.unpack_msg(data)
                    print("[SERVER] Data unpacked: ", unpacked_message)
                    # Guardar mensaje en la base datos con la data recibida
                    self.create_data_row(unpacked_message)
                
                    # El cliente ya recibió el header, por lo que 'datos' contiene la información del protocolo actual
                    # table_data = self.parse_body(data)
                    #Datos.create(**table_data) # insertar datos en la base de datos
                    # print(table_data)
                    config = self.config.get()
                    header = self.parse_header()
                    # Enviar la configuración
                    self.socket_UDP.sendto(header, addr)
                    if config['transport_layer'] != 1:
                        self.socket_TCP.settimeout(0)
                        break
            except:
                pass

            

s = Server()
s.run()
