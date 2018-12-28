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
			pP1Id		the internal ID of player 1
			pP2Id		the internal ID of player 2
			pWinnerId	the internal ID of the winning player (if not the ID of one of the participating players, the result will be counted as forfeit)
		*/
		Result(unsigned int pP1Id, unsigned int pP2Id, unsigned int pWinId);
		Result(); //default constructor not containing any players(real result) yet

		// getters and setters
		unsigned int getP1Id() const;
		unsigned int getP2Id() const;
		unsigned int getWinnerId() const;

	private:
		unsigned int p1Id;
		unsigned int p2Id;
		unsigned int winnerId;
	};
}

#endif
