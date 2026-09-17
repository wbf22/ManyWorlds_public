
# Usage

These files are used to communicate between the client and the server.

To use either the client of the server you first have to initialize sockets on windows. (weird)
So first should use the `WindowsInit.hpp` class to intialize that and then call the cleanup method when
you're done using the sockets


Here's an example using our UDP client and server

```c++
void thread_func() {

    UDPSocket client = UDPSocket(1000, 8081);
    client.send(Data::SHRUG, "127.0.0.1", 8080);

}


int main() {

    // WindowsInit::init();


    // server
    UDPSocket server = UDPSocket(1000, 8080);

    // start client    
    thread other_thread(thread_func);

    // recieve message
    char buffer[512] = {0};
    int bytes = 0;
    sockaddr_in client = server.recieve(buffer, 512, bytes);

    // null terminate
    buffer[bytes] = '\0';

    cout << buffer << endl;
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(client.sin_family, &client.sin_addr, ipStr, sizeof(ipStr));
    cout << "IP Address: " << ipStr << endl;
    cout << "Port: " << ntohs(client.sin_port) << endl; // Convert network byte order to host byte order

    // clean up
    other_thread.join();
    // WindowsInit::cleanup();

}
```


And here's an example of using our TCP client and server

```c++

void thread_func() {

    this_thread::sleep_for(chrono::seconds(2));

    TCPSocket client = TCPSocket(10000);

    try {

        client.connect("127.0.0.1", 8080);
        
        const char* msg = Data::SHRUG;
        client.send(client.socket_num, msg, strlen(msg));

        char buffer[1024];
        int bytesReceived = client.receive(client.socket_num, buffer, sizeof(buffer)); // Receive data
        cout << "Received: " << string(buffer, bytesReceived) << endl;
    } catch (const runtime_error& e) {
        cerr << "Exception: " << e.what() << endl;
    }

}


int main() {

    WindowsInit::init();

    // server
    TCPSocket server = TCPSocket(10000);
    server.server_int(8080);

    // start client    
    thread other_thread(thread_func);

    try {

        // listen for connections
        sockaddr_in client;
        SOCKET clientSocket = server.acceptConnection(client); // Accept a connection

        // recieve message
        char buffer[1024];
        int bytesReceived = server.receive(clientSocket, buffer, sizeof(buffer)); // Receive data
        cout << "Received: " << string(buffer, bytesReceived) << endl;
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(client.sin_family, &client.sin_addr, ipStr, sizeof(ipStr));
        cout << "IP Address: " << ipStr << endl;
        cout << "Port: " << ntohs(client.sin_port) << endl; // Convert network byte order to host byte order

        // send message
        const char* msg = "Hello from server";
        server.send(clientSocket, msg, strlen(msg)); // Send data
    } catch (const runtime_error& e) {
        cerr << "Exception: " << e.what() << endl;
    }

    // clean up
    other_thread.join();
    WindowsInit::cleanup();

}

```




# Notes

Here's an online library we tried but didn't get to work https://github.com/simondlevy/CppSockets.git


Here's chat gpt's example that got me working:
```c++
#include <iostream>
#include "Server.h"
#include "../server/sockets/WindowsInit.hpp"
#include "../server/sockets/UDPClient.hpp"
#include "../server/sockets/UDPServer.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")




using namespace std;




void thread_func() {
    // SET UP CLIENT
    SOCKET sockfd_client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd_client == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        WSACleanup();
        exit(5);
    }

    // Assuming sockfd_client is the client socket and it's already set up
    struct sockaddr_in si_server;
    int slen2 = sizeof(si_server);
    ZeroMemory(&si_server, slen2);
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(3490); // Port number to send data to
    inet_pton(AF_INET, "127.0.0.1", &si_server.sin_addr); // Assuming you're sending to localhost

    const char *message = "Hello, World!";
    if (sendto(sockfd_client, message, strlen(message), 0, (struct sockaddr *) &si_server, slen2) == SOCKET_ERROR) {
        std::cerr << "sendto() failed with error code : " << WSAGetLastError() << std::endl;
    }
    closesocket(sockfd_client);
}


int main() {

    // INIT WINSOCK
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        exit(1);
    }


    // SET UP SERVER
    struct addrinfo hints, *res;
    SOCKET sockfd;

    // Zero out hints
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    hints.ai_flags = AI_PASSIVE; // Use my IP

    // Get address info
    if (getaddrinfo(NULL, "3490", &hints, &res) != 0) {
        std::cerr << "getaddrinfo failed" << std::endl;
        WSACleanup();
        exit(2);
    }

    // Create a socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        freeaddrinfo(res);
        WSACleanup();
        exit(3);
    }

    // Bind socket
    if (bind(sockfd, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(res);
        closesocket(sockfd);
        WSACleanup();
        exit(4);
    }

    freeaddrinfo(res);




    std::thread other_thread(thread_func);


    // Assuming sockfd is the server socket and it's already set up
    struct sockaddr_in si_client;
    int slen = sizeof(si_client);
    char buf[512];
    if (recvfrom(sockfd, buf, 512, 0, (struct sockaddr *) &si_client, &slen) == SOCKET_ERROR) {
        std::cerr << "recvfrom() failed with error code : " << WSAGetLastError() << std::endl;
    }

    cout << buf << endl;


    
    closesocket(sockfd);
    WSACleanup();
    other_thread.join();


}

```



