#ifndef __RESULT_H
#define __RESULT_H

#if defined DLL_EXPORT
#define GLICKO_API __declspec(dllexport)
#else 
#define GLICKO_API __declspec(dllimport)
#endif

#include "glkPlayer.h"

namespace Glicko2 {
	class GLICKO_API Result {
	public:
		/* ------------ Result ------------
		Objects of the Result class represent reported outcomes of sets/matches. Only the overall outcome of a set is considered in Glicko, not single games.
		parameters:
			pP1		"player 1" of the set
			pP2		"player 2" of the set
			pWinner	the winner of the set
		*/
		Result(Player pWinner, Player pLoser);

		// getters and setters
		Player getWinner() const;
		Player getLoser() const;

	private:
		Player winner;
		Player loser;
	};
}

#endif
