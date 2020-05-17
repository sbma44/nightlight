import json
import math
import datetime
from http.server import BaseHTTPRequestHandler
from urllib import parse


class GetHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        parsed_path = parse.urlparse(self.path)
        message_parts = [
            'CLIENT VALUES:',
            'client_address={} ({})'.format(
                self.client_address,
                self.address_string()),
            'command={}'.format(self.command),
            'path={}'.format(self.path),
            'real path={}'.format(parsed_path.path),
            'query={}'.format(parsed_path.query),
            'request_version={}'.format(self.request_version),
            '',
            'SERVER VALUES:',
            'server_version={}'.format(self.server_version),
            'sys_version={}'.format(self.sys_version),
            'protocol_version={}'.format(self.protocol_version),
            '',
            'HEADERS RECEIVED:',
        ]
        for name, value in sorted(self.headers.items()):
            message_parts.append(
                '{}={}'.format(name, value.rstrip())
            )
        message_parts.append('')

        FADE_OUT = 5
        MAX_VAL = 1023
        d = datetime.datetime.now()
        hour = d.hour
        minute = d.minute
        if hour == 20:
            if minute < 50:
                val = MAX_VAL
            else:
                val = math.floor(((FADE_OUT - (minute - FADE_OUT)) / (FADE_OUT * 1.0)) * MAX_VAL)
        elif hour < 20:
            val = 0 # don't show anything but keep querying
        elif hour >= 21:
            val = -1 # sleep until reset


        message = json.dumps({'val' : int(val)})
        self.send_response(200)
        self.send_header('Content-Type',
                         'application/json; charset=utf-8')
        self.end_headers()
        self.wfile.write(message.encode('utf-8'))


if __name__ == '__main__':
    from http.server import HTTPServer
    server = HTTPServer(('0.0.0.0', 8331), GetHandler)
    print('Starting server, use <Ctrl-C> to stop')
    server.serve_forever()
