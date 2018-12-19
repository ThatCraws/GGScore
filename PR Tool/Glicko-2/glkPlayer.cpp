#define DLL_EXPORT
#include "glkPlayer.h"

namespace Glicko2 {

	Player::Player(const unsigned int pID, double pRating, double pRD, double pVolatility)
		: id(pID), rating(pRating), rd(pRD), volatility(pVolatility)
	{
	};

	//getters and setter
	const unsigned int Player::getID() const {
		return id;
	}

	double Player::getRating() const {
		return rating;
	}

	void Player::setRating(double newRating) {
		rating = newRating;
	}

	double Player::getRD() const {
		return rd;
	}

	void Player::setRD(double newRD) {
		rd = newRD;
	}

	double Player::getVolatility() const {
		return volatility;
	}

	void Player::setVolatility(double newVolatility) {
		volatility = newVolatility;
	}
}