/**
Team Members: Devon Gunn; ID# 28502436
Total Hours spent Developing:72+
**/

#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>
#include <time.h>
#include "websocket.h"
#include "player.h"
#include "ball.h"
#include "DelayedMessage.h"
#include <process.h> //including this last just to make sure there are no headerguard clashes
//process.h allows us to execute command lines from the program. We do this to sync system time

using namespace std;

webSocket server;

//creater our local time object
SYSTEMTIME myLocalTime;

//create our players and ball
player * player1 = new player(1);
player * player2 = new player(2);
//player player2;
ball * pongBall = new ball();
//vector of delayed messages for lag simulation
vector<DelayedMessage> delayedMessageQueue;
//8 bit unsigned integer countdown timer
UINT8 iCountdownTimer = 4;

//gameStarted switches to true when game begins
//startCountdown switches to true when the 3, 2, 1 countdown needs to happen
bool gameStarted = false, startCountdown = false; 

/*our custom callback intercept for open callback*/
void openInterceptHandler(int clientID)
{
	//create a temporary message
	DelayedMessage tempMessage = DelayedMessage(clientID);

	//pass it into our vector
	delayedMessageQueue.push_back(tempMessage);
}


/*our custom callback intercept for close callback*/
void closeInterceptHandler(int clientID, string text)
{
	//create a temporary message
	DelayedMessage tempMessage = DelayedMessage(clientID, text, "close");

	//pass it into our vector
	delayedMessageQueue.push_back(tempMessage);
}

/*our custom callback intercept for message callback*/
void messageInterceptHandler(int clientID, string text)
{
	//create a temporary message
	DelayedMessage tempMessage = DelayedMessage(clientID, text, "message");

	//pass it into our vector
	delayedMessageQueue.push_back(tempMessage);
}

/*custom handler to intercept and delay wsSend messages*/
void delayedWSSend(int clientID, string text)
{
	//update localtime object
	GetLocalTime(&myLocalTime);

	//convert milliseconds to int
	int tempLocalTimeStamp = static_cast<int>(myLocalTime.wMilliseconds);
	DelayedMessage tempMessage = DelayedMessage(clientID, text, tempLocalTimeStamp);

	//pass it into our vector
	delayedMessageQueue.push_back(tempMessage);
}

/* called when a client connects */
void openHandler(int clientID){

	//create a vector of ints to capture the id's of each client
	//currently connected
	vector<int> clientIDs = server.getClientIDs();
	//create vector of user names
	vector<string> clientUIDs = server.getClientUIDs();

	//create an output string stream
    ostringstream os;
	string myStr = (clientID > 1) ? "lurker" : "player";
    os << "A " << myStr << /*clientUIDs[clientID] <<*/ " has joined.";

	//iterate through the retrieved client ids
    for (unsigned int i = 0; i < clientIDs.size(); i++){
		//Send the connection message to each client that isn't the newly
		//connected client
        if (clientIDs[i] != clientID)
            delayedWSSend(clientIDs[i], os.str());
    }
	//clear string stream
	os.str("");
	//fill stream
	os << "Welcome to the chat room." << /*clientUIDs[clientID] <<*/ " You are a "
		<< ((clientID > 1) ? "lurker." : "player.");
	//Send a welcome message to the new client
    delayedWSSend(clientID, os.str());

	//if the joiner is the player, tell them how to start the game
	if (clientID < 2)
	{
		os.str("");
		os << "Enter the chat command '#start' to start the game!";
		delayedWSSend(clientID, os.str());
	}
}

/* called when a client disconnects */
void closeHandler(int clientID, string userName){

	//get the list of the currently connected clients from
	//the websocket server
	vector<int> clientIDs = server.getClientIDs();
	//create vector of user names
	vector<string> clientUIDs = server.getClientUIDs();

	//create an output string stream
	ostringstream os, myStream;
	//insert into the stream a message containing which client has left
	string myStr = (clientID > 1) ? "Lurker " : "Player ";
    os << myStr << userName << " has left.";

	//end the game and reset the game for the next player
	if (clientID < 2)
	{
		gameStarted = false;

		//delete the ball and player object
		delete pongBall;
		delete player1;
		delete player2;

		//recreate them to reset them for the next player
		pongBall = new ball();
		player1 = new player(1);
		player2 = new player(2);

		//reset countdown timer
		iCountdownTimer = 4;
	}

	//iterate through the list send chat messages
    for (unsigned int i = 0; i < clientIDs.size(); i++){
		//inform other clients of client 'x' 's disconnect
        delayedWSSend(clientIDs[i], os.str());

		//Inform the new player that they are now a player
		if (clientIDs[i] < 2)
		{
			myStream.str("");
			myStream << "You are currently Player " << clientIDs[i] + 1
			<< ".\nEnter the chat command '#start' to start the game!";
			delayedWSSend(clientIDs[i], myStream.str());
		}
		else //inform everyone who the players are
		{
			myStream.str("");
			myStream << "The current players are now: " << clientUIDs[0] << ", and "
				<< clientUIDs[1] << ".";
			delayedWSSend(clientIDs[i], myStream.str());
		}
    }

	//iterate through the list and reset game visuals for remaining clients
	for (unsigned int i = 0; i < clientIDs.size(); i++)
	{
		//clear stream buffer
		os.str("");
		//compile render data into stream and send to clients
		//player format: (yPos, score, hits)
		//ball format: (xPos, yPos)
		os << "#server#p1(250,0,0)#p2(250,0,0)#ball(400,300)";
		delayedWSSend(clientIDs[i], os.str());
	}
}

