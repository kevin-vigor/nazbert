#!/usr/bin/env python3
import os
from http.server import BaseHTTPRequestHandler, HTTPServer
import sys


host_name = 'localhost'    # Change this to your Raspberry Pi IP address
host_port = 8000

def blatterCommand(cmd):
    try:
        with open("/dev/shm/pounceblat.fifo", 'wb', 0) as fifo:
            fifo.write(cmd)
            #print(b'D', file=fifo, end='')
    except Exception as e:
        print(f"Error sending command to pouceblat: {e}") 

def blatterStatus():
    try:
        with open("/dev/shm/pounceblat.status", 'r') as status:
            return status.read()
    except Exception as e:
        print(f"Error reading pouceblat status: {e}") 
        return "Unknown."

class MyServer(BaseHTTPRequestHandler):
    """ A special implementation of BaseHTTPRequestHander for reading data from
        and control GPIO of a Raspberry Pi
    """

    def do_HEAD(self):
        """ do_HEAD() can be tested use curl command 
            'curl -I http://server-ip-address:port' 
        """
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_GET(self):
        """ do_GET() can be tested using curl command 
            'curl http://server-ip-address:port' 
        """


        html = f'''
            <html>
            <body>
            <h1>Welcome to NAZBERT</h1>
            <p>Status</p>
            <pre>{blatterStatus()}</pre>            
            <p>Turn FLAMING DEATH: <a href="/on">On</a> <a href="/off">Off</a></p>
            </body>
            </html>
        '''
        self.do_HEAD()
        status = ''
        if self.path=='/':
            pass
        elif self.path=='/on':
            blatterCommand(b'E') # enable
        elif self.path=='/off':
            blatterCommand(b'D') # disable
        self.wfile.write(html.format(status).encode("utf-8"))

if __name__ == '__main__':
    http_server = HTTPServer(("", host_port), MyServer)
    print("Server Starts - %s:%s" % (host_name, host_port))

    try:
        http_server.serve_forever()
    except KeyboardInterrupt:
        http_server.server_close()
