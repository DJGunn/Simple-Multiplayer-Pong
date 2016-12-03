/**
Team Members: Devon Gunn; ID# 28502436
**/
#ifndef BALL_H
#define BALL_H

#include <stdlib.h>
#include <time.h>
#include "myCirc.h"
#include "player.h"

class ball
{
public:
	ball();

	const int getXPos();

	const int getYPos();

	const int getRadius();

	bool update(player& player1, player& player2);

	~ball();
private:
	myCirc myBall;
	float xVelocity, yVelocity;
	int mP1YPosLast, mP2YPosLast;
	bool mP1Moved, mP2Moved;
};

#endif