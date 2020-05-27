import socket
import select

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.settimeout(5)

while True:
    try:
        # State1:init
        message = input('Client Sending:')
        if message == "":  # 非法输入
            continue
        s.sendto(message.encode('utf-8'), ('127.0.0.1', 8001))

        # State2:Get response
        resp = s.recv(1024)
        if len(resp) == 0:
            continue
        else:
            print(resp.decode('utf-8'))
    except Exception as e:
        print(repr(e))
