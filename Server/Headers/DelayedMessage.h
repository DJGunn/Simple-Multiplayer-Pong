/**
Team Members: Devon Gunn; ID# 28502436
**/
#ifndef DELAYEDMESSAGE_H
#define DELAYEDMESSAGE_H

#include <string>
#include <time.h>

//creating the myCirc data structure for use in our classes
struct DelayedMessage
{
	//for 'open' inbound messages
	DelayedMessage(int clientID)
	{
		//save client id and define what type of delayed message this is
		this->clientID = clientID;
		messageType = "open";

		//save time message was 'sent'
		timeWasSent = clock();

		//save time message was 'receieved' and when it should be processed
		timeReceived = timeWasSent + (rand() % 25 + 1);

	}

	//for 'close' and 'message' inbound messages
	DelayedMessage(int clientID, std::string message, std::string type)
	{
		//save client id and define what type of delayed message this is
		this->clientID = clientID;
		this->myMessage = message; //save the actual message
		messageType = type;

		//save time message was 'sent'
		timeWasSent = clock();

		//save time message was 'receieved' and when it should be processed
		timeReceived = timeWasSent + (rand() % 25 + 1);
	}

	//for 'wsSend' outbound messages
	DelayedMessage(int clientID, std::string message, int timeStamp)
	{
		//save client id and define what type of delayed message this is
		this->clientID = clientID;
		messageType = "outbound";

		//save time message was 'sent'
		timeWasSent = clock();

		//save time message was 'receieved' and when it should be processed
		timeReceived = timeWasSent + (rand() % 25 + 1);

		//save local time stamp
		localTimeStamp = timeStamp;

		//save the message
		myMessage = message;
	}

	std::string myMessage, messageType;
	int clientID, localTimeStamp;
	clock_t timeWasSent, timeReceived;
};

#endif