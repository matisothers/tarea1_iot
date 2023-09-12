import socket
from enum import Enum
HOST = '127.0.0.1'  # Escucha en todas las interfaces disponibles
PORT = 1234       # Puerto en el que se escucha

address = (HOST, PORT)

# TODO:
# 3. Paquetes recibidos por la Raspberry Pi deben ser descompuestos y almacenados en la base de datos
# 4. En cada envío de datos, deberá verificarse la configuración de la base de datos.
# 5. Se deberá ajustar el protocolo de envío o el tipo de conexión.

# TL: Transport Layer
class TL(Enum):
    TCP = 'TCP'
    UDP = 'UDP'

# TODO: revisar si es necesario limitar la cantidad de conexiones (self.socket.listen(conns))
class Server:
    def __init__(self, transport_layer:TL= TL.TCP, host:str='127.0.0.1', port:int=1234, buff_size:int=1024) -> None:
        self.transport_layer = transport_layer
        self.host = host
        self.port = port
        self.socket = None
        self.buff_size = buff_size
        self.address = (self.host, self.port)
        self.set_socket()

    # Cerrar el socket actual para reemplazarlo por uno que utilice el nuevo protocolo
    def set_socket(self) -> None:
        print("Changing socket protocol")
        if self.socket:
            self.socket.close()
            self.socket = None
        if self.transport_layer == TL.TCP:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.bind(self.address)
            self.socket.listen()
        else:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket.bind(self.address)
        print(f"{self.transport_layer.name} socket waiting for connections at port {self.port}")
    
    # Si el protocolo cambia, también lo hace el socket
    def set_protocol(self, transport_layer:TL) -> None:
        if self.transport_layer != transport_layer:
            self.transport_layer = transport_layer
            self.set_socket()

    def run(self):
        if self.transport_layer == TL.TCP:
            self.tcp_handle()
        else:
            self.udp_handle()
    
    def tcp_handle(self):
        while True:
            conn = None
            try:
                conn, addr = self.socket.accept()
                with conn:
                    print(f'{addr} has connected')
                    data = conn.recv(self.buff_size)
                    # Aquí quizá una función handle_protocol(data)
                    if data:
                        # TODO: Revisar el protocolo y transport_layer de la base de datos
                        print(f"[CLIENT] {data.decode('utf-8')}")
                        response = f"[SERVER] Your message is: {data.decode('utf-8')}"
                        conn.sendall(response.encode('utf-8'))  # Envía la respuesta al cliente
            except KeyboardInterrupt:
                if conn:
                    conn.close()
                self.socket.close()
                break
        exit()

    def udp_handle(self): pass

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