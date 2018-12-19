#ifndef __PLAYER_H
#define __PLAYER_H

#if defined DLL_EXPORT
#define GLICKO_API __declspec(dllexport)
#else
#define GLICKO_API __declspec(dllimport)
#endif

namespace Glicko2 {
	class GLICKO_API  Player {
	public:
		/* ------------ Player ------------

		Represents a player to be ranked by the glicko-algorithm.
		Holds all neccessary information to calculate the ranking except for results, which have to be provided externally.
		Names/Tags/Aliases have to be managed outside the API (deriving from this class is recommended)

		Parameters:
			pRating		the initial rating of the player, it will be adjusted by the algorithm
			pRD			the initial rating deviation of the player, it will be adjusted by the algorithm
			pVolatility	the initial rating volatility of the player, it will be adjusted by the algorithm
		*/
		//GLICKO_API 
		Player(const unsigned int pID, double pRating = 1500.0f, double pRD = 350.0f, double pVolatility = 0.06); // Constructor providing option to provide start-values is exported

		// getters and setters
		const unsigned int getID() const; //GLICKO_API 

		double getRating() const; //GLICKO_API
		void setRating(double newRating);

		double getRD() const; //GLICKO_API 
		void setRD(double newRD);

		double getVolatility() const; //GLICKO_API 
		void setVolatility(double newVolatility);
	private:
		const unsigned int id;
		double rating;
		double rd;
		double volatility;
	};
}

#endif