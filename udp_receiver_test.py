import socket

UDP_IP = "0.0.0.0"
UDP_PORT = 1234

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))

while True:
    data, addr = sock.recvfrom(65536) # buffer size can be resized to 1024 bytes if needed
    # Received data is in byte form, example line looks like this: b'2308,49l,-318'
    # Format is patient_id (integer), elapsed time (milliseconds), processed accelerometer signal (integer in units of g/10000)
    data=str(data).split("'")
    data=data[1]
    data=data.replace('l', '')
    print(data)
