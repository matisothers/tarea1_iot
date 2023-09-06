from peewee import *

# Configuración de la base de datos
db_config = {
    'host': 'localhost', 
    'port': 5432, 
    'user': 'postgres', 
    'password': 'postgres', 
    'database': 'iot_db'
}
db = PostgresqlDatabase(**db_config)

# Definición de un modelo
class BaseModel(Model):
    class Meta:
        database = db

class Datos(BaseModel):
    timestamp = TimestampField()
    id_device = CharField()
    mac = CharField()

class Logs(BaseModel):
    id_device = CharField()
    transport_layer = CharField(max_length=3)
    timestamp = TimestampField()

class Configuration(BaseModel):
    id_protocol = IntegerField()
    transport_layer = CharField(max_length=3)
    
    @classmethod
    def get(cls):
        return cls.get(1)
    
    @classmethod
    def toggle_protocol(cls, id_protocol, transport_layer):
        row = cls.get(1)
        row.id_protocol = id_protocol
        row.transport_layer = transport_layer
        row.save()


class Loss(BaseModel):
    delay = IntegerField() # ms
    packet_loss = IntegerField()



# Ahora puedes definir tus modelos específicos heredando de BaseModel
# y db estará conectado al servicio de PostgreSQL cuando realices operaciones de base de datos.


## Ver la documentación de peewee para más información, es super parecido a Django

# TODO: 
# Crear tablas Datos, Logs, Configuration, Loss
# Crear función para modificar la tabla "Configuration" 2 (done?)
# 