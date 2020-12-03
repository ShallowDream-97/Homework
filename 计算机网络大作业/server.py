import socket
import time


def  send_msg(udp_socket_send,dest_ip,dest_port,send_data):
	"""发送消息，根据接收到的报文头来转发消息"""
	#获取要发送的内容	
	udp_socket_send.sendto(send_data,(dest_ip,int(dest_port)))
	print("转发完毕")

def recv_msg(udp_socket_recv):
	"""接收消息，获取目标IP地址和内容"""

	recv_data = udp_socket_recv.recvfrom(1024)
	recv_packet = recv_data[0].split( )

	print("成功获取包")

	dest_ip = recv_packet[0]#获取到字符串格式的目标ip
	dest_port = recv_packet[1]#获取到目标port
	recv_content = recv_packet[2]#获取内容

	print("解析包完毕")

	return dest_ip,dest_port,recv_packet[2]

def turn_to(udp_socket_recv,udp_socket_send):
	"""转发函数，利用send_msg和recv_msg来实现转发"""
	
	print("接收信息中")
	dest_ip,dest_port,send_data = recv_msg(udp_socket_recv)
	print("转发中")
	send_msg(udp_socket_send,dest_ip,dest_port,send_data)


def main():
	"""服务端应当接收来自客户端的信息，并将此信息转发给另一个客户端"""

	print("----------计网信息工具开发版------------")
	print("开始创建udp套接字")
	
	#创建一个套接字
	udp_socket_recv = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
	udp_socket_send = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
	
	print("udp套接字创建成功")
	print("开始套接字发送窗口绑定")
	#绑定信息
	udp_socket_recv.bind(("",7788))
	udp_socket_send.bind(("",8899))

	
	print("进入通道")

	#利用循环来处理
	while True:
		print("进入工作状态")
		#接收其中一个客户端的信息
		turn_to(udp_socket_recv,udp_socket_send)
		time.sleep(1)

if __name__ == "__main__":
	main()