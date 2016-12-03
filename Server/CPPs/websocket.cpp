/**
Team Members: Devon Gunn; ID# 28502436
**/
#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <ifaddrs.h>
#elif _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "websocket.h"
#include "base64.h"
#include "sha1.h"

using namespace std;

void showAvailableIP(){

#ifdef __linux__

    char name[INET_ADDRSTRLEN];
    struct ifaddrs *iflist;
    if (getifaddrs(&iflist) < 0){
        cout << "Error on getting available IP!" << endl;
    }

    cout << "Available IP:" << endl;

    struct in_addr addr;
    for (struct ifaddrs *p = iflist; p; p = p->ifa_next){
        if (p->ifa_addr->sa_family == AF_INET){
            addr = ((struct sockaddr_in*)p->ifa_addr)->sin_addr;
            if (inet_ntop(AF_INET, &addr, name, sizeof(name)) == NULL)
                continue;

            cout << "    " << p->ifa_name << " " << name << endl;
        }
    }

#elif _WIN32
    
    /* Variables used by GetIpAddrTable */
    PMIB_IPADDRTABLE pIPAddrTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    IN_ADDR IPAddr;

    /* Variables used to return error message */
    LPVOID lpMsgBuf;

    // Before calling AddIPAddress we use GetIpAddrTable to get
    // an adapter to which we can add the IP.
    pIPAddrTable = (MIB_IPADDRTABLE *) MALLOC(sizeof (MIB_IPADDRTABLE));

    if (pIPAddrTable) {
        // Make an initial call to GetIpAddrTable to get the
        // necessary size into the dwSize variable
        if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) ==
            ERROR_INSUFFICIENT_BUFFER) {
            FREE(pIPAddrTable);
            pIPAddrTable = (MIB_IPADDRTABLE *) MALLOC(dwSize);

        }
        if (pIPAddrTable == NULL) {
            printf("Memory allocation failed for GetIpAddrTable\n");
            return;
        }
    }
    // Make a second call to GetIpAddrTable to get the
    // actual data we want
    if ( (dwRetVal = GetIpAddrTable( pIPAddrTable, &dwSize, 0 )) != NO_ERROR ) { 
        printf("GetIpAddrTable failed with error %d\n", dwRetVal);
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),       // Default language
                          (LPTSTR) & lpMsgBuf, 0, NULL)) {
            printf("\tError: %s", lpMsgBuf);
            LocalFree(lpMsgBuf);
        }
        return;
    }

    cout << "Available IP:" << endl;

    for (int i = 0; i < (int) pIPAddrTable->dwNumEntries; i++) {
        IPAddr.S_un.S_addr = (u_long) pIPAddrTable->table[i].dwAddr;
        cout << "    " << i << ": " << inet_ntoa(IPAddr) << endl;
    }

    if (pIPAddrTable) {
        FREE(pIPAddrTable);
        pIPAddrTable = NULL;
    }

#endif
}

vector<int> webSocket::getClientIDs(){
    vector<int> clientIDs;
    for (int i = 0; i < wsClients.size(); i++){
        if (wsClients[i] != NULL)
            clientIDs.push_back(i);    
    }

    return clientIDs;
}

//our custom method to return client UserIDs
vector<string> webSocket::getClientUIDs(){
	vector<string> clientUIDs;
	for (int i = 0; i < wsClients.size(); i++){
		if (wsClients[i] != NULL)
			clientUIDs.push_back(wsClients[i]->getUID());
	}

	return clientUIDs;
}

//our custom method to set a client's UserID
void webSocket::setClientUID(int clientID, string clientUID){
	wsClients[clientID]->setUID(clientUID);
}

string webSocket::getClientIP(int clientID){
    return string(inet_ntoa(wsClients[clientID]->addr));
}

