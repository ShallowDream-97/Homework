import socket

def main():
	#1.创建套接字
	udp_socket = socket.socket(socket.AF_INET,socket_SOCK_DGRAM)
	#2.绑定一个本地信息(不一定需要绑定)
	localaddr = ("",7788)
	udp_socket.bind(localaddr)#必须绑定自己电脑的ip和port，其他的不行
	#3.接收数据
	while True:
		recv_data = udp_socket.recvfrom(1024)#参数是接收的最大长度
		#4.打印接收到的数据
		#print(recv_data)
		print("%s:%s" % (str(send_addr),recv_msg.decode("gbk")))#这里是因为windows发送的是gbk编码的信息
	#5.关闭套接字
	udp_socket.close()


if __name__ == "__main__":
	main()