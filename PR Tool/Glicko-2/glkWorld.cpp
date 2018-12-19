#define DLL_EXPORT // to be able to use the other exported functions. (Player::getID to be precise)

#include "glkWorld.h"

#include "glkPlayer.h"

namespace Glicko2 {

	World::World(double pTau) {
	}

	World::~World() {
		for (Player* currPlayer : players) {
			delete currPlayer;
		}
	}

	bool World::addPlayer(Player* toAdd) {
		for (Player* currPlayer : players)
		{
			if (currPlayer == toAdd) {
				return false;
			}
		}
		players.push_back(toAdd);
		return true;
	}

	bool World::removePlayer(Player* toRem) {
		for (std::vector<Player*>::iterator iter = players.begin(); iter != players.end(); iter++)
		{
			if (*iter == toRem) {
				players.erase(iter);
				delete toRem;
				return true;
			}
		}
		return false;
	}

	Player* World::getPlayerByID(unsigned int id) const {
		for (Player* currPlayer : players)
		{
			if (currPlayer->getID() == id) {
				return currPlayer;
			}
		}
		return nullptr;
	}

	const std::vector<Player*>& World::getPlayers() const {
		return players;
	}

}