void webSocket::wsCheckIdleClients(){
    clock_t current = clock();
    for (int i = 0; i < wsClients.size(); i++){
        if (wsClients[i] != NULL && wsClients[i]->ReadyState != WS_READY_STATE_CLOSED){
            if (wsClients[i]->PingSentTime != 0){
                if (difftime(current, wsClients[i]->PingSentTime) >= WS_TIMEOUT_PONG){
                    wsSendClientClose(i, WS_STATUS_TIMEOUT);
                    wsRemoveClient(i);
                }
            }
            else if (difftime(current, wsClients[i]->LastRecvTime) != WS_TIMEOUT_RECV){
                if (wsClients[i]->ReadyState != WS_READY_STATE_CONNECTING) {
                    wsClients[i]->PingSentTime = clock();
                    wsSendClientMessage(i, WS_OPCODE_PING, "");
                }
                else
                    wsRemoveClient(i);
            }
        }
    }
}

bool webSocket::wsSendClientMessage(int clientID, unsigned char opcode, string message){
    // check if client ready state is already closing or closed
    if (clientID >= wsClients.size())
        return false;

	//maybe stop crash if its been nulled
	if (wsClients[clientID] == NULL)
	{
		return true;
	}
	else if (wsClients[clientID]->ReadyState == WS_READY_STATE_CLOSING || wsClients[clientID]->ReadyState == WS_READY_STATE_CLOSED)
        return true;

    // fetch message length
    int messageLength = message.size();

    // set max payload length per frame
    int bufferSize = 4096;

    // work out amount of frames to send, based on $bufferSize
    int frameCount = ceil((float)messageLength / bufferSize);
    if (frameCount == 0)
        frameCount = 1;

    // set last frame variables
    int maxFrame = frameCount - 1;
    int lastFrameBufferLength = (messageLength % bufferSize) != 0 ? (messageLength % bufferSize) : (messageLength != 0 ? bufferSize : 0);

    // loop around all frames to send
    for (int i = 0; i < frameCount; i++) {
        // fetch fin, opcode and buffer length for frame
        unsigned char fin = i != maxFrame ? 0 : WS_FIN;
        opcode = i != 0 ? WS_OPCODE_CONTINUATION : opcode;

        size_t bufferLength = i != maxFrame ? bufferSize : lastFrameBufferLength;
        char *buf;
        size_t totalLength;

        // set payload length variables for frame
        if (bufferLength <= 125) {
            // int payloadLength = bufferLength;
            totalLength = bufferLength + 2;
            buf = new char[totalLength];
            buf[0] = fin | opcode;
            buf[1] = bufferLength;
            memcpy(buf+2, message.c_str(), message.size());
        }
        else if (bufferLength <= 65535) {
            // int payloadLength = WS_PAYLOAD_LENGTH_16;
            totalLength = bufferLength + 4;
            buf = new char[totalLength];
            buf[0] = fin | opcode;
            buf[1] = WS_PAYLOAD_LENGTH_16;
            buf[2] = bufferLength >> 8;
            buf[3] = bufferLength;
            memcpy(buf+4, message.c_str(), message.size());
        }
        else {
            // int payloadLength = WS_PAYLOAD_LENGTH_63;
            totalLength = bufferLength + 10;
            buf = new char[totalLength];
            buf[0] = fin | opcode;
            buf[1] = WS_PAYLOAD_LENGTH_63;
            buf[2] = 0;
            buf[3] = 0;
            buf[4] = 0;
            buf[5] = 0;
            buf[6] = bufferLength >> 24;
            buf[7] = bufferLength >> 16;
            buf[8] = bufferLength >> 8;
            buf[9] = bufferLength;
            memcpy(buf+10, message.c_str(), message.size());
        }

        // send frame
        int left = totalLength;
        char *buf2 = buf;
        do {
			//this send uses winsock2.h
            int sent = send(wsClients[clientID]->socket, buf2, left, 0);
            if (sent == -1)
                return false;

            left -= sent;
            if (sent > 0)
                buf2 += sent;
        }
        while (left > 0);

        delete buf;
    }

    return true;
}

bool webSocket::wsSend(int clientID, string message, bool binary){
    return wsSendClientMessage(clientID, binary ? WS_OPCODE_BINARY : WS_OPCODE_TEXT, message);
}

