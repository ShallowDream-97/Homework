import socket

def main():
	#创建一个udp套接字对象
	udp_socket = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

	#获取对方的IP：
	dest_ip = input("请输入对方的ip地址:")
	
	#获取对方的port：
	dest_port = int(input("请输入对方的port:"))
	
	#绑定本地信息：
	udp_socket.bind(("",7890))#绑定本地端口
	
	while True:
		#从键盘获取数据
		send_data = input("请输入要发送的数据：")

		#如果输入的数据是exit，那么就退出程序：
		if send_data == "exit":
			break
		#可以用套接字收发数据
		#udp_socket.sendto("hahaha",对方的ip以及port)，第二个参数用一个元组表示
		udp_socket.sendto(send_data.encode("utf-8"),(dest_ip,dest_port))#第一个参数要转换成字节类型

	#接受回送过来的数据
	recv_data = udp_socket.recvfrom(1024)
	#关闭套接字
	udp_socket.close()

if __name__ == "__main__":
	main()