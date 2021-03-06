#define DLL_EXPORT

#include "Glicko-2.h"
#include "glkWorld.h"
#include "glkPlayer.h"
//Result imported in header

Glicko2::World* myWorld;
unsigned int id;
double tau = 0.5;

void Glicko2::createWorld() {
	if (myWorld != nullptr) {
		delete myWorld;
	}
	myWorld = new World();
	id = 0;
}

void Glicko2::closeWorld() {
	delete myWorld;
	myWorld = nullptr;
}

unsigned int Glicko2::createPlayer(double pRating, double pRD, double pVolatility) {
	myWorld->addPlayer(new Player(id, pRating, pRD, pVolatility));
	return id++;
}

void Glicko2::glicko(std::vector<Result>& repSets) {

	std::vector<Player> newPlayers;

	for(Player* currPlayer : myWorld->getPlayers()) { //iterate through all players and...

		std::vector<Result> relevantRes;
		
		for (Result currRes : repSets) { //...store all results...
			if (currPlayer->getID() == currRes.getP1Id() || currPlayer->getID() == (currRes.getP2Id())) //...which include this player
			{
				relevantRes.push_back(currRes);
			}
		}

		newPlayers.push_back(algorithm(currPlayer, relevantRes)); // apply algorithm to the current player with the collected relevant results and store the updated scores
	}

	//to this point no players have actually been updated (because new rating would affect calculating the rating of the other players)
	Player* toUpdate;
	for (Player currPlayer : newPlayers) {

		toUpdate = myWorld->getPlayerByID(currPlayer.getID());

		if (toUpdate != nullptr) { // player with the same ID was found
			//maybe put else.. nullptr should not happen here

			// update scores of the player with the same ID
			toUpdate->setRating(currPlayer.getRating());
			toUpdate->setRD(currPlayer.getRD());
			toUpdate->setVolatility(currPlayer.getVolatility());
		}
	}
}