void webSocket::wsSendClientClose(int clientID, unsigned short status){
    // check if client ready state is already closing or closed
    if (wsClients[clientID]->ReadyState == WS_READY_STATE_CLOSING || wsClients[clientID]->ReadyState == WS_READY_STATE_CLOSED)
        return;

    // store close status
    wsClients[clientID]->ReadyState = status;

    // send close frame to client
    wsSendClientMessage(clientID, WS_OPCODE_CLOSE, "");

    // set client ready state to closing
    wsClients[clientID]->ReadyState = WS_READY_STATE_CLOSING;
}

void webSocket::wsClose(int clientID){
    wsSendClientClose(clientID, WS_STATUS_NORMAL_CLOSE);
}

bool webSocket::wsCheckSizeClientFrame(int clientID){
    wsClient *client = wsClients[clientID];
    // check if at least 2 bytes have been stored in the frame buffer
    if (client->FrameBytesRead > 1) {
        // fetch payload length in byte 2, max will be 127
        size_t payloadLength = (unsigned char)client->FrameBuffer.at(1) & 127;

        if (payloadLength <= 125){
            // actual payload length is <= 125
            client->FramePayloadDataLength = payloadLength;
        }
        else if (payloadLength == 126){
            // actual payload length is <= 65,535
            if (client->FrameBuffer.size() >= 4){
                std::vector<unsigned char> length_bytes;
                length_bytes.resize(2);
                memcpy((char*)&length_bytes[0], client->FrameBuffer.substr(2, 2).c_str(), 2);

                size_t length = 0;
                int num_bytes = 2;
                for (int c = 0; c < num_bytes; c++)
                    length += length_bytes[c] << (8 * (num_bytes - 1 - c));
                client->FramePayloadDataLength = length;
            }
        }
        else {
            if (client->FrameBuffer.size() >= 10){
                std::vector<unsigned char> length_bytes;
                length_bytes.resize(8);
                memcpy((char*)&length_bytes[0], client->FrameBuffer.substr(2, 8).c_str(), 8);

                size_t length = 0;
                int num_bytes = 8;
                for (int c = 0; c < num_bytes; c++)
                    length += length_bytes[c] << (8 * (num_bytes - 1 - c));
                client->FramePayloadDataLength = length;
            }
        }

        return true;
    }

    return false;
}

void webSocket::wsRemoveClient(int clientID){
    //get the username of the person that disconnected
	string UID = wsClients[clientID]->getUID();

    wsClient *client = wsClients[clientID];

    // fetch close status (which could be false), and call wsOnClose
    int closeStatus = wsClients[clientID]->CloseStatus;

    // close socket
#ifdef __linux__
    close(client->socket);
#elif _WIN32
    closesocket(client->socket);
#endif
    FD_CLR(client->socket, &fds);

    socketIDmap.erase(wsClients[clientID]->socket);
    wsClients[clientID] = NULL;
    delete client;

	//now we call our callback
	if (callOnClose != NULL)
		callOnClose(clientID, UID);
}

bool webSocket::wsProcessClientMessage(int clientID, unsigned char opcode, string data, int dataLength){
    wsClient *client = wsClients[clientID];
    // check opcodes
    if (opcode == WS_OPCODE_PING){
        // received ping message
        return wsSendClientMessage(clientID, WS_OPCODE_PONG, data);
    }
    else if (opcode == WS_OPCODE_PONG){
        // received pong message (it's valid if the server did not send a ping request for this pong message)
        if (client->PingSentTime != 0) {
            client->PingSentTime = 0;
        }
    }
    else if (opcode == WS_OPCODE_CLOSE){
        // received close message
        if (client->ReadyState == WS_READY_STATE_CLOSING){
            // the server already sent a close frame to the client, this is the client's close frame reply
            // (no need to send another close frame to the client)
            client->ReadyState = WS_READY_STATE_CLOSED;
        }
        else {
            // the server has not already sent a close frame to the client, send one now
            wsSendClientClose(clientID, WS_STATUS_NORMAL_CLOSE);
        }

        wsRemoveClient(clientID);
    }
    else if (opcode == WS_OPCODE_TEXT || opcode == WS_OPCODE_BINARY){
        if (callOnMessage != NULL)
            callOnMessage(clientID, data.substr(0, dataLength));
    }
    else {
        // unknown opcode
        return false;
    }

    return true;
}

