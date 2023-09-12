import socket
from enum import Enum


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

    def send_udp(self, msg): pass

    def run(self):
        if self.transport_layer == TL.TCP:
            self.send_tcp("Hola mundo")
        else:
            self.send_udp("Hola mundo")

c = Client()
c.run()