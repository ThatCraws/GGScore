#ifndef __GLICKO_H
#define __GLICKO_H

#if defined DLL_EXPORT
#define GLICKO_API __declspec(dllexport)
#else
#define GLICKO_API __declspec(dllimport)
#endif

#include<vector>

#include "glkResult.h"

namespace Glicko2 {

	// forward declarations
	class World;
	class Player;

	/*------------ createWorld ------------

	Creates an empty world waiting to be filled with players.
	Has to be called before any other method for the API to work.
	Is dynamically allocated, so closeWorld() has to be called at the end.
	Can also be used to reset world.
	*/
	GLICKO_API void createWorld();

	/*------------ closeWorld ------------

	Shuts down current world, has to be called before exiting the application using this API, when createWorld() was called.
	*/
	GLICKO_API void closeWorld();

	/*------------ glicko ------------

	The main method of the API. Will update the previously created players' scores, using the given results and the Glicko-2 algorithm.

	Parameter:
		repSets		a vector of Results to be used to calculate new players' scores
	*/
	GLICKO_API void glicko(std::vector<Result>& repSets);

	/*------------ algorithm ------------

	The main algorithm. Will update a previously created player's scores, using the given results (which should all include the player).

	Parameter:
		repSets		a vector of Results to be used to calculate the new player's rating values
	Return:
		returns a player-object with the same ID as the given one and updated rating values
	*/
	Player algorithm(Player* player, std::vector<Result>& repSets);
	// help methods
	double g(double ownPhi);
	double E(double ownMu, double oppMu, double oppPhi);
	double funcF(double x, double ownPhi, double delta, double v, double a);

	/*
	------------ createPlayer ------------

	Adds a player to the world, custom start values can be assigned, otherwise the suggested start values of Prof. Glickman are used.
	Parameter:
		pRating		the start value of the skill rating of the player
		pRD			the start value of the rating deviation of the player
		pVolatility	the start value of the rating volatility of the player

	Return:
		an integer representing the ID of the player in the world. Used by getPlayerByID()-method

	See also: Player::Player(), getPlayerByID()
	*/
	GLICKO_API unsigned int createPlayer(double pRating = 1500.0f, double pRD = 350, double pVolatility = 0.06);

	/* ------------ playerByID ------------

	Returns a pointer to the player with the specified ID

	Parameter:
		id		the ID of the player to return

	Return:
		a pointer to the player with the specified ID
		nullptr, if no player with the specified ID could be found

	See also: World::getPlayerByID()
	*/
	GLICKO_API Player* playerByID(unsigned int id);

	GLICKO_API void setTau(double newTau);
}

#endif