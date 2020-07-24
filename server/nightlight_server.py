import struct
import time
import math
import datetime
from http.server import BaseHTTPRequestHandler
import paho.mqtt.client as mqtt
from pushover import Client

try:
    from local_settings import *
except:
    pass

TOPIC = 'nightlight'
SESSION_LENGTH = 30
FADEOUT = 5
TIC_INCREMENT = 1.0
MAX_VALUE = 1023

global current_session_end
current_session_end = -1
global last_value
last_value = -1

pushover_client = Client(PUSHOVER_USER, api_token=PUSHOVER_TOKEN)
pushover_client.send_message('nightlight server started')

class GetHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        global current_session_end
        global last_value

        if '/on' in self.path:
            current_session_end = time.time() + (SESSION_LENGTH * 60)
        elif '/off' in self.path:
            current_session_end = time.time() - 1
        last_value = -99

        self.send_response(200)
        self.send_header("Content-Type", "text/ascii")
        self.send_header("Content-Length", "2")
        self.end_headers()
        self.wfile.write("OK".encode("utf-8"))


# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe(TOPIC)

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    global current_session_end
    global last_value

    print('[{}] {}'.format(msg.topic, str(msg.payload)))

    if msg.topic != TOPIC:
        return

    if msg.payload == b'start':
        # no late night reading!
        hour = datetime.datetime.now().hour
        if hour <= 20 and hour >= 7:
            current_session_end = time.time() + (SESSION_LENGTH * 60)
            last_value = -99 # force send of new value
            print('starting new session, ends at {}'.format(current_session_end))
            pushover_client.send_message('nightlight session started')
        else:
            # turn off immediately
            print('forbidden hours, sending shutdown')
            current_session_end = time.time() - 1
            last_value = -99

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("127.0.0.1", 1883, 60)
client.loop_start()

from http.server import HTTPServer
server = HTTPServer(('0.0.0.0', 8173), GetHandler)
print('Starting server, use <Ctrl-C> to stop')
server.timeout = TIC_INCREMENT

while True:
    if current_session_end < 0:
        val = None
    elif current_session_end < time.time():
        val = -1
    else:
        remaining = (current_session_end - time.time()) / 60.0
        if remaining > FADEOUT:
            val = MAX_VALUE
        else:
            val = math.floor((remaining / (1.0 * FADEOUT)) * MAX_VALUE)

    if val is not None and val != last_value:
        val = min(val, MAX_VALUE)

        if val == -1:
            pushover_client.send_message('nightlight session ending')
            print('session ended, putting nightlight to sleep')

        last_value = val
        print('sending {} to {}'.format(val, TOPIC))
        client.publish(TOPIC, struct.pack('>i', val))

    # replaces sleep; times out after TIC_INCREMENT
    server.handle_request()
