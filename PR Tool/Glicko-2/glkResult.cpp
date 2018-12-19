#define DLL_EXPORT

#include "glkResult.h"

namespace Glicko2 {
	Result::Result(Player pWinner, Player pLoser)
		:winner(pWinner), loser(pLoser)
	{

	}

	Player Result::getWinner() const {
		return winner;
	}

	Player Result::getLoser() const {
		return loser;
	}
}