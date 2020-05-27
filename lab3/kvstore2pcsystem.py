import socket
import sys
import threading
import time

# Read Conf file to bind ip:port
conf = []
ip = []
port = []
participants = {}
par_num = 0


# 获取时间
def get_time():
    return time.strftime('%Y.%m.%d %H:%M:%S', time.localtime(time.time()))


# 格式化输出
def p(sign, thread_id, data):
    print(sign, "time:", get_time(), " thread id:", thread_id, " data:", data)


# 读取配置文件
def R_Conf(argv, ROLE=0, par_num=0):
    conf_file_name = argv[2]
    for line in open(conf_file_name):
        line = line.strip('\n')
        conf.append(line)
    role = conf[0].split(' ')
    if role[1] == "participant":
        ROLE = 0
        ip_port_arr = conf[1].split(' ')
        ip_port = ip_port_arr[1].split(':')
        ip.append(ip_port[0])
        port.append(ip_port[1])
    elif role[1] == "coordinator":
        par_num = len(conf) - 2
        ROLE = 1
        for x in range(par_num + 1):
            ip_port_arr = conf[x + 1].split(' ')
            ip_port = ip_port_arr[1].split(':')
            ip.append(ip_port[0])
            port.append(ip_port[1])
    return ROLE


# ping心跳包
def pingHeartBeat(s, srcip, par_num):
    participant_ip = srcip[0]
    participant_port = str(srcip[1])
    id = participant_port
    thread_id = threading.currentThread().ident
    while True:
        try:
            print(participants)
            if participants[id] == 1:  # 参与者存活才发送心跳包
                s.sendto('+PING\\r\\n'.encode('utf-8'), (participant_ip, int(participant_port) + 1000))
                p('[->]', thread_id,
                  '+PING\\r\\n' + " " + participant_ip + ":" + str(int(participant_port) + 1000))
                resp = s.recvfrom(1024)
                p('[<-]', thread_id, resp)
            time.sleep(8)  # 隔8秒发送一次
        except socket.timeout:
            print("participant " + str(srcip[1]) + "down...")
            participants[id] = 0  # 节点下线
            par_num -= 1


# pong心跳包
def pongHeartBeat(ip, port):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("127.0.0.1", int(port) + 1000))
    s.settimeout(12)  # 超时

    thread_id = threading.currentThread().ident # 获取当前线程ID
    while True:
        message, srcip = s.recvfrom(1024)
        p("[<-]", thread_id, message)
        data = '+PONG Participant ' + port + '\\r\\n' # 发送PONG心跳包
        s.sendto(data.encode(), srcip)
        p('[->]', thread_id, data.encode())