/* called when a client sends a message to the server */
void messageHandler(int clientID, string message){

	//get the list of the currently connected clients from
	//the websocket server
	vector<int> clientIDs = server.getClientIDs();
	//create vector of user names
	vector<string> clientUIDs = server.getClientUIDs();

	//create output string stream
    ostringstream os;

	//catch part of the string to see if its a server command
	auto senderData = message.substr(0, 6);
	//check!
	if (senderData == "#serve")
	{
		//parse the rest of the string
		senderData = message.substr(6);

		//move paddle up or down if it's our player
		if (senderData == "#paddleUp" && clientID < 2 && gameStarted)
		{
			if (clientID == 0) player1->moveUp();
			else player2->moveUp();
		}
		else if (senderData == "#paddleDown" && clientID < 2 && gameStarted)
		{
			if (clientID == 0) player1->moveDown();
			else player2->moveDown();
		}
		else if (message.substr(6, 7) == "#UserID")
		{
			server.setClientUID(clientID, message.substr(14));
		}
		else if (senderData == "#start" && !gameStarted)
		{
			//only start the game if there are two players present
			//need to make it so both players must type #start
			if (clientID == 0)
			{
				if (!player1->isReady()) player1->toggleReady();

				//if both players are ready, start the initial countdown
				if (player2->isReady())
				{
					startCountdown = true;
					
					//tell the clients the playernames
					os.str("");
					os << "#playerIDs#" << clientUIDs[0] << "#" << clientUIDs[1];

					//iteratively send message to each client
					for (unsigned int i = 0; i < clientIDs.size(); i++) delayedWSSend(clientIDs[i], os.str());
				}
				else
				{
					os << "Player 1 is ready. The game will start when player 2 types #start.";
					delayedWSSend(0, os.str());
					delayedWSSend(1, os.str());
				}
			}
			else if (clientID == 1)
			{
				if (!player2->isReady()) player2->toggleReady();

				//if both players are ready, start the initial countdown
				if (player1->isReady())
				{
					startCountdown = true;

					//tell the clients the playernames
					os.str("");
					os << "#playerIDs#" << clientUIDs[0] << "#" << clientUIDs[1];

					//iteratively send message to each client
					for (unsigned int i = 0; i < clientIDs.size(); i++) delayedWSSend(clientIDs[i], os.str());
				}
				else
				{
					os << "Player 2 is ready. The game will start when player 1 types #start.";
					delayedWSSend(0, os.str());
					delayedWSSend(1, os.str());
				}
			} //if the person typing #start isn't a player, inform them only players can start
			else
			{
				os << "Sorry, only players can start the game.";
				delayedWSSend(clientID, os.str());
			}
		}
	} // this is where we check to see if its a time packet
	//this calculates the ping between the client -> server
	else if (senderData == "#timer")
	{
		//update localtime object
		GetLocalTime(&myLocalTime);

		//parse the rest of the string
		senderData = message.substr(6);

		//create an int to hold converted sender data
		int convertedSenderData;

		//use an input string stream to convert to int
		istringstream(senderData) >> convertedSenderData;

		//calculate difference
		int difference = static_cast<int>(myLocalTime.wMilliseconds) - convertedSenderData;

		//calculate ping
		int ping = (difference < 0) ? (difference + 1000) : difference;

		//print ping
		std::cout << clientUIDs[clientID] << "'s incoming ping: " << ping << endl;
	}
	else //if it's not a server message, carry on as normal and forward chat
	{
		//insert compiled client chat message into stream
		string myStr = (clientID > 1) ? " (Lurker):" : " (Player):";
		os.str("");
		os << clientUIDs[clientID] << myStr << message;

		//iteratively send message to each other client
		for (unsigned int i = 0; i < clientIDs.size(); i++){
			if (clientIDs[i] != clientID)
				delayedWSSend(clientIDs[i], os.str());
		}
	}
}

