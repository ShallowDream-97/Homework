import socket

def  send_msg(udp_socket_send):
	"""发送消息"""
	#获取要发送的内容
	server_ip = input("请输入服务器地址：")
	server_port = int(input("请输入服务器的port："))
	dest_ip = input("请输入对方的ip地址：")
	dest_port = input("请输入对方的port：")
	send_data = input("请输入要发送的信息：")	
	send_packet = dest_ip+"\n"+dest_port+"\n"+send_data
	print(send_packet)
	udp_socket_send.sendto(send_packet.encode("utf-8"),(server_ip,server_port))


def main():
	"""
	客户端要实现：
	1、将发送内容，和目标的ip与port打包发送给服务器
	2、接受来自服务器端的内容
	"""
	#创建套接字
	udp_socket_send = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

	#绑定信息
	udp_socket_send.bind(("",8899))#所有用户考虑均使用此端口


	#对send_msg函数进行一些修改后实现打包和发送
	while True:
		send_msg(udp_socket_send)
		print("发送成功")




if __name__ == "__main__":
	main()