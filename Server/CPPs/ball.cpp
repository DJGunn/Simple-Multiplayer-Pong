/**
Team Members: Devon Gunn; ID# 28502436
**/
#include "ball.h"

ball::ball()
{
	//center the ball on our 800 x 400 canvas
	myBall.xPos = 400;
	myBall.yPos = 300;
	myBall.radius = 8;

	//initialize P1 and P2 last Y-positions at 250, the default
	mP1YPosLast = mP2YPosLast = 250;
	//init movement flags for the players
	mP1Moved = mP2Moved = false;

	//random x/y velocity between 5-10 pixels/frame
	xVelocity = ((rand() % 10 + 0.5) <= 5 ? -1 : 1) * (rand() % 3 + 1);
	yVelocity = ((rand() % 10 + 0.5) <= 5 ? -1 : 1) * (rand() % 3 + 0.5);
}

const int ball::getXPos()
{
	return myBall.xPos;
}

const int ball::getYPos()
{
	return myBall.yPos;
}

const int ball::getRadius()
{
	return myBall.radius;
}

bool ball::update(player& player1, player& player2)
{
	//test to see if the paddle has moved. If it has, set a flag and reset last position
	if (player1.getYPos() != mP1YPosLast)
	{
		mP1Moved = true;
		mP1YPosLast = player1.getYPos();
	}
	if (player2.getYPos() != mP2YPosLast)
	{
		mP2Moved = true;
		mP2YPosLast = player2.getYPos();
	}

	//flag to avoid double hit increment
	bool incremented = false;

	//update position
	myBall.xPos += xVelocity;
	myBall.yPos += yVelocity;

	//if the ball bounces off the ceiling or floor
	if (myBall.yPos - myBall.radius <= 0)
	{
		yVelocity *= -1; //reverse y vel
		myBall.yPos = 8; //place ball back in bounds
	}
	else if (myBall.yPos + myBall.radius >= 600)
	{
		yVelocity *= -1; //reverse y vel
		myBall.yPos = 592; //place ball back in bounds
	}
	
	//alter velocity if ball collides with a paddle
	//set it up so that if it's one velocity or less from the top of bottom
	//only reverse yVelocity, otherwise reverse xVelocity

	//some temp absolute values for velocity
	int absYVel, absXVel;
	absYVel = abs(static_cast<int>(yVelocity));
	absXVel = abs(static_cast<int>(xVelocity));

	//handle top edge of the paddle for player 1
	if ((absYVel >= (myBall.yPos + myBall.radius) - (player1.getYPos() + (mP1Moved ? -10 : 0))//this is where we insert
		&& (myBall.yPos + myBall.radius) - (player1.getYPos() + (mP1Moved ? -10 : 0)) >= 0
		&& (player1.getXPos() + player1.getWidth())
		>= myBall.xPos - myBall.radius
		&& myBall.xPos + myBall.radius >= player1.getXPos()))
	{
		//change direction and increment counters
		yVelocity *= -1;
		player1.incrementHits();
		incremented = true;

		//set the ball's y position so it won't render inside paddle
		myBall.yPos = player1.getYPos() - myBall.radius;

	} //handle bottom edge of the paddle for player 1
	else if ((absYVel >= (player1.getYPos()
		+ player1.getHeight() + (mP1Moved ? 10 : 0)) - (myBall.yPos - myBall.radius)
		&& (player1.getYPos()
		+ player1.getHeight() + (mP1Moved ? 10 : 0)) - (myBall.yPos - myBall.radius) >= 0
		&& (player1.getXPos() + player1.getWidth())
		>= myBall.xPos - myBall.radius
		&& myBall.xPos + myBall.radius >= player1.getXPos()))
	{
		//change direction and increment counters
		yVelocity *= -1;
		player1.incrementHits();
		incremented = true;

		//set the ball's y position so it won't render inside paddle
		myBall.yPos = player1.getYPos() + player1.getHeight() + myBall.radius;

	} //handle the top edge of the paddle for player 2
	else if ((absYVel >= (myBall.yPos + myBall.radius) - (player2.getYPos() + (mP2Moved ? -10 : 0))
		&& (myBall.yPos + myBall.radius) - (player2.getYPos() + (mP2Moved ? -10 : 0)) >= 0
		&& (player2.getXPos() + player2.getWidth())
		>= myBall.xPos - myBall.radius
		&& myBall.xPos + myBall.radius >= player2.getXPos()))
	{
		//change direction and increment counters
		yVelocity *= -1;
		player2.incrementHits();
		incremented = true;

		//set the ball's y position so it won't render inside paddle
		myBall.yPos = player2.getYPos() - myBall.radius;

	} //handle the bottom edge of the paddle for player 2
	else if ((absYVel >= (player2.getYPos()
		+ player2.getHeight() + (mP2Moved ? 10 : 0)) - (myBall.yPos - myBall.radius)
		&& (player2.getYPos()
		+ player2.getHeight() + (mP2Moved ? 10 : 0)) - (myBall.yPos - myBall.radius) >= 0
		&& (player2.getXPos() + player2.getWidth())
		>= myBall.xPos - myBall.radius
		&& myBall.xPos + myBall.radius >= player2.getXPos()))
	{
		//change direction and increment counters
		yVelocity *= -1;
		player2.incrementHits();
		incremented = true;

		//set the ball's y position so it won't render inside paddle
		myBall.yPos = player2.getYPos() + player2.getHeight() + myBall.radius;
	}

	//handle front edge of the paddle for player 1
	if ((absXVel >= (player1.getXPos() + player1.getWidth())
		- (myBall.xPos - myBall.radius)
		&& (player1.getXPos() + player1.getWidth())
		- (myBall.xPos - myBall.radius) >= 0)
		&& myBall.yPos + myBall.radius >= player1.getYPos() + (mP1Moved ? -10 : 0)
		&& myBall.yPos - myBall.radius <= player1.getYPos() + player1.getHeight() + (mP1Moved ? 10 : 0))
	{
		//add 1 to the xVelocity on paddle hit
		xVelocity += (xVelocity > 0) ? 1.0 : -1.0;

		//then reverse velocity
		xVelocity *= -1;

		if (!incremented) player1.incrementHits();

		//set ball's x position so it won't render inside the paddle
		myBall.xPos = player1.getXPos() + player1.getWidth() + myBall.radius;
	} //handle the front edge of the paddle for player 2
	else if ((absXVel >= (myBall.xPos + myBall.radius) - player2.getXPos()
		&& (myBall.xPos + myBall.radius) - player2.getXPos() >= 0)
		&& myBall.yPos + myBall.radius >= player2.getYPos() + (mP2Moved ? -10 : 0)
		&& myBall.yPos - myBall.radius <= player2.getYPos() + player2.getHeight() + (mP2Moved ? 10 : 0))
	{
		//add 1 to the xVelocity on paddle hit
		xVelocity += (xVelocity > 0) ? 1.0 : -1.0;

		//then reverse velocity
		xVelocity *= -1;

		if (!incremented) player2.incrementHits();

		//set ball's x position so it won't render inside the paddle
		myBall.xPos = player2.getXPos() - myBall.radius;
	}

	//reset movement flags
	mP1Moved = false;
	mP2Moved = false;

	//This handles player 2 scoring on player 1
	if (myBall.xPos - myBall.radius <= 0)
	{
		player1.resetHits();
		player2.incrementScore();

		//reset ball position
		//center the ball on our 800 x 400 canvas
		myBall.xPos = 400;
		myBall.yPos = 300;
		myBall.radius = 8;

		//random x/y velocity between 5-10 pixels/frame
		xVelocity = ((rand() % 10 + 0.5) <= 5 ? -1 : 1) * (rand() % 3 + 1);
		yVelocity = ((rand() % 10 + 0.5) <= 5 ? -1 : 1) * (rand() % 3 + 0.5);

		//round ends, restart countdown
		return true;
	}
	//this handles player 1 scoring on player 2
	else if (myBall.xPos + myBall.radius >= 800)
	{
		player2.resetHits();
		player1.incrementScore();

		//reset ball position
		//center the ball on our 800 x 400 canvas
		myBall.xPos = 400;
		myBall.yPos = 300;
		myBall.radius = 8;

		//random x/y velocity between 5-10 pixels/frame
		xVelocity = ((rand() % 10 + 0.5) <= 5 ? -1 : 1) * (rand() % 3 + 1);
		yVelocity = ((rand() % 10 + 0.5) <= 5 ? -1 : 1) * (rand() % 3 + 0.5);

		//round ends, restart countdown
		return true;
	}

	//round doesn't end, don't start countdown
	return false;
}

ball::~ball()
{
}
