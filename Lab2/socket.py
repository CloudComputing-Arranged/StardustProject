import socket
from multiprocessing import Process

# http server
def handleClient(clientSocket):
    # 用一个新的进程，为一个客户端进行服务
    # 接受客户端的请求
    recvData = clientSocket.recv(2014)
    requestHeaderLines = recvData.splitlines()
    for line in requestHeaderLines:
        print(line)

    # http内容
    responseHeaderLines = "HTTP/1.1 200 OK\r\n"
    responseHeaderLines += "\r\n"
    # clientIp = clientSocket.getsockname()
    responseBody = "Hello world"

    response = responseHeaderLines + responseBody
    # 发送http内容给客户端
    clientSocket.send(response.encode())


def main():
    # 创建一个TCP套接字
    serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # 端口复用，先留个坑
    serverSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # tcp套接字绑定端口
    serverSocket.bind(("127.0.0.1", 7788))  # 如果host是"", 则说明允许来自一切主机的连接
    # socket的等待数为10
    serverSocket.listen(10)
    # 死循环处理连接
    while True:
        # 接收到客户端的连接
        clientSocket, clientAddr = serverSocket.accept()
        # 启动一个新的进程来处理这个连接
        clientP = Process(target=handleClient, args=(clientSocket,))
        clientP.start()
        # 关闭套接字
        clientSocket.close()


if __name__ == '__main__':
    main()