bool webSocket::wsProcessClientFrame(int clientID){
    wsClient *client = wsClients[clientID];
    // store the time that data was last received from the client
    client->LastRecvTime = clock();

    // check at least 6 bytes are set (first 2 bytes and 4 bytes for the mask key)
    if (client->FrameBuffer.size() < 6)
        return false;

    // fetch first 2 bytes of header
    unsigned char octet0 = client->FrameBuffer.at(0);
    unsigned char octet1 = client->FrameBuffer.at(1);

    unsigned char fin = octet0 & WS_FIN;
    unsigned char opcode = octet0 & 0x0f;

    //unsigned char mask = octet1 & WS_MASK;
    if (octet1 < 128)
        return false; // close socket, as no mask bit was sent from the client

    // fetch byte position where the mask key starts
    int seek = client->FrameBytesRead <= 125 ? 2 : (client->FrameBytesRead <= 65535 ? 4 : 10);

    // read mask key
    char maskKey[4];
    memcpy(maskKey, client->FrameBuffer.substr(seek, 4).c_str(), 4);

    seek += 4;

    // decode payload data
    string data;
    for (int i = seek; i < client->FrameBuffer.size(); i++){
        //data.append((char)(((int)client->FrameBuffer.at(i)) ^ maskKey[(i - seek) % 4]));
        char c = client->FrameBuffer.at(i);
        c = c ^ maskKey[(i - seek) % 4];
        data += c;
    }

    // check if this is not a continuation frame and if there is already data in the message buffer
    if (opcode != WS_OPCODE_CONTINUATION && client->MessageBufferLength > 0){
        // clear the message buffer
        client->MessageBufferLength = 0;
        client->MessageBuffer.clear();
    }

    // check if the frame is marked as the final frame in the message
    if (fin == WS_FIN){
        // check if this is the first frame in the message
        if (opcode != WS_OPCODE_CONTINUATION){
            // process the message
            return wsProcessClientMessage(clientID, opcode, data, client->FramePayloadDataLength);
        }
        else {
            // increase message payload data length
            client->MessageBufferLength += client->FramePayloadDataLength;

            // push frame payload data onto message buffer
            client->MessageBuffer.append(data);

            // process the message
            bool result = wsProcessClientMessage(clientID, client->MessageOpcode, client->MessageBuffer, client->MessageBufferLength);

            // check if the client wasn't removed, then reset message buffer and message opcode
            if (wsClients[clientID] != NULL){
                client->MessageBuffer.clear();
                client->MessageOpcode = 0;
                client->MessageBufferLength = 0;
            }

            return result;
        }
    }
    else {
        // check if the frame is a control frame, control frames cannot be fragmented
        if (opcode & 8)
            return false;

        // increase message payload data length
        client->MessageBufferLength += client->FramePayloadDataLength;

        // push frame payload data onto message buffer
        client->MessageBuffer.append(data);

        // if this is the first frame in the message, store the opcode
        if (opcode != WS_OPCODE_CONTINUATION) {
            client->MessageOpcode = opcode;
        }
    }

    return true;
}

bool webSocket::wsBuildClientFrame(int clientID, char *buffer, int bufferLength){
    wsClient *client = wsClients[clientID];
    // increase number of bytes read for the frame, and join buffer onto end of the frame buffer
    client->FrameBytesRead += bufferLength;
    client->FrameBuffer.append(buffer, bufferLength);

    // check if the length of the frame's payload data has been fetched, if not then attempt to fetch it from the frame buffer
    if (wsCheckSizeClientFrame(clientID) == true){
        // work out the header length of the frame
        int headerLength = (client->FramePayloadDataLength <= 125 ? 0 : (client->FramePayloadDataLength <= 65535 ? 2 : 8)) + 6;

        // check if all bytes have been received for the frame
        int frameLength = client->FramePayloadDataLength + headerLength;
        if (client->FrameBytesRead >= frameLength){
            char *nextFrameBytes;
            // check if too many bytes have been read for the frame (they are part of the next frame)
            int nextFrameBytesLength = client->FrameBytesRead - frameLength;
            if (nextFrameBytesLength > 0) {
                client->FrameBytesRead -= nextFrameBytesLength;
                nextFrameBytes = buffer + frameLength;
                client->FrameBuffer = client->FrameBuffer.substr(0, frameLength);
            }

            // process the frame
            bool result = wsProcessClientFrame(clientID);

            // check if the client wasn't removed, then reset frame data
            if (wsClients[clientID] != NULL){
                client->FramePayloadDataLength = -1;
                client->FrameBytesRead = 0;
                client->FrameBuffer.clear();
            }

            // if there's no extra bytes for the next frame, or processing the frame failed, return the result of processing the frame
            if (nextFrameBytesLength <= 0 || !result)
                return result;

            // build the next frame with the extra bytes
            return wsBuildClientFrame(clientID, nextFrameBytes, nextFrameBytesLength);
        }
    }

    return true;
}

