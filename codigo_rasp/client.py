import socket
from enum import Enum
import struct
import time


class TL(Enum):
    TCP = 'TCP'
    UDP = 'UDP'


class Client:
    def __init__(self, transport_layer:TL= TL.TCP, host:str='127.0.0.1', port:int=1234, buff_size:int=1024) -> None:
        self.transport_layer = transport_layer
        self.host = host
        self.port = port
        self.address = (host, port)
        self.buff_size = buff_size
        self.socket = None
        self.set_socket()
    
    def set_socket(self) -> None:
        print("Changing socket protocol")
        if self.socket:
            self.socket.close()
            self.socket = None
        if self.transport_layer == TL.TCP:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect(self.address)
        else:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        print(f"{self.transport_layer.name} socket ready to send data")

    def send_tcp(self, msg):
        self.socket.sendall(msg.encode('utf-8'))
        response = self.socket.recv(self.buff_size)
        print(response.decode('utf-8'))

    def send_udp(self, msg):
        # enviar mensaje vacío para comenzar la conexión
        
        # recibir header con la configuración

        # while true para enviar datos hasta que cambie el transport layer
        pass

    def run(self):
        if self.transport_layer == TL.TCP:
            self.send_tcp("Hola mundo")
        else:
            self.send_udp("Hola mundo")

#c = Client()
#c.run()

"""sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
connected = False
while True:
    if not connected:
        sock.sendto(b'', ('127.0.0.1', 1234))
        connected=True
    header = sock.recv(1024)
    msg = struct.unpack('<H6sBBH', header)
    print(msg)
    sock.sendto(b'test', ('127.0.0.1', 1234))
    time.sleep(1)"""

# Define the host and port to listen on
host = '127.0.0.1'  # Use '0.0.0.0' to listen on all available network interfaces
port = 1234

# Create a socket object
socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

socket.connect((host, port))
header = socket.recv(1024)
msg = struct.unpack('<H6sBBH', header)
print(msg)

while True:
    # enviar protocolo
    socket.sendall(b"Hello, world")
    time.sleep(1)
    header = socket.recv(1024)
    msg = struct.unpack('<H6sBBH', header)
    print(msg)
    # if transport-layer != TCP
        # me cambio de socket

    