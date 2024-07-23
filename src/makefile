all:navClient navServer

client:navClient.cpp
	g++ -g -o navClient navClient.cpp

navServer:navServer.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Acceptor.cpp Connection.cpp Buffer.cpp Navigation.cpp ThreadPool.cpp	TimeStamp.cpp
	g++ -g -o navServer navServer.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Acceptor.cpp Connection.cpp Buffer.cpp Navigation.cpp ThreadPool.cpp	TimeStamp.cpp -lpthread

clean:
	rm -f navClient navServer
