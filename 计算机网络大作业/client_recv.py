import socket


def recv_msg(udp_socket_recv):
	"""接收消息"""
	recv_data = udp_socket.recvfrom(1024)
	print("%s:%s" % (str(recv_data[1]),recv_data[0].decode("utf-8")))


def main():
	"""client_recv用于实现用户端对服务端的全时监听"""
	udp_socket_recv = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

	#绑定信息
	udp_socket_recv.bind(("",7788))

	#循环来保持监听
	while True:
		recv_msg(udp_socket_recv)



if __name__ == "__main__" :
	main()