bool webSocket::wsProcessClientHandshake(int clientID, char *buffer){
    // fetch headers and request line
    string buf(buffer);
    size_t sep = buf.find("\r\n\r\n");
    if (sep == string::npos)
        return false;

    string headers = buf.substr(0, sep);

    string request = headers.substr(0, headers.find("\r\n"));
    if (request.size() == 0)
        return false;

    string part;
    part = request.substr(0, request.find(" "));
    if (part.compare("GET") != 0 && part.compare("get") != 0 && part.compare("Get") != 0)
        return false;

    part = request.substr(request.rfind("/") + 1);
    if (atof(part.c_str()) < 1.1)
        return false;

    string host;
    string ws_key;
    string ws_version;
    headers = headers.substr(headers.find("\r\n") + 2);
    while (headers.size() > 0){
        request = headers.substr(0, headers.find("\r\n"));
        if (request.find(":") != string::npos){
            string key = request.substr(0, request.find(":"));
            if (key.find_first_not_of(" ") != string::npos)
                key = key.substr(key.find_first_not_of(" "));
            if (key.find_last_not_of(" ") != string::npos)
                key = key.substr(0, key.find_last_not_of(" ") + 1);

            string value = request.substr(request.find(":") + 1);
            if (value.find_first_not_of(" ") != string::npos)
                value = value.substr(value.find_first_not_of(" "));
            if (value.find_last_not_of(" ") != string::npos)
                value = value.substr(0, value.find_last_not_of(" ") + 1);

            if (key.compare("Host") == 0){
                host = value;
            }
            else if (key.compare("Sec-WebSocket-Key") == 0){
                ws_key = value;
            }
            else if (key.compare("Sec-WebSocket-Version") == 0){
                ws_version = value;
            }
        }
        if (headers.find("\r\n") == string::npos)
            break;
        headers = headers.substr(headers.find("\r\n") + 2);
    }

    if (host.size() == 0)
        return false;

    if (ws_key.size() == 0 || base64_decode(ws_key).size() != 16)
        return false;

    if (ws_version.size() == 0 || atoi(ws_version.c_str()) < 7)
        return false;

    unsigned char hash[20];
    ws_key.append("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    SHA1((unsigned char *)ws_key.c_str(), ws_key.size(), hash);
    string encoded_hash = base64_encode(hash, 20);

    string message = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ";
    message.append(encoded_hash);
    message.append("\r\n\r\n");

    int socket = wsClients[clientID]->socket;

    int left = message.size();
    do {
        int sent = send(socket, message.c_str(), message.size(), 0);
        if (sent == false) return false;

        left -= sent;
        if (sent > 0)
            message = message.substr(sent);
    }
    while (left > 0);

    return true;
}

bool webSocket::wsProcessClient(int clientID, char *buffer, int bufferLength){
    bool result;

    if (clientID >= wsClients.size() || wsClients[clientID] == NULL)
        return false;

    if (wsClients[clientID]->ReadyState == WS_READY_STATE_OPEN){
        // handshake completed
        result = wsBuildClientFrame(clientID, buffer, bufferLength);
    }
    else if (wsClients[clientID]->ReadyState == WS_READY_STATE_CONNECTING){
        // handshake not completed
        result = wsProcessClientHandshake(clientID, buffer);
        if (result){
            if (callOnOpen != NULL)
                callOnOpen(clientID);

            wsClients[clientID]->ReadyState = WS_READY_STATE_OPEN;
        }
    }
    else {
        // ready state is set to closed
        result = false;
    }

    return result;
}

int webSocket::wsGetNextClientID(){
    int i;
    for (i = 0; i < wsClients.size(); i++){
        if (wsClients[i] == NULL)
            break;
    }
    return i;
}

void webSocket::wsAddClient(int socket, in_addr ip){
    FD_SET(socket, &fds);
    if (socket > fdmax)
        fdmax = socket;

    int clientID = wsGetNextClientID();
    wsClient *newClient = new wsClient(socket, ip);
    if (clientID >= wsClients.size()){
        wsClients.push_back(newClient);
    }
    else {
        wsClients[clientID] = newClient;
    }
    socketIDmap[socket] = clientID;
}

void webSocket::setOpenHandler(defaultCallback callback){
    callOnOpen = callback;
}

void webSocket::setCloseHandler(messageCallback callback){
    callOnClose = callback;
}

void webSocket::setMessageHandler(messageCallback callback){
    callOnMessage = callback;
}

void webSocket::setPeriodicHandler(nullCallback callback){
    callPeriodic = callback;
}

void webSocket::startServer(int port){
    showAvailableIP();

    int yes = 1;
    char buf[4096];
    struct sockaddr_in serv_addr, cli_addr;

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(&cli_addr, '0', sizeof(cli_addr));
    serv_addr.sin_family = AF_INET;
    //serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

#if _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(int)) == -1){
        perror("setsockopt() error!");
        exit(1);
    }
    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        perror("bind() error!");
        exit(1);
    }
    if (listen(listenfd, 10) == -1){
        perror("listen() error!");
        exit(1);
    }

    fdmax = listenfd;
    fd_set read_fds;
    FD_ZERO(&fds);
    FD_SET(listenfd, &fds);
    FD_ZERO(&read_fds);

    struct timeval timeout;
    clock_t nextPingTime = clock() + 34;
    while (FD_ISSET(listenfd, &fds)){
        read_fds = fds;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;
        if (select(fdmax+1, &read_fds, NULL, NULL, &timeout) > 0){
            for (int i = 0; i <= fdmax; i++){
                if (FD_ISSET(i, &read_fds)){
                    if (i == listenfd){
                        socklen_t addrlen = sizeof(cli_addr);
                        int newfd = accept(listenfd, (struct sockaddr*)&cli_addr, &addrlen);
                        if (newfd != -1){
                            /* add new client */
                            wsAddClient(newfd, cli_addr.sin_addr);
                            printf("New connection from %s on socket %d\n", inet_ntoa(cli_addr.sin_addr), newfd);
                        }
                    }
                    else {
                        int nbytes = recv(i, buf, sizeof(buf), 0);
                        if (socketIDmap.find(i) != socketIDmap.end()){
                            if (nbytes < 0)
                                wsSendClientClose(socketIDmap[i], WS_STATUS_PROTOCOL_ERROR);
                            else if (nbytes == 0)
                                wsRemoveClient(socketIDmap[i]);
                            else {
                                if (!wsProcessClient(socketIDmap[i], buf, nbytes))
                                    wsSendClientClose(socketIDmap[i], WS_STATUS_PROTOCOL_ERROR);
                            }
                        }
                    }
                }
            }
        }

        if (clock() >= nextPingTime){
            wsCheckIdleClients();
            nextPingTime = clock() + 34;
        }

        if (callPeriodic != NULL)
            callPeriodic();
    }
}

void webSocket::stopServer(){
    for (int i = 0; i < wsClients.size(); i++){
        if (wsClients[i] != NULL){
            if (wsClients[i]->ReadyState != WS_READY_STATE_CONNECTING)
                wsSendClientClose(i, WS_STATUS_GONE_AWAY);
#ifdef __linux__
                close(wsClients[i]->socket);
#elif _WIN32
                closesocket(wsClients[i]->socket);
#endif
        }
    }
#ifdef __linux__
    close(listenfd);
#elif _WIN32
    closesocket(listenfd);
#endif

    wsClients.clear();
    socketIDmap.clear();
    FD_ZERO(&fds);
    fdmax = 0;
}
