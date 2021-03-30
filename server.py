#!/usr/bin/env python3
import os
from http.server import BaseHTTPRequestHandler, HTTPServer


host_name = 'localhost'    # Change this to your Raspberry Pi IP address
host_port = 8000


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
        html = '''
            <html>
            <body style="width:960px; margin: 20px auto;">
            <h1>Welcome to NAZBERT</h1>
            <p>Turn FLAMING DEATH: <a href="/on">On</a> <a href="/off">Off</a></p>
            <div id="led-status"></div>
            <script>
                document.getElementById("led-status").innerHTML="{}";
            </script>
            </body>
            </html>
        '''
        self.do_HEAD()
        status = ''
        if self.path=='/':
            pass
        elif self.path=='/on':
            #GPIO.output(18, GPIO.HIGH)
            status='DEATH is On'
        elif self.path=='/off':
            #GPIO.output(18, GPIO.LOW)
            status='DEATH is Off'
        self.wfile.write(html.format(status).encode("utf-8"))

if __name__ == '__main__':
    http_server = HTTPServer((host_name, host_port), MyServer)
    print("Server Starts - %s:%s" % (host_name, host_port))

    try:
        http_server.serve_forever()
    except KeyboardInterrupt:
        http_server.server_close()
