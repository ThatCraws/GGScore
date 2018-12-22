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
			winId		the internal ID of the winner
			loseId		the internal ID of the loser
		*/
		Result(unsigned int pWinId, unsigned int pLoseId);
		Result(); //default constructor not containing any players(real result) yet

		// getters and setters
		unsigned int getWinId() const;
		unsigned int getLoseId() const;

	private:
		unsigned int winId;
		unsigned int loseId;
	};
}

#endif
