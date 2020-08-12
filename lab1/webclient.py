import sys
import os
from socket import *

serverName=sys.argv[1] 
serverPort=int(sys.argv[2]) 
if(len(sys.argv) > 3):
	filename=sys.argv[3]
	if(filename == '/'):
		filename = ''
else:
	filename=''
split_filename = filename.split('.')
file_suffix = split_filename[len(split_filename)-1]

txt_size = 1024
image_size = 512

command='GET /'
http_version = ' HTTP/1.1\r\n'
host = 'Host: ' + str(serverName) + '\r\n'
request_head = command + filename + http_version + host

clientSocket=socket(AF_INET,SOCK_STREAM)
clientSocket.connect((serverName,serverPort)) 

clientSocket.send(request_head.encode()) 


response = clientSocket.recv(txt_size)
print(response.split('\r\n')[0])
response_number = int(response.split('\r\n')[0].split()[1])
#print(response)
#print(response_number)
if(response_number == 404 or response_number == 400 or response_number == 505):
	pass
elif(response_number == 200):
	if(filename == ''): filename = 'index.html'
	if(file_suffix == 'txt' or file_suffix == 'html'):
		if(not os.path.isdir('Download')):
			os.mkdir('Download')
		with open(os.path.join('./Download', filename), 'w') as file_to_write:
			file_to_write.write(str(response[len(response.split('\r\n\n')[0])+len('\r\n\n'):]))	
			while True:
				data = clientSocket.recv(txt_size)
				if not data:
					break
				file_to_write.write(data)
		file_to_write.close()
	else:
		if(not os.path.isdir('Download')):
			os.mkdir('Download')
		with open(os.path.join('./Download', filename), 'w') as img:
		#img = open(filename,'w')
			img.write(str(response[len(response.split('\r\n\n')[0])+len('\r\n\n'):]))	
			while True:
				img_data = clientSocket.recv(txt_size)
				if not img_data:
					break
				img.write(img_data)
		img.close()
else:
	print("Unrecognized response!")
clientSocket.close()