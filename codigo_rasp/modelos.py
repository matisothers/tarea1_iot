from peewee import *
from playhouse.postgres_ext import ArrayField


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
    # HEADER
    header_id = IntegerField()
    mac = IntegerField()
    transport_layer = CharField(max_length=3)
    id_protocol = IntegerField()
    length = IntegerField()

    # BODY
    batt_level = IntegerField(null=True)
    temp = IntegerField(null=True)
    press = IntegerField(null=True)
    hum = IntegerField(null=True)
    co = FloatField(null=True)
    rms = FloatField(null=True)
    amp_x = FloatField(null=True)
    frec_x = FloatField(null=True)
    amp_y = FloatField(null=True)
    frec_y = FloatField(null=True)
    amp_z = FloatField(null=True)
    frec_z = FloatField(null=True)

    # Arrays
    acc_x = ArrayField()
    acc_y = ArrayField()
    acc_z = ArrayField()

    rgyr_x = ArrayField()
    rgyr_y = ArrayField()
    rgyr_z = ArrayField()
    
    timestamp = TimestampField()
    id_device = CharField()
    mac = CharField()

class Logs(BaseModel):
    id_device = CharField()
    transport_layer = CharField(max_length=3)
    protocol_id = IntegerField()
    timestamp = TimestampField()

class Configuration(BaseModel):
    id_protocol = IntegerField()
    transport_layer = CharField(max_length=3)
    
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