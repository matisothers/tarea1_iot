from peewee import *
from playhouse.postgres_ext import ArrayField
import datetime

# Configuración de la base de datos
db_config = {
    'host': '0.0.0.0', 
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
    header_mac = CharField(max_length=255)
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
    acc_x = ArrayField(FloatField, null=True)
    acc_y = ArrayField(FloatField, null=True)
    acc_z = ArrayField(FloatField, null=True)

    rgyr_x = ArrayField(FloatField, null=True)
    rgyr_y = ArrayField(FloatField, null=True)
    rgyr_z = ArrayField(FloatField, null=True)
    
    timestamp = TimestampField()
    id_device = CharField(null=True)

class Logs(BaseModel):
    id_device = CharField()
    transport_layer = IntegerField(null=True)
    id_protocol = IntegerField(null=True)
    timestamp = DateTimeField()

class Configuration(BaseModel):
    id_protocol = IntegerField()
    transport_layer = IntegerField()
    
    @classmethod
    def toggle_protocol(cls, id_protocol, transport_layer):
        row = cls.get(1)
        row.id_protocol = id_protocol
        row.transport_layer = transport_layer
        row.save()

class Loss(BaseModel):
    delay = IntegerField() # ms
    packet_loss = IntegerField()


def create_tables():
    with db:
        db.create_tables([Datos, Logs, Configuration, Loss])
        print("tablas creadas")


# Ahora puedes definir tus modelos específicos heredando de BaseModel
# y db estará conectado al servicio de PostgreSQL cuando realices operaciones de base de datos.


## Ver la documentación de peewee para más información, es super parecido a Django

# TODO: 
# Crear tablas Datos, Logs, Configuration, Loss
# Crear función para modificar la tabla "Configuration" 2 (done?)
# 