Glicko2::Player Glicko2::algorithm(Player* player, std::vector<Result>& repSets) {

	// ------------ Step 1 - Done. Tau is set by createWorld() ------------

	// ------------ Step 2 - Translate Glicko-1 values to Glicko-2 values ------------

	double ownMu = (player->getRating() - 1500) / 173.7178f;
	double ownPhi = player->getRD() / 173.7178f;
	double ownSigma = player->getVolatility();

	// for storing updated values (used from step 5)
	double newMu;
	double newPhi;
	double newSigma;

	// ------------ Step 3 - Calculate estimated variance of player rating based on games ------------

	// start calculating v
	double v = 0.0;
	
	Player* currOpp;
	double currOppMu;
	double currOppPhi;

	for (Result currRes : repSets) {
		if ((currRes.getP1Id()) != player->getID()) {
			currOpp = playerByID(currRes.getP1Id());
		}
		else {
			currOpp = playerByID(currRes.getP2Id());
		}
		// calculate to glicko-2 system
		currOppMu = (currOpp->getRating() - 1500) / 173.7178f;
		currOppPhi = currOpp->getRD() / 173.7178f;

		v += pow(g(currOppPhi), 2) * E(ownMu, currOppMu, currOppPhi) * (1 - E(ownMu, currOppMu, currOppPhi));

	}
	v = pow(v, -1);

	// ------------ Step 4 - Calculate delta, estimated improvement of mu(rating) by comparing "pre - period rating to the performance rating"???

	double delta = 0.0;
	float winToCurrOpp; // possible values: 0, 0.5, 1.

	for (Result currRes : repSets) {
		if ((currRes.getP1Id()) != player->getID()) {
			currOpp = playerByID(currRes.getP1Id());
		}
		else {
			currOpp = playerByID(currRes.getP2Id());
		}

		// calculate to glicko-2 system
		currOppMu = (currOpp->getRating() - 1500) / 173.7178f;
		currOppPhi = currOpp->getRD() / 173.7178f;

		// did the player whose rating is getting adjusted win? (1 for win, 0 for loss, 0.5 for tie)
		if (player->getID() == currRes.getWinnerId()) {
			winToCurrOpp = 1.0f;
		}
		else if (currOpp->getID() == currRes.getWinnerId()) {
			winToCurrOpp = 0.0f;
		}
		else {
			winToCurrOpp = 0.5f;
		}

		// delta calculation
		delta += g(currOppPhi) * ((double)winToCurrOpp - E(ownMu, currOppMu, currOppPhi));

	}

	delta *= v;

	// ------------ Step 5 - Calculate new value of sigma, the rating volatility ------------

	// a)

	double a = log(pow(ownSigma, 2));
	//funcF(x, ownPhi, v, delta, a);

	double epsilon = 0.000001;


	// b)

	double A = a;
	double B;

	if (pow(delta, 2) > pow(ownPhi, 2) + v) {
		B = log(pow(delta, 2) - pow(ownPhi, 2) - v);
	}
	else {
		int k = 1;
		while (funcF((a - k * tau), ownPhi, v, delta, a) < 0) {
			k++;
		}
		B = a - k * tau;
	}


	// c)

	double fA = funcF(A, ownPhi, v, delta, a);
	double fB= funcF(B, ownPhi, v, delta, a);

	// d)

	double C;
	double fC;

	while (abs(B - A) > epsilon) {
		C = A + (A - B) * fA / (fB - fA); // (a)
		fC = funcF(C, ownPhi, v, delta, a); // (a)

		if (fC*fB < 0) { // (b)
			A = B; // (b)
			fA = fB; // (b)
		}
		else {
			fA /= 2; // (b)
		}

		B = C; // (c)
		fB = fC; // (c)
	}
	
	newSigma = exp(A / 2);

	// ------------ Step 6 - Updating rating deviation to pre-rating period value (phi*) ------------

	double astPhi = sqrt(pow(ownPhi, 2) + pow(newSigma, 2));

	// ------------ Step 7 - Updating rating and rating deviation ------------

	double part1 = 1 / pow(astPhi, 2);
	double part2 = 1 / v;
	double part3 = (part1 + part2);

	newPhi = 1 / sqrt(part3);

	double sum = 0;

	for (Result currRes : repSets) {
		if (currRes.getP1Id() != player->getID()) {
			currOpp = playerByID(currRes.getP1Id());
		}
		else {
			currOpp = playerByID(currRes.getP2Id());
		}

		// calculate to glicko-2 system
		currOppMu = (currOpp->getRating() - 1500) / 173.7178f;
		currOppPhi = currOpp->getRD() / 173.7178f;

		// did the player whose rating is getting adjusted win? (1 for win, 0 for loss, 0.5 for tie)
		if (player->getID() == currRes.getWinnerId()) {
			winToCurrOpp = 1.0f;
		}
		else if (currOpp->getID() == currRes.getWinnerId()) {
			winToCurrOpp = 0.0f;
		}
		else {
			winToCurrOpp = 0.5f;
		}

		sum += g(currOppPhi) * ((double)winToCurrOpp - E(ownMu, currOppMu, currOppPhi));
	}

	newMu = ownMu + pow(newPhi, 2) * sum;

	// ------------ Step 8 - Translate back to Glicko-1 (and create player to return) ------------

	Player toRet = Player(player->getID(), 173.7178 * newMu + 1500, 173.7178 * newPhi, newSigma);

	return toRet;
}

double Glicko2::g(double ownPhi) {
	const double PI = 3.141592653589793;

	double ret = 1 / sqrt((1 + 3 * pow((double)ownPhi, 2) / pow(PI, 2)));
	return ret;
}

double Glicko2::E(double ownMu, double oppMu, double oppPhi) {
	double ret = 1 / (1 + exp((g(oppPhi)*-1) * (ownMu - oppMu)));
	return ret;
}

double Glicko2::funcF(double x, double ownPhi, double v, double delta, double a) {
	double fracUpper = exp(x) * (pow(delta, 2) - pow(ownPhi, 2) - v - exp(x));
	double fracLower = 2 * pow((pow(ownPhi, 2) + v + exp(x)), 2);
	double subtrahend = (x - a) / pow(tau, 2);

	return fracUpper / fracLower - subtrahend;
}

Glicko2::Player* Glicko2::playerByID(unsigned int id) {
	return myWorld->getPlayerByID(id);
}

void Glicko2::setTau(double newTau) {
	tau = newTau;
}
