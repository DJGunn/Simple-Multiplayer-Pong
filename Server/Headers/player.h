/**
Team Members: Devon Gunn; ID# 28502436
**/
#ifndef PLAYER_H
#define PLAYER_H

#include "myRect.h"

class player
{
public:
	player();

	player(int playerNumber);

	const int getXPos();

	const int getYPos();

	const int getWidth();

	const int getHeight();

	const int getScore();

	const int getHits();

	const void incrementScore();

	const void incrementHits();

	const void resetScore();

	const void resetHits();

	const void resetPos();
	
	const void moveUp();

	const void moveDown();

	const void toggleReady();

	const bool isReady();

	~player();
private:
	myRect paddle;
	int score, hits, clientID;
	bool ready;
};

#endif