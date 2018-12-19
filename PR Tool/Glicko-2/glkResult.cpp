#define DLL_EXPORT

#include "glkResult.h"

namespace Glicko2 {
	Result::Result(unsigned int pWinId, unsigned int pLoseId)
		:winId(pWinId), loseId(pLoseId)
	{

	}

	unsigned int Result::getWinId() const {
		return winId;
	}

	unsigned int Result::getLoseId() const {
		return loseId;
	}
}