if __name__ == '__main__':

    # ROLE=1:Coordinator;ROLE=0:Participant
    # par_num stand for the number of participant
    ROLE = R_Conf(sys.argv, 0, 0)
    thread_id = threading.currentThread().ident

    if ROLE == 0:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.bind((ip[0], int(port[0])))
        KV_Store = {}

        # 参与者上线
        data = "+UP Participant " + port[0]
        s.sendto(data.encode(), ("127.0.0.1", 8001))
        p("[->]", thread_id, data.encode())
        pongThead = threading.Thread(target=pongHeartBeat, args=(ip[0], port[0],))  # 回复心跳包
        pongThead.start()

        while True:
            # State1:init
            message, srcip = s.recvfrom(1024)
            p("[<-]", thread_id, message.decode() + " " + srcip[0] + ":" + str(srcip[1]))

            # State2:get request
            if message.decode('utf-8') == "Are u prepared":
                s.sendto("Yes".encode('utf-8'), srcip)

            # State3:commit/operate
            para = s.recv(1024).decode('utf-8').split()
            p("[<-]", thread_id, para)
            # Set
            if para[0].upper() == "SET":
                KV_Store[para[1]] = para[2]
            if para[0].upper() == 'GET' and para[-1] == "0":
                if para[1] in KV_Store.keys():
                    s.sendto(KV_Store[para[1]].encode('utf-8'), srcip)
                else:
                    s.sendto(("nil").encode('utf-8'), srcip)
            if para[0].upper() == 'DEL':
                z = 0
                i = 1
                while i < len(para):
                    if para[1] in KV_Store.keys():
                        del KV_Store[para[i]]
                        z += 1
                        i += 1
                    else:
                        i += 1
                        continue
                if para[-1] == "0":
                    s.sendto(str(z).encode('utf-8'), srcip)
            print("KV_Store:", KV_Store)
    elif ROLE == 1:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s_connect_participant = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.bind((ip[0], int(port[0])))

        s_ping = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s_ping.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s_ping.bind(("127.0.0.1", 10000))
        s_ping.settimeout(12)  # 超时

        while True:
            # State1:initial
            message, srcip = s.recvfrom(1024)  # srcip 客户端的ip地址
            p("[<-]", thread_id, message.decode() + " " + srcip[0] + ":" + str(srcip[1]))
            # State2:2pc
            response = 1

            # 参与者上线
            para = message.decode('utf-8').split()
            if para[0] == "+UP":
                id = para[2]
                participants[id] = 1
                par_num += 1
                pingThread = threading.Thread(target=pingHeartBeat, args=(s_ping, srcip, par_num,))  # 开始心跳检测
                pingThread.start()
            else:
                # 协商
                count = 0
                for x in range(par_num):
                    s_connect_participant.sendto("Are u prepared".encode('utf-8'), (ip[x + 1], int(port[x + 1])))
                    p('[->]', thread_id, "Are u prepared" + " " + ip[x + 1] + ":" + port[x + 1])
                    temp_m = s_connect_participant.recv(1024).decode('utf-8')
                    p('[<-]', thread_id, temp_m)
                    if temp_m == "No": # 有参与者不同意，则放弃这次交易
                        response = 0

            # State3:Abort or Commit
            # Abort
            if response == 0:
                s.sendto('-Error\\r\\n'.encode('utf-8'), srcip)
            # Commit
            else:
                if para[0].upper() == 'SET':
                    s.sendto('+OK\\r\\n'.encode('utf-8'), srcip)
                # contact participants
                for x in range(par_num):
                    if para[0].upper() == "GET":
                        only_get_once = message.decode('utf-8') + " " + str(x)
                        s_connect_participant.sendto(only_get_once.encode('utf-8'), (ip[x + 1], int(port[x + 1])))
                        p('[->]', thread_id, only_get_once + " " + ip[x + 1] + ":" + port[x + 1])
                        if x == 0:  # 暂定将第一个参与者发回的信息发送给client
                            get_value = s_connect_participant.recv(1024).decode('utf-8')
                            p('[<-]', thread_id, get_value)
                            s1 = get_value.split(' ')
                            response_code = '*' + str(len(s1)) + '\\r\\n'
                            for x in s1:
                                response_code += '$' + str(len(x)) + '\\r\\n' + x + '\\r\\n'
                            s.sendto(response_code.encode('utf-8'), srcip)  # 发回给client
                            p('[->]', thread_id, response_code + " " + srcip[0] + ":" + str(srcip[1]))
                    elif para[0].upper() == 'SET':
                        s_connect_participant.sendto(message.decode('utf-8').encode('utf-8'), (ip[x + 1], int(port[x + 1])))
                        p('[->]', thread_id, message.decode('utf-8') + " " + srcip[0] + ":" + str(srcip[1]))
                    elif para[0].upper() == 'DEL':
                        only_get_once = message.decode('utf-8') + " " + str(x)  # DEL A 0
                        s_connect_participant.sendto(only_get_once.encode('utf-8'), (ip[x + 1], int(port[x + 1])))
                        p('[->]', thread_id, only_get_once + " " + srcip[0] + ":" + str(srcip[1]))
                        if x == 0:  # 暂定将第一个参与者发回的信息发送给client
                            get_value = s_connect_participant.recv(1024).decode('utf-8')
                            p('[<-]', thread_id, get_value)
                            response_code = ':' + get_value + '\\r\\n'
                            s.sendto(response_code.encode('utf-8'), srcip)  # 发回给client
                            p('[->]', thread_id, response_code + " " + srcip[0] + ":" + str(srcip[1]))