/* called once per select() loop */
void periodicHandler(){

		//build our stringstream
		ostringstream os;

		//get milliseconds and set next benchmark
		static clock_t next = clock() + 17; //30 fps @ 34 ms; 60 fps @ 17 ms; we are experimentally using 120 fps @ 8ms
		clock_t current = clock();

		//This is where I'll iterate through the vector of delayed messages
		//Any message that is ready to be received will then be passed to 
		//it's appropriate handlers.
		//iterate through queue if it's not empty
		if (!delayedMessageQueue.empty())
		{
			//iterate through message queue
			for (unsigned int i = 0; i < delayedMessageQueue.size(); ++i)
			{
				//check to see if a message is ready to send
				if (current >= delayedMessageQueue[i].timeReceived)
				{
					//switch statement to determine where to send it
					if (delayedMessageQueue[i].messageType == "open")
					{
						openHandler(delayedMessageQueue[i].clientID);
					}
					else if (delayedMessageQueue[i].messageType == "close")
					{
						closeHandler(delayedMessageQueue[i].clientID, delayedMessageQueue[i].myMessage);
					}
					else if (delayedMessageQueue[i].messageType == "message")
					{
						messageHandler(delayedMessageQueue[i].clientID, delayedMessageQueue[i].myMessage);
					}
					else if (delayedMessageQueue[i].messageType == "outbound") //HANDLE OUTBOUND MESSAGES
					{
						//reset out string stream just in case
						os.str("");

						//create our output string
						os << "#timer" << delayedMessageQueue[i].localTimeStamp;

						//send the timestamp
						server.wsSend(delayedMessageQueue[i].clientID, os.str());

						//send the message
						server.wsSend(delayedMessageQueue[i].clientID, delayedMessageQueue[i].myMessage);

						//reset our stringstream for the next users
						os.str("");
					}

					//delete this one out of the stack
					delayedMessageQueue.erase(delayedMessageQueue.begin() + i);

					//subtract 1 from i so it hits the next item in queue
					i -= i;
				}
			}
		}

		//timed loop
		if (current >= next){
			//get this frame's clientIDs
			vector<int> clientIDs = server.getClientIDs();

			//if its time to start the countdown, do so and start the game
			if (startCountdown)
			{
				//set initial next for 4 sec countdown
				static clock_t countdownNext = clock() + 1000;
				static bool sendInitialCountdownText = true;

				if (sendInitialCountdownText)
				{
					//let players know round is abotu to start
					os << "Round starting in:";

					//cycles through the client IDs and sends them game start data
					for (unsigned int i = 0; i < clientIDs.size(); i++)
					{
						delayedWSSend(clientIDs[i], os.str());
					}

					sendInitialCountdownText = false;
				}

				
				//loop 4 times for 3, 2, 1, GO
				if (iCountdownTimer > 0)
				{
					//send update every 1000 ms
					if (current - countdownNext >= 0)
					{
						//reset next
						countdownNext = clock() + 1000;

						//build and send string stream for countdown
						if (iCountdownTimer > 1)
						{
							os.str("");
							os << iCountdownTimer - 1 << ". . .";

							//cycles through the client IDs and sends them game start data
							for (unsigned int j = 0; j < clientIDs.size(); ++j)
							{
								delayedWSSend(clientIDs[j], os.str());
							}
						}
						else
						{
							os.str("");
							os << "GO!";

							//cycles through the client IDs and sends them game start data
							for (unsigned int j = 0; j < clientIDs.size(); ++j)
							{
								delayedWSSend(clientIDs[j], os.str());
							}
						}

						//increment
						--iCountdownTimer;
					}
				}
				//reset everything for next countdown
				if (iCountdownTimer == 0)
				{
					gameStarted = true;
					startCountdown = false;
					sendInitialCountdownText = true;
				}
			}

			if (gameStarted)
			{
				//advance the game logic, if a player scores, restart countdown
				if (startCountdown = pongBall->update(*player1, *player2))
				{
					//prevent game from continuing until new countdown finishes
					gameStarted = false;

					//reset player paddle positions
					player1->resetPos();
					player2->resetPos();

					iCountdownTimer = 4;
				}
					
				//cycles through the client IDs and sends them game data
				for (unsigned int i = 0; i < clientIDs.size(); i++)
				{
					//delayedWSSend(clientIDs[i], os.str());
					//clear stream buffer
					os.str("");
					//compile render data into stream and send to clients
					os << "#server#p1(" << player1->getYPos() << ","
						<< player1->getScore() << "," << player1->getHits()
						<< ")#p2(" << player2->getYPos() << ","
						<< player2->getScore() << "," << player2->getHits()
						<< ")#ball(" << pongBall->getXPos() << ","
						<< pongBall->getYPos() << ")";
					delayedWSSend(clientIDs[i], os.str());
				}
			}

			next = clock() + 17; //dictates fps
		}
}

int main(int argc, char *argv[]){
    int port;
	//init random seed
	srand(static_cast<unsigned int>(time(NULL)));

	//synchronize the clock!
	cout << "Synching system clock. Please Wait..." << endl;

	system("w32tm /resync"); //need to manually sync system time once, and run .exe as admin for this to work

	//set the port which our server listens on
    cout << "Please set server port: ";
    cin >> port;

    /* set event handler */
	server.setOpenHandler(openInterceptHandler);
	server.setCloseHandler(closeInterceptHandler);
	server.setMessageHandler(messageInterceptHandler);
    server.setPeriodicHandler(periodicHandler);

    /* start the chatroom server, listen to ip '127.0.0.1' and port '8000' */
    server.startServer(port);

    return 1;
}
