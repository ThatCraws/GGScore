#ifndef __WORLD_H
#define __WORLD_H

#include <vector>

namespace Glicko2 {

	// forward declaration
	class Player;

	/* ------------ World ------------

	Manages and holds the main requirements to apply the Glicko-algorithm.
	The constructor gets called by the Glicko-2 createWorld()-method.
	*/
	class World
	{
	public:
		/*------------ constructor ------------

		Creates an empty world waiting to be filled with players by Glicko-2's createPlayer()-method.

		Parameters
			pTau	the value of tau which decides how much rating volatility is affected by a single result (upset influence)
		*/
		World(double pTau = 0.5f);

		/* ------------ destructor ------------

		Due to the Player-objects being created in a method of Glicko-2 they have to be dynamically allocated and therefore manually deleted.
		*/
		~World();

		/* ------------ addPlayer ------------

		adds a player to player base of this world

		parameters:
			toAdd	pointer to the player to add
		return
			true,	if player was added successfully
			false,	if player is already in list
		*/
		bool addPlayer(Player* toAdd);

		/* ------------ removePlayer ------------

		removes a player from player base of this world

		parameters:
			toRem	pointer to the player to remove
		return
			true,	if player was removed successfully
			false,	if player was not found in the list
		*/
		bool removePlayer(Player* toRem);

		/* ------------ getPlayerByID ------------

		Finds a player based on ID and returns a pointer to it

		parameters:
			id	ID of the player to find
		return
			a pointer to the player with the specified ID
			a nullptr if no player with the specified ID was found
		*/
		Player* getPlayerByID(unsigned int id) const;

		//getters and setters
		const std::vector<Player*>& getPlayers() const;

	private:
		std::vector<Player*> players;
	};
}

#endif