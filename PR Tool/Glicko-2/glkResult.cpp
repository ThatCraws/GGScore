#define DLL_EXPORT

#include "glkResult.h"

namespace Glicko2 {
	Result::Result()
		:p1Id(-1), p2Id(-1), winnerId(0) {}

	Result::Result(unsigned int pP1Id, unsigned int pP2Id, unsigned int pWinId)
		:p1Id(pP1Id), p2Id(pP2Id), winnerId(pWinId) {}

	unsigned int Result::getP1Id() const {
		return p1Id;
	}

	unsigned int Result::getP2Id() const {
		return p2Id;
	}

	unsigned int Result::getWinnerId() const {
		return winnerId;
	}
}