from socket import *
import sys
import os
#import time

#get port number
PORT = int(sys.argv[1])
connect_number = 5
path = './Upload'

#index to get information from received message
CMD = 0
FILENAME = 1
HTTP_VERSION = 2

#result to execute command
FAILED = 0
SUCCEEDED = 1
BAD_CMD = 2
UNSUPPORTED_VERSION = 3
DENIED = 4

def main():
	print("=========open===========")

	#set up server socket
	serverSocket = socket(AF_INET, SOCK_STREAM) 
	serverSocket.bind(('', PORT))
	serverSocket.listen(connect_number) 

	print("Server is ready, port: " + str(PORT))

	#multithread:get parent process information
	print('Parent PID (PPID): {pid}\n'.format(pid=os.getpid()))

	while True:
		#set up connection
		connectionSocket, addr = serverSocket.accept()

		#multithread:get process id
		pid = os.fork()

		if pid == 0:
			serverSocket.close()
			formatted_message = get_message(connectionSocket)
			try:
				#get http version
				#print(formatted_message[HTTP_VERSION])
				request_version = formatted_message[HTTP_VERSION].split('/')[1]
				
				if(request_version != '1.1' and request_version != '1.0'):
					unsupported_httpversion(connectionSocket,formatted_message[HTTP_VERSION])
				else:
					#parse received message
					command = formatted_message[CMD]
					exe_result = exc_cmd(command,formatted_message,connectionSocket)

					if(exe_result == FAILED):
						#if there are some problems opening the file, return 404
						not_found(connectionSocket,formatted_message[HTTP_VERSION])
					elif(exe_result == BAD_CMD):
						bad_request(connectionSocket,formatted_message[HTTP_VERSION])
					elif(exe_result == DENIED):
						permission_denied(connectionSocket,formatted_message[HTTP_VERSION])
			except IOError:
				bad_request(connectionSocket,formatted_message[HTTP_VERSION])
					
			os._exit(0)
		else:
			connectionSocket.close()

	print("=========close==========")

#after receiving request, set up new TCP connection
def set_up_connection(serverSocket):
	connectionSocket, addr = serverSocket.accept()
	return connectionSocket, addr

#get message after connection is built
def get_message(connection_socket):
	message = connection_socket.recv(1024)

	print(
		'Child PID: {pid}. Parent PID {ppid}'.format(
			pid=os.getpid(),
			ppid=os.getppid(),
		)
	)

	print("Message content: " + message)
	#transfer message to array format with arr split_message=[cmd,filename,http version]
	split_message = message.split()
	#print(split_message)
	return split_message

#get command and execute based on protocols
def exc_cmd(cmd,formatted_message,connection_socket):
	result = FAILED
	if(cmd == 'GET'):
		request_file = formatted_message[FILENAME]
		split_request_file = request_file.split('.')
		#print(split_request_file)
		file_suffix = split_request_file[len(split_request_file)-1]

		if(request_file == '/'):
			request_file = '/index.html'
			file_suffix = 'html'
		
		try:
			if(os.access(path+request_file,os.R_OK)):
				if(file_suffix == 'txt' or file_suffix == 'html'):
					f = open(path+request_file,'rb')

					output = f.read()

					#Send one HTTP header to socket
					#ok_header = 'HTTP/1.1 200 OK\r\n'
					ok_header = ' HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\n' % (len(output))
			
					print(ok_header)
					connection_socket.send(ok_header.encode())
					
					#connection_socket.send(output.encode())
					connection_socket.send(output.encode())
					#time.sleep(30)
					connection_socket.close()
					
					#return whether read is successful
					result = SUCCEEDED
					return result
				else:
					img = open(path+request_file,'rb')

					#ok_header = 'HTTP/1.1 200 OK\r\n'
					
					img_data = img.read()
					ok_header = ' HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Type: image/jpeg/gif\r\nContent-Length: %d\r\n\n' % (len(img_data))
					
					print(ok_header)
					connection_socket.send(ok_header.encode())

					connection_socket.send(img_data)
					#time.sleep(30)

					#while True:
					#	img_data = img.readline(512)
					#	if not img_data:
					#		break
					#	connection_socket.send(img_data)
					img.close()

					connection_socket.close()

					#return whether read is successful
					result = SUCCEEDED
					return result
			else:
				result = DENIED
				return result
		except IOError:
			result = FAILED
			return result
		
		#close the connection, and the while loop will open a new connection
		
		#return whether read is successful
		result = FAILED
		return result
	else:
		#if command is not recognized, return failed
		
		result = BAD_CMD
		return result

#handle situation of not found 404
def not_found(connection_socket,http_vers):
	#create header message to send
	not_found_header = http_vers + ' 404 Not Found\r\n\n'
	print(not_found_header)
	connection_socket.send(not_found_header.encode())
	#close socket
	connection_socket.close()

def permission_denied(connection_socket,http_vers):
	#create header message to send
	not_found_header = http_vers + ' 403 Forbidden\r\n\n'
	connection_socket.send(not_found_header.encode())
	connection_socket.close()

#handle situation of bad request 400
def bad_request(connection_socket,http_vers):
	#create header message to send
	bad_request_header = http_vers + ' 400 Bad Request\r\n\n'
	print(bad_request_header)
	connection_socket.send(bad_request_header.encode())
	#close socket
	connection_socket.close()

#handle situation of unsupported version
def unsupported_httpversion(connection_socket,http_vers):
	#create header message to send
	bad_request_header = http_vers + ' 505 HTTP Version Not Supported\r\n\n'
	print(bad_request_header)
	connection_socket.send(bad_request_header.encode())
	#close socket
	connection_socket.close()

if __name__ == "__main__":
	main()
