/**
Team Members: Devon Gunn; ID# 28502436
**/
#include "player.h"

player::player()
{

}

player::player(int playerNumber)
{
	//position our paddle based on if they are player 1 or 2
	if (playerNumber == 1) paddle.xPos = 30;
	else paddle.xPos = 750;

	paddle.yPos = 250;
	paddle.width = 20;
	paddle.height = 100;

	score = 0;
	hits = 0;

	ready = false;
}

const int player::getXPos()
{
	return paddle.xPos;
}

const int player::getYPos()
{
	return paddle.yPos;
}

const int player::getWidth()
{
	return paddle.width;
}

const int player::getHeight()
{
	return paddle.height;
}

const int player::getScore()
{
	return score;
}

const int player::getHits()
{
	return hits;
}

const void player::incrementScore()
{
	score += 1;
}

const void player::incrementHits()
{
	hits += 1;
}

const void player::resetScore()
{
	score = 0;
}

const void player::resetHits()
{
	hits = 0;
}

const void player::resetPos()
{
	paddle.yPos = 250;
}

const void player::moveUp()
{
	//move paddle up
	paddle.yPos -= 10;

	//prevent paddle from moving off screen up
	if (paddle.yPos <= 0) paddle.yPos = 0;
}

const void player::moveDown()
{
	//move paddle down
	paddle.yPos += 10;

	//prevent paddle from moving off screen down
	if (paddle.yPos + paddle.height >= 600) paddle.yPos = 600 - paddle.height;
}

const void player::toggleReady()
{
	ready = !ready;
}

const bool player::isReady()
{
	return ready;
}

player::~player()
{
}
