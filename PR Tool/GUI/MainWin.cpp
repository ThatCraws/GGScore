#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/notebook.h>
#include <wx/sstream.h>

#include <fstream>

#include "json/json.h"
#include "jsoncpp.cpp"

#include "GetRequester.h"

#include "Glicko-2.h"

#include "MainWin.h"
#include "WinMatchRep.h"
#include "WinRatPer.h"
#include "WinSetAbt.h"
#include "WinPlayerEdit.h"
#include "AssignPlayerDialog.h"
#include "GlobalVars.h"

// Struct holding relevant information for a rating
struct MainWin::Rating {
	double rating;
	double deviation;
	double volatility;
};

// Struct holding all relevant information of a player
struct MainWin::Player {
	unsigned int id;
	std::vector<std::string> aliases;
	std::vector<Rating> ratings;
	unsigned int wins; // Wins in previous (pre-finalizing) periods
	unsigned int losses; // Losses in previous (pre-finalizing) periods
	unsigned int ties; // Ties in previous (pre-finalizing) periods
	bool visible;
};

// Struct holding all relevant information of a result. Ties are managed by the Glicko2-Result.
// The winnerID will neither be player 1 nor player 2, if it is a tie.
struct MainWin::Result {
	Glicko2::Result result;
	wxDateTime date;
	bool forfeit;
	std::string desc;
};

// Helper functions (has to be up here to be read before ratingPeriods-set is initialized)
bool comparePeriods(const std::pair<wxDateTime, wxDateTime>& periodOne, const std::pair<wxDateTime, wxDateTime>& periodTwo) {
	return (periodOne.first.IsEarlierThan(periodTwo.first));
}

// to sort results by date
bool compareResultDates(const MainWin::Result& resultOne, const MainWin::Result& resultTwo) {
	return resultOne.date.IsEarlierThan(resultTwo.date);
}

MainWin::MainWin()
	: wxFrame(NULL, wxID_ANY, wxString("GGScore"))
{
	SetIcon(wxIcon(wxString("A_MYICON")));

	// initialize set of results with compare-function for sorting by date
	results = std::multiset<Result, bool(*)(const Result&, const Result&)>(&compareResultDates);

	/*
		The GUI is made of sizers and derivations of wxWindow, the hierarchy is as follows:
			MainWin(this) -> mainSizer -> navi -> actual Windows to use the application(sorted in tabs)
	*/

	// The navi notebook provides tabs for navigation through the application
	wxNotebook* navi = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxSize(100, 100));
	periodWindow = new WinRatPer(navi);
	matchWindow = new WinMatchRep(navi);
	playerEditWindow = new WinPlayerEdit(navi);
	setAbtWindow = new WinSetAbt(navi);

	// The windows have to exist before loading the world, so let's also create it here
	Glicko2::createWorld();
	loadWorld();
	// world has to be loaded before results
	loadResults();
	// after windows were created
	loadSettings(); 
	Glicko2::setTau(setAbtWindow->getTau());

	// Now that all players and their aliases are loaded give mainAliases to playerEdit tab
	playerEditWindow->setMainAliases(retrieveMainAliases());

	navi->AddPage(periodWindow, "Rating Periods", true);
	navi->AddPage(matchWindow, "Report Matches");
	navi->AddPage(playerEditWindow, "Player Edit");
	navi->AddPage(setAbtWindow, "Settings/About");

	wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL); // holds the notebook
	mainSizer->SetMinSize(winMinWidth, winMinHeight);
	mainSizer->Add(navi, 1, wxEXPAND);
	this->SetSizer(mainSizer);

	// insert loaded world into GUI
	for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
		periodWindow->addRatingPeriod(currPeriod->first , currPeriod->second);
	}

	// insert loaded results into GUI
	for (auto currResult = results.begin(); currResult != results.end(); currResult++) {

		if (currResult->result.getP1Id() == currResult->result.getWinnerId()) {
			matchWindow->addResult(getMainAlias(currResult->result.getP1Id()), getMainAlias(currResult->result.getP2Id()), currResult->date, currResult->forfeit, false, currResult->desc);
		}
		else if (currResult->result.getP2Id() == currResult->result.getWinnerId()) {
			matchWindow->addResult(getMainAlias(currResult->result.getP2Id()), getMainAlias(currResult->result.getP1Id()), currResult->date, currResult->forfeit, false, currResult->desc);
		}
		else {
			matchWindow->addResult(getMainAlias(currResult->result.getP1Id()), getMainAlias(currResult->result.getP2Id()), currResult->date, currResult->forfeit, true, currResult->desc);
		}
	}
	matchWindow->sortResultTable();

	recalculateAllPeriods();

	SetSizerAndFit(mainSizer);

	// Binding functions to the events of the main window ("system calls" (close, mini-/maximize)).
	Bind(wxEVT_CLOSE_WINDOW, &MainWin::OnExit, this);
	// Events propagated from Rating Period tab
	Bind(wxEVT_BUTTON, &MainWin::OnRatPerAddBtn, this, ID_RAT_PER_ADD_BTN);
	Bind(wxEVT_BUTTON, &MainWin::OnRatPerRemBtn, this, ID_RAT_PER_REM_BTN);
	Bind(wxEVT_BUTTON, &MainWin::OnRatPerFinBtn, this, ID_RAT_PER_FIN_BTN);
	// Events propagated from Match Report tab
	Bind(wxEVT_BUTTON, &MainWin::OnMatRepAddBtn, this, ID_MAT_REP_ADD_BTN);
	Bind(wxEVT_BUTTON, &MainWin::OnMatRepRemBtn, this, ID_MAT_REP_REM_BTN);
	Bind(wxEVT_BUTTON, &MainWin::OnMatRepImportBtn, this, ID_MAT_REP_IMP_BTN);
	// Events propagated from Player Edit tab
	Bind(wxEVT_CHOICE, &MainWin::OnPlayerEditPlayerChoice, this, ID_PLA_EDIT_PLA_CHOICE);
	Bind(wxEVT_BUTTON, &MainWin::OnPlayerEditAliasAddBtn, this, ID_PLA_EDIT_ADD_ALIAS_BTN);
	Bind(wxEVT_BUTTON, &MainWin::OnPlayerEditAliasRemBtn, this, ID_PLA_EDIT_REM_ALIAS_BTN);
	Bind(wxEVT_BUTTON, &MainWin::OnPlayerEditAliasMainBtn, this, ID_PLA_EDIT_MAIN_ALIAS_BTN);
	Bind(wxEVT_CHECKBOX, &MainWin::OnPlayerEditToggleVisibility, this, ID_PLA_EDIT_HIDE_PLA_BTN);
	Bind(wxEVT_BUTTON, &MainWin::OnPlayerEditPlayerRemBtn, this, ID_PLA_EDIT_REM_BTN);

	// Event not handled in Settings/About tab
	Bind(wxEVT_CHECKBOX, &MainWin::OnSetAbtIncludeBox, this, ID_SET_ABT_INC_BOX);
	Bind(wxEVT_SPINCTRLDOUBLE, &MainWin::OnSetAbtTauSpin, this, ID_SET_ABT_TAU_SPIN);
}

void MainWin::finalize() {
	// set the current ratings as first element in the rating-vectors and delete the other entries.
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		// Count wins and losses per player and add to the wins/losses-field in their struct
		unsigned int wins = 0;
		unsigned int losses = 0;
		unsigned int ties = 0;
		for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {

			std::vector<Glicko2::Result> relevantResults;

			// fill relevantResults vector with Glicko-Results from period
			std::vector<Result> structResults = getResultsInPeriod(currPeriod->first, currPeriod->second, setAbtWindow->getIncludeForfeits());
			for (auto currStructResult = structResults.begin(); currStructResult != structResults.end(); currStructResult++) {
				relevantResults.push_back(currStructResult->result);
			}

			for (auto currResult = relevantResults.begin(); currResult != relevantResults.end(); currResult++) {

				bool isP1 = currPlayer->id == currResult->getP1Id();
				bool isP2 = currPlayer->id == currResult->getP2Id();
				
				// Check if player is in result at all
				if (isP1) {
					if (currResult->getP1Id() == currResult->getWinnerId()) {
						wins++;
					}
					else if (currResult->getP2Id() == currResult->getWinnerId()) {
						losses++;
					}
					else {
						ties++;
					}
				}
				else if (isP2) {
					if (currResult->getP2Id() == currResult->getWinnerId()) {
						wins++;
					}
					else if (currResult->getP1Id() == currResult->getWinnerId()) {
						losses++;
					}
					else {
						ties++;
					}
				}
			}
		}
		currPlayer->wins += wins;
		currPlayer->losses += losses;
		currPlayer->ties += ties;

		currPlayer->ratings[0] = currPlayer->ratings[currPlayer->ratings.size() - 1];
		currPlayer->ratings.resize(1);
	}
	for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
		periodWindow->removeRatingPeriod(currPeriod->first, currPeriod->second);
	}
	ratingPeriods.clear();

	matchWindow->clearResultTable();
	results.clear();
}

void MainWin::recalculateAllPeriods() {
	Glicko2::closeWorld();
	Glicko2::createWorld();

	std::map<unsigned int, unsigned int> oldNewIdMap = { {-1, -1} }; // initialize this way to assign a winID of -1 back to -1 (for ties)
	std::vector<Player> newPlayerBase; // this will replace the current playerBase in the end
	std::multiset<Result, bool(*)(const Result&, const Result&)> newResults(&compareResultDates); // this will replace the current results in the end

	for (unsigned int i = 0;  i < ratingPeriods.size(); i++) {
		// get results in current period
		std::vector<Result> relResults = getResultsInPeriod(ratingPeriods[i].first, ratingPeriods[i].second, setAbtWindow->getIncludeForfeits());
		// to save updated ID-results for only the current period (to apply glicko-algorithm)
		std::vector<Glicko2::Result> newRelevantResults;

		for (auto currResult = relResults.begin(); currResult != relResults.end(); currResult++) {
			// if the result contains a player ID, that has not been updated yet, create new player and map old ID to new one.
			if (oldNewIdMap.find(currResult->result.getP1Id()) == oldNewIdMap.end()) {
				unsigned int oldId = currResult->result.getP1Id();

				// lookup player in playerbase to retrieve ratings to create new player in glicko-world
				for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
					if (currPlayer->id == oldId) {
						// create with starting values and map new ID to old one
						oldNewIdMap[oldId] = Glicko2::createPlayer(currPlayer->ratings[0].rating, currPlayer->ratings[0].deviation, currPlayer->ratings[0].volatility);
						// assign new ID
						currPlayer->id = oldNewIdMap[oldId];

						// only update ID in the rating table if player is present in the first place
						if (currPlayer->visible) {
							periodWindow->updatePlayerID(oldId, currPlayer->aliases[0], oldNewIdMap[oldId]);
						}

						// remove from old playerbase to not have duplicate IDs
						newPlayerBase.push_back(*currPlayer);
						playerBase.erase(currPlayer);

						// player found, so move on to next one
						break;
					}
				}
			}
			// same for player 2 ID
			// if the result contains a player ID, that has not been updated yet, create new player and map old ID to new one.
			if (oldNewIdMap.find(currResult->result.getP2Id()) == oldNewIdMap.end()) {
				unsigned int oldId = currResult->result.getP2Id();

				// lookup player in playerbase to retrieve ratings to create new player in glicko-world
				for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
					if (currPlayer->id == oldId) {
						// create with starting values and map new ID to old one
						oldNewIdMap[oldId] = Glicko2::createPlayer(currPlayer->ratings[0].rating, currPlayer->ratings[0].deviation, currPlayer->ratings[0].volatility);
						// assign new ID
						currPlayer->id = oldNewIdMap[oldId];

						// only update ID in the rating table if player is present in the first place
						if (currPlayer->visible) {
							periodWindow->updatePlayerID(oldId, currPlayer->aliases[0], oldNewIdMap[oldId]);
						}

						// remove from old playerbase to not have duplicate IDs
						newPlayerBase.push_back(*currPlayer);
						playerBase.erase(currPlayer);

						// player found, so move on to next one
						break;
					}
				}
			}

			// Now to recreate the result with updated IDs
			Glicko2::Result newIdResult = Glicko2::Result(oldNewIdMap[currResult->result.getP1Id()],
				oldNewIdMap[currResult->result.getP2Id()], oldNewIdMap[currResult->result.getWinnerId()]);
			Result toAdd;
			toAdd.date = currResult->date;
			toAdd.forfeit = currResult->forfeit;
			toAdd.result = newIdResult;
			toAdd.desc = currResult->desc;

			// Insert it into the newRelevantResults-vector to apply glicko-algorithm (only for this period)
			newRelevantResults.push_back(toAdd.result);
			// Insert into new results-set to have all results recreated with new IDs in the end
			newResults.insert(toAdd);

			// Remove from old results-vector (if there are results not inside any period, they'll be left in here at the end)
			for (auto currResultFromVector = results.begin(); currResultFromVector != results.end(); currResultFromVector++) {
				// just compare all fields (old IDs in both), if there's "another" result with the same fields, we can just delete that one.
				if (currResult->date.IsSameDate(currResultFromVector->date) &&
					currResult->forfeit == currResultFromVector->forfeit &&
					currResult->result.getP1Id() == currResultFromVector->result.getP1Id() &&
					currResult->result.getP2Id() == currResultFromVector->result.getP2Id() &&
					currResult->result.getWinnerId() == currResultFromVector->result.getWinnerId()) 
				{
					results.erase(currResultFromVector);
					break;
				}
				
			}

		} // at this point every player in the current periods' results has been recreated in the Glicko-world, mapped to the new ID...
		//...and was pushed into the newPlayerBase (and does not appear in the old playerBase anymore).
		// Also the results IDs have been updated and then got removed from the results-vector and added to the new one.

		Glicko2::glicko(newRelevantResults);

		// Now update the newPlayerBase players' rating values for the current period. newPlayerBase only contains players already...
		// ...created in the new Glicko-world
		Glicko2::Player* realPlayer;
		for (auto currPlayer = newPlayerBase.begin(); currPlayer != newPlayerBase.end(); currPlayer++) { // Iterate over playerBase
			realPlayer = Glicko2::playerByID(currPlayer->id); // get Player-object from Glicko2-world
			//if(nullptr)????
			currPlayer->ratings[i + 1].rating = realPlayer->getRating();
			currPlayer->ratings[i + 1].deviation = realPlayer->getRD();
			currPlayer->ratings[i + 1].volatility = realPlayer->getVolatility();
		}

	}

	// after recreating all players/results from the rating periods there might be some players/results left, that were not within any rating period.
	// Recreate those with updated IDs
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		unsigned int oldId = currPlayer->id;

		oldNewIdMap[oldId] = Glicko2::createPlayer(currPlayer->ratings[0].rating, currPlayer->ratings[0].deviation, currPlayer->ratings[0].volatility);

		currPlayer->id = oldNewIdMap[oldId];

		// Players did not participate in any rated matches, so set all rating values to their start values
		std::vector<Rating>& ratingVector = currPlayer->ratings;
		for (auto currRating = ratingVector.begin(); currRating != ratingVector.end(); currRating++) {
			*currRating = ratingVector[0];
		}

		newPlayerBase.push_back(*currPlayer);

		// only update ID in the rating table if player is present in the first place
		if (currPlayer->visible) {
			periodWindow->updatePlayerID(oldId, currPlayer->aliases[0], oldNewIdMap[oldId]);
		}
	}

	for (auto currResult = results.begin(); currResult != results.end(); currResult++) {
		const Glicko2::Result& toRecreate = currResult->result;

		Glicko2::Result newIdResult = Glicko2::Result(oldNewIdMap[toRecreate.getP1Id()], oldNewIdMap[toRecreate.getP2Id()], oldNewIdMap[toRecreate.getWinnerId()]);

		Result toAdd;
		toAdd.date = currResult->date;
		toAdd.forfeit = currResult->forfeit;
		toAdd.result = newIdResult;
		toAdd.desc = currResult->desc;

		newResults.insert(toAdd);
	}

	playerBase.swap(newPlayerBase);
	results.swap(newResults);

	// Update rating period-tab rating table 
	// Count wins and losses per player
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		// only update when player is visible
		if (currPlayer->visible) {
			unsigned int wins = 0;
			unsigned int losses = 0;
			unsigned int ties = 0;


			for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
				std::vector<Glicko2::Result> relevantResults;

				// fill relevantResults vector with Glicko-Results from period
				std::vector<Result> structResults = getResultsInPeriod(currPeriod->first, currPeriod->second, setAbtWindow->getIncludeForfeits());
				for (auto currStructResult = structResults.begin(); currStructResult != structResults.end(); currStructResult++) {
					relevantResults.push_back(currStructResult->result);
				}

				for (auto currResult = relevantResults.begin(); currResult != relevantResults.end(); currResult++) {
					bool isP1 = currPlayer->id == currResult->getP1Id();
					bool isP2 = currPlayer->id == currResult->getP2Id();

					if (isP1) {
						if (currResult->getP1Id() == currResult->getWinnerId()) {
							wins++;
						}
						else if (currResult->getP2Id() == currResult->getWinnerId()) {
							losses++;
						}
						else {
							ties++;
						}
					}
					else if (isP2) {
						if (currResult->getP2Id() == currResult->getWinnerId()) {
							wins++;
						}
						else if (currResult->getP1Id() == currResult->getWinnerId()) {
							losses++;
						}
						else {
							ties++;
						}
					}
				}
			}
			periodWindow->updatePlayer(currPlayer->id, currPlayer->ratings[currPlayer->ratings.size() - 1].rating, currPlayer->wins + wins, currPlayer->losses + losses, currPlayer->ties + ties);
		}
	}
	periodWindow->sortMatchTable();
}

// TODO - reimplement
void MainWin::recalculateFromPeriod(const std::pair<wxDateTime, wxDateTime>& ratingPeriod) {
}


unsigned int MainWin::addNewPlayer(std::vector<std::string> atLeastOneAlias, std::vector<Rating>* optionalRatingVector, unsigned int wins, unsigned int losses, unsigned int ties, bool visibility) {
	if (atLeastOneAlias.empty()) {
		wxMessageBox(wxString("Tried to add player without alias for some reason. \n" "No player added."), wxString("Alias-list to initialize was empty"));
		return -1;
	}

	std::vector<Rating> ratingVector;
	if (optionalRatingVector == nullptr) {

		// create from default ratings
		Rating toAdd;
		toAdd.rating = setAbtWindow->getDefaultRating();
		toAdd.deviation = setAbtWindow->getDefaultDeviation();
		toAdd.volatility = setAbtWindow->getDefaultVolatility();

		ratingVector.push_back(toAdd);
		optionalRatingVector = &ratingVector;
	}

	// When a new player is created he has the same rating in ALL rating periods... 
	// AFTER this method the rating values will be adjusted.
	// If a result with new player in an older period is added the old values will be adjusted
	// It is important to create his ratingPeriods-vector with one entry for the start values AND one entry for each rating period that currently exists
	// When a ratingVector is provided as parameter it will be assumed that his first value is the starting value,...
	// ...the second the values after the first rating period and so on. Therefore, if the vector is smaller than 1(start values)+number of rating periods...
	// ...its last element will be pushed (so the number of elements is right AND contains the current value) until it is at least of that size.
	while (optionalRatingVector->size() < 1 + ratingPeriods.size()) {
		optionalRatingVector->push_back((*optionalRatingVector)[optionalRatingVector->size() - 1]);
	}

	// creating current player with the last(current) read rating values and...
	// storing the players' ID
	Rating& currentRatingValues = (*optionalRatingVector)[optionalRatingVector->size() - 1];
	unsigned int id = Glicko2::createPlayer(currentRatingValues.rating, currentRatingValues.deviation, currentRatingValues.volatility);

	// uniting the stored ID, aliases and ratings to add them to playerBase
	Player toAdd;
	toAdd.id = id;
	toAdd.aliases = atLeastOneAlias;
	toAdd.ratings = *optionalRatingVector;
	toAdd.wins = wins;
	toAdd.losses = losses;
	toAdd.ties = ties;
	toAdd.visible = visibility;

	playerBase.push_back(toAdd);;

	// Update match reports alias-vector
	matchWindow->setMainAliases(retrieveMainAliases());

	// Add player to the rating-periods rating table, if visible
	if (visibility) {
		periodWindow->addPlayer(id, atLeastOneAlias[0]);
		periodWindow->updatePlayer(id, (*optionalRatingVector)[optionalRatingVector->size() - 1].rating, 0, 0, 0);
		periodWindow->sortMatchTable();
	}

	// Add Alias to the alias selection in the player edit window
	playerEditWindow->setMainAliases(retrieveMainAliases());

	return id;
}

void MainWin::removePlayer(unsigned int id) {
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		// find player in playerBase
		if (currPlayer->id == id) {
			// remove all results including the player
			auto currResult = results.begin();
			for (; currResult != results.end(); ) {
				if (currResult->result.getP1Id() == id || currResult->result.getP2Id() == id) {
					std::string winnerAlias; 
					std::string loserAlias;

					if (currResult->result.getP2Id() == currResult->result.getWinnerId()) {
						winnerAlias = getMainAlias(currResult->result.getP2Id());
						loserAlias = getMainAlias(currResult->result.getP1Id());
					} else {
						// in this case either player 1 is the winner or the result is a tie in which case we wanna put player 1 first
						winnerAlias = getMainAlias(currResult->result.getP1Id());
						loserAlias = getMainAlias(currResult->result.getP2Id());
					}

					matchWindow->removeResult(winnerAlias, loserAlias, currResult->date); //TODO check back here if forfeit-field is needed

					currResult = results.erase(currResult);
				}
				else {
					currResult++;
				}
			}

			if (currPlayer->visible) {
				// remove from rating table window (if visible)
				periodWindow->removePlayer(id);
			}
			// remove from playerBase
			playerBase.erase(currPlayer);
			// remove from match report window's report-dialog
			matchWindow->setMainAliases(retrieveMainAliases());
			// remove player/last alias in player edit (and set to default option "<New player>"
			playerEditWindow->setMainAliases(retrieveMainAliases());
			playerEditWindow->setPlayersAliases(std::vector<std::string>());
			// recalculate players' ratings without the matches against this one
			recalculateAllPeriods();
			return;
		}
	}
}


const std::pair<wxDateTime, wxDateTime>* MainWin::findPeriod(wxDateTime& start, wxDateTime& end) {
	const std::pair<wxDateTime, wxDateTime>* toRet = nullptr;

	for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
		if (start.IsSameDate(currPeriod->first)) { // if entered periods start date is equal to one that's already entered, check...
			if (end.IsSameDate(currPeriod->second)) { // ... if end date also is already entered (duplicate)
				toRet = &(*currPeriod);
				return toRet;
			}
		}
	}
	return toRet;
}

std::vector<MainWin::Result> MainWin::getResultsInPeriod(const wxDateTime& start, const wxDateTime& end, bool includeForfeits) {
	std::vector<Result> toRet;

	for (auto currRes = results.begin(); currRes != results.end(); currRes++) {
		if ((currRes->date.IsLaterThan(start) || currRes->date.IsEqualTo(start)) &&
			(currRes->date.IsEarlierThan(end) || currRes->date.IsEqualTo(end))) {
			if ((!currRes->forfeit) || includeForfeits) {
				toRet.push_back(*currRes);
			}
		}
	}

	return toRet;
}


unsigned int MainWin::playerIDByAlias(std::string alias) {
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		for (auto currAlias = currPlayer->aliases.begin(); currAlias != currPlayer->aliases.end(); currAlias++) {
			if (alias == *currAlias) {
				return currPlayer->id;
			}
		}
	}
	return -1;
}

std::vector<std::string> MainWin::getPlayersAliases(unsigned int id) {
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		if (currPlayer->id == id) {
			return currPlayer->aliases;
		}
	}
	return std::vector<std::string>();
}

std::string MainWin::getMainAlias(unsigned int id) {
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		if (currPlayer->id == id) {
			return currPlayer->aliases[0];
		}
	}
	return "";
}

std::vector<std::string> MainWin::retrieveMainAliases() {
	std::vector<std::string> toRet;

	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		toRet.push_back(currPlayer->aliases[0]);
	}

	return toRet;
}

unsigned int MainWin::assignNewAlias(std::string aliasToAssign) {

	std::vector<std::string> existingMainAliases = retrieveMainAliases();


	// Create dialog to assign the unknown alias to a player or create a new one
	AssignPlayerDialog* assignDialog = new AssignPlayerDialog(this, wxID_ANY, wxString("Player ''" + aliasToAssign + " not found"), aliasToAssign, existingMainAliases,
		setAbtWindow->getDefaultRating(), setAbtWindow->getDefaultDeviation(), setAbtWindow->getDefaultVolatility());

	switch (assignDialog->ShowModal())
	{
	case wxID_OK:

		// If new player is created
		if (assignDialog->getAliasToAssignTo() == "") {
			std::vector<std::string> newPlayerAliases; // If new player is created, save his alias here
			newPlayerAliases.push_back(aliasToAssign);

			assignDialog->Destroy();

			std::vector<Rating> startValues;
			// Create start Rating
			Rating toAdd;
			toAdd.rating = assignDialog->getRating();
			toAdd.deviation = assignDialog->getDeviation();
			toAdd.volatility = assignDialog->getVolatility();

			startValues.push_back(toAdd);

			return addNewPlayer(newPlayerAliases, &startValues);
		}
		// if the alias is to be added to another player (by here it should already have been checked if it's a duplicate (non-main alias))
		else {
			// Assign alias to the chosen player
			for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
				for (auto currAlias = currPlayer->aliases.begin(); currAlias != currPlayer->aliases.end(); currAlias++) {
					if (*currAlias == assignDialog->getAliasToAssignTo()) {
						currPlayer->aliases.push_back(aliasToAssign);

						assignDialog->Destroy();
						return currPlayer->id;
					}
				}
			}
		}

	case wxID_CANCEL:
	default:
		assignDialog->Destroy();
	}

	return -1;
}

// --- Json-file loading and saving ---
bool MainWin::loadWorld() {
	std::ifstream ifs("players.json");

	if (ifs.fail()) { // If the file does not exist yet(first time running app), it will be created on exiting the app, when saving the settings.
		return false;
	}
	Json::CharReaderBuilder reader;
	Json::Value obj;
	JSONCPP_STRING errs;

	if (!Json::parseFromStream(reader, ifs, &obj, &errs)) {
		wxMessageBox(wxString("Couldn't parse players.json. Empty file? \n" "Playerbase not loaded, file will be ''fixed'' upon exiting.\n" "Error string:\n    " + errs), wxString("Playerbase not loaded."));
		return false;
	}

	const Json::Value& players = obj["players"]; // array of players

	// load rating periods before players so the number of rating values per player can be corrected if neccessary
	const Json::Value& periods = obj["rating periods"]; // array of start and end dates of all rating periods

	wxDateTime start;
	wxDateTime end;

	for (unsigned int i = 0; i < periods.size(); i++) {
		start.ParseISODate(periods[i]["start date"].asString());
		end.ParseISODate(periods[i]["end date"].asString());

		ratingPeriods.push_back(std::pair<wxDateTime, wxDateTime>(start, end));
	}
	std::sort(ratingPeriods.begin(), ratingPeriods.end(), comparePeriods);

	//Create a player for each element in players-array and push their ID to player base with their aliases
	for (unsigned int i = 0; i < players.size(); i++) {

		std::vector<std::string> aliases; // vector containing all known aliases of the current player

		// storing the players' aliases
		for (unsigned int j = 0; j < players[i]["aliases"].size(); j++) {
			aliases.push_back(players[i]["aliases"][j].asString());
		}

		std::vector<Rating> ratingVector; // vector containing Rating for each rating period (start values -> current)

		const Json::Value& ratingVals = players[i]["rating values"]; // array of current players' rating values (or rather rating values-objects)

		// storing the players' rating values per rating period
		for (unsigned int j = 0; j < ratingVals.size(); j++) {
			Rating toAdd;
			toAdd.rating = ratingVals[j]["rating"].asDouble();
			toAdd.deviation = ratingVals[j]["deviation"].asDouble();
			toAdd.volatility = ratingVals[j]["volatility"].asDouble();

			ratingVector.push_back(toAdd);
		}

		// could use the default values of addPlayer, but this spares us the ifception
		unsigned int wins = 0;
		unsigned int losses = 0;
		unsigned int ties = 0;

		if (players[i].isMember("wins")) {
			wins = players[i]["wins"].asUInt64();
		}
		if (players[i].isMember("losses")) {
			losses = players[i]["losses"].asUInt64();
		}
		if (players[i].isMember("ties")) {
			ties = players[i]["ties"].asUInt64();
		}

		if (players[i].isMember("visible")) {
			addNewPlayer(aliases, &ratingVector, wins, losses, ties, players[i]["visible"].asBool());
		}
		else {
			addNewPlayer(aliases, &ratingVector, wins, losses, ties);
		}
	}

	ifs.close();

	return ifs.good();
}

bool MainWin::saveWorld() {
	std::ofstream ofs = std::ofstream("players.json", std::ios_base::trunc);

	if (!ofs) {
		wxMessageBox(wxString("Opening players.json/output-stream not successful. Playerbase and Rating periods will not be saved."));
		return false;
	}

	Json::StreamWriterBuilder builder;
	Json::Value playersJson; // to create the whole json-object to write to file

	Json::Value allPlayers(Json::arrayValue);

	for (auto currPlayerBasePlayer = playerBase.begin(); currPlayerBasePlayer != playerBase.end(); currPlayerBasePlayer++) { // iterate through playerbase
		Json::Value currEntry; // to hold the entry of the currently saved player
		Json::Value currAliases(Json::arrayValue); // create the aliases section of the player

		for (auto currAlias = currPlayerBasePlayer->aliases.begin(); currAlias != currPlayerBasePlayer->aliases.end(); currAlias++) { // iterate through aliases of current player
			currAliases.append(Json::Value(*currAlias)); // filling the aliases section with the aliases of the current player
		}

		Json::Value currPlayerRatings(Json::arrayValue); // equal to the "rating values" in players.json (to save all rating values for every period)

		for (auto currRating = currPlayerBasePlayer->ratings.begin(); currRating != currPlayerBasePlayer->ratings.end(); currRating++) {
			Json::Value ratingValsToAdd; // Contains the rating values of the currently saved period
			ratingValsToAdd["rating"] = currRating->rating;
			ratingValsToAdd["deviation"] = currRating->deviation;
			ratingValsToAdd["volatility"] = currRating->volatility;
			currPlayerRatings.append(ratingValsToAdd);
		}

		currEntry["rating values"] = currPlayerRatings;
		currEntry["wins"] = currPlayerBasePlayer->wins;
		currEntry["losses"] = currPlayerBasePlayer->losses;
		currEntry["ties"] = currPlayerBasePlayer->ties;
		currEntry["aliases"] = currAliases;
		currEntry["visible"] = currPlayerBasePlayer->visible;

		allPlayers.append(currEntry);
	}
	playersJson["players"] = allPlayers;

	Json::Value ratingPeriodDates(Json::arrayValue);

	Json::Value currPeriodToAdd;

	for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
		currPeriodToAdd["start date"] = (*currPeriod).first.FormatISODate().ToStdString();
		currPeriodToAdd["end date"] = (*currPeriod).second.FormatISODate().ToStdString();
		ratingPeriodDates.append(currPeriodToAdd);
	}

	playersJson["rating periods"] = ratingPeriodDates;

	Json::StreamWriter* writer = builder.newStreamWriter();
	writer->write(playersJson, &ofs);

	delete writer;

	ofs.close();
	return ofs.good();
}

bool MainWin::loadResults() {
	std::ifstream ifs("results.json");

	if (ifs.fail()) { // If the file does not exist yet(first time running app), it will be created on exiting the app, when saving the settings. 
		return false;
	}
	Json::CharReaderBuilder reader;
	Json::Value obj;
	JSONCPP_STRING errs;

	if (!Json::parseFromStream(reader, ifs, &obj, &errs)) {
		wxMessageBox(wxString("Couldn't parse results.json. Empty file? \n" "Results not loaded, file will be ''fixed'' upon exiting.\n" "Error string:\n    " + errs), wxString("Results not loaded."));
		return false;
	}

	const Json::Value& resultsLoaded = obj["results"]; // array of results consisting of the main-alias of winner and loser and date

	for (auto currResult = resultsLoaded.begin(); currResult != resultsLoaded.end(); currResult++) {
		unsigned int p1Id = playerIDByAlias((*currResult)["player 1"].asString());
		unsigned int p2Id = playerIDByAlias((*currResult)["player 2"].asString());
		bool p1Won = (*currResult)["player 1"].asString() == (*currResult)["winner"].asString();
		bool tie = (*currResult)["winner"].asString() == "";

		if (p1Id == -1) {
			wxMessageBox(wxString("Unable to assign player with the following alias to a player from the playerbase: " +
				(*currResult)["player 1"].asString() + ". \n" "Please assign manually or result will be disregarded"),
				wxString("Unexpected alias while reading results.json"));
			p1Id = assignNewAlias((*currResult)["player 1"].asString());
		}

		// We do not wanna show the assignPlayerDialog again, if it was shown for the winner and the user clicked cancel (winnerID still -1)
		if (p2Id == -1 && p1Id != -1) {
			wxMessageBox(wxString("Unable to assign player with the following alias to a player from the playerbase: " +
				(*currResult)["player 2"].asString() + ". \n" "Please assign manually or result will be disregarded"),
				wxString("Unexpected alias while reading results.json"));
			p2Id = assignNewAlias((*currResult)["player 2"].asString());
		}

		// if one of the IDs is still -1 here or both players got assigned to the same name, skip adding the result
		if (!(p1Id == -1 || p2Id== -1 || p1Id == p2Id)) {
			unsigned int winnerId;
			if (tie) {
				winnerId = -1;
			}
			else {
				if (p1Won) {
					winnerId = p1Id;
				}
				else {
					winnerId = p2Id;
				}
			}
			Glicko2::Result theResult = Glicko2::Result(p1Id, p2Id, winnerId);

			wxDateTime resultsDate;
			if (!resultsDate.ParseISODate((*currResult)["date"].asString())) {
				wxMessageBox(wxString("Unable to parse following date: " + (*currResult)["date"].asString() + "\n" "Result will be disregarded"),
					wxString("Unexpected date string while reading results.json"));
			}
			else {
				Result toAdd;
				toAdd.result = theResult;
				toAdd.date = resultsDate;
				toAdd.forfeit = (*currResult)["forfeit"].asBool();
				toAdd.desc = (*currResult)["description"].asString();

				results.insert(toAdd);
			}
		}
		else if (p1Id == p2Id && p1Id != -1) {
			wxMessageBox(wxString("Result not added. Both aliases were assigned to the same player."));
		}
	}

	ifs.close();

	return ifs.good();
}

bool MainWin::saveResults() {
	std::ofstream ofs = std::ofstream("results.json", std::ios_base::trunc);

	if (!ofs) {
		wxMessageBox(wxString("Opening results.json/output-stream not successful. Results will not be saved."));
		return false;
	}
	Json::StreamWriterBuilder builder;
	Json::Value resultsJson; // to create the whole json-object to write to file

	Json::Value allResults(Json::arrayValue);

	for (auto currResult = results.begin(); currResult != results.end(); currResult++) { // iterate through playerbase
		Json::Value resultToAdd;
		resultToAdd["player 1"] = getMainAlias(currResult->result.getP1Id());
		resultToAdd["player 2"] = getMainAlias(currResult->result.getP2Id());
		resultToAdd["winner"] = getMainAlias(currResult->result.getWinnerId());
		resultToAdd["date"] = currResult->date.FormatISODate().ToStdString();
		resultToAdd["forfeit"] = currResult->forfeit;
		resultToAdd["description"] = currResult->desc;

		allResults.append(resultToAdd);
	}

	resultsJson["results"] = allResults;

	Json::StreamWriter* writer = builder.newStreamWriter();
	writer->write(resultsJson, &ofs);

	delete writer;

	ofs.close();
	return ofs.good();
}

bool MainWin::loadSettings() {
	std::ifstream ifs("settings.json");

	if (ifs.fail()) { // If the file does not exist yet(first time running app), it will be created on exiting the app, when saving the settings. 
		return false;
	}
	Json::CharReaderBuilder reader;
	Json::Value obj;
	JSONCPP_STRING errs;

	if (!Json::parseFromStream(reader, ifs, &obj, &errs)) {
		wxMessageBox(wxString("Couldn't parse settings.json. Empty file? \n" "Settings not loaded, default values will be used. File will be ''fixed'' upon exiting.\n" "Error string:\n    " + errs), wxString("Settings not loaded."));
		return false;
	}

	const Json::Value& settingsLoaded = obj["settings"]; // array of settings consisting of the default values set in settings/about tab

	setAbtWindow->setAPIKey(settingsLoaded["api key"].asString());
	setAbtWindow->setIncludeForfeits(settingsLoaded["include forfeits"].asBool());

	setAbtWindow->setDefaultRating(settingsLoaded["default values"]["rating"].asDouble());
	setAbtWindow->setDefaultDeviation(settingsLoaded["default values"]["deviation"].asDouble());
	setAbtWindow->setDefaultVolatility(settingsLoaded["default values"]["volatility"].asDouble());
	setAbtWindow->setTau(settingsLoaded["tau"].asDouble());

	ifs.close();

	return ifs.good();

}

bool MainWin::saveSettings() {
	std::ofstream ofs = std::ofstream("settings.json", std::ios_base::trunc);

	if (!ofs) {
		wxMessageBox(wxString("Opening settings.json/output-stream not successful. Settings will not be saved."));
		return false;
	}
	Json::StreamWriterBuilder builder;
	Json::Value settingsJson; // to create the whole json-object to write to file

	Json::Value defaultVals;

	defaultVals["rating"] = setAbtWindow->getDefaultRating();
	defaultVals["deviation"] = setAbtWindow->getDefaultDeviation();
	defaultVals["volatility"] = setAbtWindow->getDefaultVolatility();

	Json::Value challongeUser;

	settingsJson["settings"]["default values"] = defaultVals;
	settingsJson["settings"]["api key"] = setAbtWindow->getAPIKey();
	settingsJson["settings"]["include forfeits"] = setAbtWindow->getIncludeForfeits();
	settingsJson["settings"]["tau"] = setAbtWindow->getTau();

	Json::StreamWriter* writer = builder.newStreamWriter();
	writer->write(settingsJson, &ofs);

	delete writer;

	ofs.close();
	return ofs.good();
}

// --- Bind-methods ---
// Rating periods tab
void MainWin::OnRatPerAddBtn(wxCommandEvent& event) {
	std::pair<wxDateTime, wxDateTime>* thePeriod = (std::pair<wxDateTime, wxDateTime>*)event.GetClientData();

	if (findPeriod(thePeriod->first, thePeriod->second) != nullptr) {
		wxMessageBox(wxString("Entered rating period already added!"), wxString("Duplicate detected"));
		delete thePeriod;
		return;
	}
	for (auto currRatingPeriod = ratingPeriods.begin(); currRatingPeriod != ratingPeriods.end(); currRatingPeriod++) {
		// if entered periods start date is before an existing one's AND...
		// entered end date is after the existing one's start date, they overlap (repeat for existing one's end date)
		if ((thePeriod->first.IsEarlierThan(currRatingPeriod->first) && thePeriod->second.IsLaterThan(currRatingPeriod->first)) || 
			(thePeriod->first.IsEarlierThan(currRatingPeriod->second) && thePeriod->second.IsLaterThan(currRatingPeriod->second))) {
			wxMessageBox(wxString("Entered Rating period overlaps with an existing one!"), wxString("Overlap detected"));
			delete thePeriod;
			return;
		}
	}

	periodWindow->addRatingPeriod(thePeriod->first, thePeriod->second);	

	// Insert new entry in ratingValues-vector for every player at the right position ([0] in the vector are starting values, [1] after first period).
	// It's (hopefully) safe to assume that we can insert at the same index each time (duh?)
	ratingPeriods.push_back(*thePeriod);
	std::sort(ratingPeriods.begin(), ratingPeriods.end(), comparePeriods);

	unsigned int index = 0; // start at the starting values, if the first period is added, we wanna read those
	// count the steps it takes to get to our newly added rating period (they are sorted by date, so the index corresponds with the rating values after the period)
	for (unsigned int i = 0; ratingPeriods[i] != *thePeriod; i++) {
		index = i + 1;
	}

	// Extend ratingvectors to have an entry for each rating period + starting values
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {

		if (ratingPeriods.size() + 1 == currPlayer->ratings.size()) {
			wxMessageBox(wxString("Rating-vector already has the right amount of entries."));
		}
		else {
			// just copy the last right rating values
			currPlayer->ratings.insert(currPlayer->ratings.begin() + index, currPlayer->ratings[index]);
		}
	}

	// TODO use again, once reimplemented
	//recalculateFromPeriod(*thePeriod);
	recalculateAllPeriods();

	delete thePeriod;
}

void MainWin::OnRatPerRemBtn(wxCommandEvent& event) {
	std::pair<wxDateTime, wxDateTime>* periodToRem = (std::pair<wxDateTime, wxDateTime>*)event.GetClientData();

	const std::pair<wxDateTime, wxDateTime>* found = findPeriod(periodToRem->first, periodToRem->second);

	if (found != nullptr) {
		wxDateTime startOfRemoved = found->first;

		for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
			if ((&(*currPeriod) == found)) {
				ratingPeriods.erase(currPeriod);
				break; // iterator is invalid now
			}
		}

		periodWindow->removeRatingPeriod(periodToRem->first, periodToRem->second);

		// get ratingPeriod that was before *found.
		const std::pair<wxDateTime, wxDateTime>* prevPeriod = nullptr;
		for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
			if (currPeriod->second.IsEarlierThan(startOfRemoved)) {
				prevPeriod = &(*currPeriod);
			}
			else {
				break;
			}
		}

		// adjust ratingVectors. Drop one entry that comes AFTER the last value that was not influenced by the rating period that was removed
		for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
			currPlayer->ratings.pop_back();
		}

		if (prevPeriod != nullptr) { // if this is nullptr the first period got removed

			// TODO use again, once reimplemented
			//recalculateFromPeriod(*prevPeriod);
			recalculateAllPeriods();
		}
		else {
			recalculateAllPeriods();
		}
	}
	else {
		wxMessageBox(wxString("Tried to remove non-existant rating period somehow. Please report!"), wxString("Unexpected behaviour"));
	}

	delete periodToRem;
}

void MainWin::OnRatPerFinBtn(wxCommandEvent& event) {
	// Just the handler for the button press. Call finalize-method, because it could be useful to have it in a separate method.
	finalize();
}

// Match report tab
void MainWin::OnMatRepAddBtn(wxCommandEvent& event) {
	// resultTuple: 0: winner, 1: loser, 2: date, 3: forfeit, 4: tie, 5: Description
	std::tuple<std::string, std::string, wxDateTime, bool, bool, std::string>* resultTuple = (std::tuple<std::string, std::string, wxDateTime, bool, bool, std::string>*)event.GetClientData();
	if (resultTuple == nullptr) {
		wxMessageBox(wxString("Result could not be inserted."), wxString("Invalid result"));
		delete resultTuple;
		return;
	}

	// Retrieve the ID to have access to the playerBase entry
	unsigned int winID = playerIDByAlias(std::get<0>(*resultTuple));
	unsigned int loseID = playerIDByAlias(std::get<1>(*resultTuple));

	// Handle unknown player alias (if a name was added using the edit field in the addResultDialog) (we can be sure the alias does not exist yet, coz look two lines up)
	if (winID == -1) {
		winID = assignNewAlias(std::get<0>(*resultTuple));

		// if winID is still -1, the user clicked cancel or closed the dialog (not clicking OK)
		if (winID == -1) {
			delete resultTuple;
			return;
		}
	}
	if (loseID == -1) {
		loseID = assignNewAlias(std::get<1>(*resultTuple));

		// if loseID is still -1, the user clicked cancel or closed the dialog (not clicking OK)
		if (loseID == -1) {
			delete resultTuple;
			return;
		}
	}

	// Check if during the assignPlayerDialog the players got assigned to the same player
	if (winID == loseID) {
		wxMessageBox(wxString("Both aliases got assigned to the same player. \n" "Result will not be added."), wxString("Invalid result"));
		delete resultTuple;
		return;
	}

	Glicko2::Result resultToAdd;
	if (std::get<4>(*resultTuple)) {
		resultToAdd = Glicko2::Result(winID, loseID, -1);
	}
	else {
		resultToAdd = Glicko2::Result(winID, loseID, winID);
	}

	Result toAdd;
	toAdd.result = resultToAdd;
	toAdd.date = std::get<2>(*resultTuple);
	toAdd.forfeit = std::get<3>(*resultTuple);
	toAdd.desc = std::get<5>(*resultTuple);

	results.insert(toAdd);
	matchWindow->addResult(getMainAlias(winID), getMainAlias(loseID), std::get<2>(*resultTuple), toAdd.forfeit, std::get<4>(*resultTuple), toAdd.desc);
	matchWindow->sortResultTable();

	for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
		if ((std::get<2>(*resultTuple).IsLaterThan(currPeriod->first) || std::get<2>(*resultTuple).IsEqualTo(currPeriod->first)) 
			&& (std::get<2>(*resultTuple).IsEarlierThan(currPeriod->second) || std::get<2>(*resultTuple).IsEqualTo(currPeriod->second))) {
			// TODO use again, once reimplemented
			// recalculateFromPeriod(*currPeriod);
			recalculateAllPeriods();
		}
	}

	delete resultTuple;
}

void MainWin::OnMatRepRemBtn(wxCommandEvent& event) {
	// data : 0: winner, 1: loser, 2: date, 3: forfeit, 4: tie, 5: Description
	std::tuple <std::string, std::string, wxDateTime, bool, bool, std::string>* data = (std::tuple <std::string, std::string, wxDateTime, bool, bool, std::string>*)event.GetClientData();

	// Go through results until result to be removed is found
	for(auto currResult = results.begin(); currResult != results.end(); currResult++) {
		// compare Dates first
		if (currResult->date.IsSameDate(std::get<2>(*data))) {

			// check if given winner(player 1 for ties) alias is a player in the current result
			if (getMainAlias(currResult->result.getP1Id()) == std::get<0>(*data)) {
				// compare other alias
								
				if (getMainAlias(currResult->result.getP2Id()) == std::get<1>(*data)) {
					// the result-flag of the result to remove and the current one has to be the same
					if (std::get<3>(*data) == currResult->forfeit) {

						// if the result to remove is a tie the winnerId should be -1 (or we'd remove the wrong result)
						if ((std::get<4>(*data) && currResult->result.getWinnerId() == -1)
							|| (!std::get<4>(*data) && currResult->result.getWinnerId() != -1)) {
							// lastly, the description (or lack thereof) has to be the same for both
							if (std::get<5>(*data) == currResult->desc) {
								results.erase(currResult);
								matchWindow->removeResult();

								// TODO use again, once recalculateFromPeriod is reimplemented (or just leave as is)...
								/*
								// If match was inside of an existing rating period recalculate from that period on
								for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
									if ((std::get<2>(*data).IsLaterThan(currPeriod->first) || std::get<2>(*data).IsEqualTo(currPeriod->first))
										&& (std::get<2>(*data).IsEarlierThan(currPeriod->second) || std::get<2>(*data).IsEqualTo(currPeriod->second))) {
										//recalculateFromPeriod(*currPeriod);
									}
								}
								*/
								recalculateAllPeriods();

								delete data;
								return;
							}
						}
					}
				}
			}
		}
	}
	wxMessageBox(wxString("Internal result to remove not found."), wxString("Unexpected result"));
	delete data;
}

void MainWin::OnMatRepImportBtn(wxCommandEvent& event) {
	if (setAbtWindow->getAPIKey() == "") {
		wxMessageBox(wxString("Please enter an API key in the Settings/About-tab."), wxString("No API key supplied"));
		return;
	}
	
	wxTextEntryDialog* challongeURL = new wxTextEntryDialog(this, wxString("Please enter the ID of the challonge-bracket/tourney to import results from.\n" 
		"That is the URL without ''challonge.com/''"), wxString("Enter challonge-bracket ID"));

	std::string bracketID;
	std::string::iterator currChar;
	switch (challongeURL->ShowModal()) {
	case wxID_OK:
		bracketID = challongeURL->GetValue().ToStdString();
		challongeURL->Destroy();
		// remove spaces
		currChar = bracketID.begin();
		while(currChar != bracketID.end()) {
			if (*currChar == ' ') {
				currChar = bracketID.erase(currChar);
			}
			else {
				currChar++;
			}
		}
		//check if empty string was entered
		if (bracketID == "") { 
			wxMessageBox(wxString("Please enter a challonge-bracket ID."), wxString("Nothing entered"));
			return; 
		}
		break;
	case wxID_CANCEL:
	case wxID_EXIT:
		return;
	}
	// Request URI: https://api.challonge.com/v1/tournaments/'bracketID'/matches.json?api_key='api_key' 
	std::string apiUrl = "https://api.challonge.com/v1/tournaments/" + bracketID + "/participants.json?api_key=" + setAbtWindow->getAPIKey();

	GetRequester getResponses = GetRequester();

	std::string response = getResponses.sendRequest(apiUrl);

	long responseCode = getResponses.getResponseCode();
	if (responseCode != 200) {
		std::string errorString = "Did not get expected response, when trying to retrieve player list.";

		switch(responseCode) {
		case 401:
			errorString = "''Unauthorized'' response code. Wrong api key?";
			break;
		case 404:
			errorString = "Response code implies that the entered ID does not represent an existing bracket.";
			break;
		case 500:
			errorString = "External error. Challonge.com might be down. Please try again (later).";
			break;
		case 619:
			errorString = "Timeout while connecting. Please try again.";
			break;
		case 690:
			errorString = "Timeout in file transfer. Please try again.";
		}

		wxMessageBox(wxString(errorString),	wxString("Response Code was: " + std::to_string(getResponses.getResponseCode())));
		return;
	}

	// Parsing downloaded .json-file to json-object
	std::stringstream theJson = std::stringstream(response);

	Json::CharReaderBuilder reader;
	Json::Value obj;
	JSONCPP_STRING errs;

	if (!Json::parseFromStream(reader, theJson, &obj, &errs)) {
		wxMessageBox(wxString("Couldn't parse participants list    " + errs), wxString("Participants could not be parsed."));
		return;
	}

	// maps participant's ID to display name
	std::map<unsigned int, std::string> participantsMap = std::map<unsigned int, std::string>();
	for (unsigned int i = 0; i < obj.size(); i++) {
		participantsMap[obj[i]["participant"]["id"].asUInt()] = obj[i]["participant"]["display_name"].asString();
	}


	// get {tournament ID}.json containing information about the tournament
	apiUrl = "https://api.challonge.com/v1/tournaments/" + bracketID + ".json?api_key=" + setAbtWindow->getAPIKey();

	response = getResponses.sendRequest(apiUrl);

	responseCode = getResponses.getResponseCode();
	if (responseCode != 200) {
		std::string errorString = "Did not get expected response, when trying to retrieve match list.";

		switch (responseCode) {
		case 401:
			errorString = "''Unauthorized'' response code. Wrong api key?";
			break;
		case 404:
			errorString = "Response code implies that the entered ID does not represent an existing bracket.";
			break;
		case 500:
			errorString = "External error. Challonge.com might be down. Please try again (later).";
			break;
		case 619:
			errorString = "Timeout while connecting. Please try again.";
			break;
		case 690:
			errorString = "Timeout in file transfer. Please try again.";
		}

		wxMessageBox(wxString(errorString), wxString("Response Code was: " + std::to_string(getResponses.getResponseCode())));
		return;
	}

	// Parsing downloaded .json-file to json-object
	theJson = std::stringstream(response);

	obj.clear();
	errs.clear();

	if (!Json::parseFromStream(reader, theJson, &obj, &errs)) {
		wxMessageBox(wxString("Couldn't parse tournament info:    " + errs), wxString("Tournament info could not be retrieved."));
		return;
	}

	
	// get date
	wxDateTime dateOfTourney;
	dateOfTourney.ParseFormat(wxString(obj["tournament"]["started_at"].asString().substr(0, 10)), wxString("%Y-%m-%d"));

	std::string descOfTourney = obj["tournament"]["name"].asString();


	// get matches.json containing a list of matches
	apiUrl = "https://api.challonge.com/v1/tournaments/" + bracketID + "/matches.json?api_key=" + setAbtWindow->getAPIKey();

	response = getResponses.sendRequest(apiUrl);

	responseCode = getResponses.getResponseCode();
	if (responseCode != 200) {
		std::string errorString = "Did not get expected response, when trying to retrieve match list.";

		switch (responseCode) {
		case 401:
			errorString = "''Unauthorized'' response code. Wrong api key?";
			break;
		case 404:
			errorString = "Response code implies that the entered ID does not represent an existing bracket.";
			break;
		case 500:
			errorString = "External error. Challonge.com might be down. Please try again (later).";
			break;
		case 619:
			errorString = "Timeout while connecting. Please try again.";
			break;
		case 690:
			errorString = "Timeout in file transfer. Please try again.";
		}

		wxMessageBox(wxString(errorString), wxString("Response Code was: " + std::to_string(getResponses.getResponseCode())));
		return;
	}

	// Parsing downloaded .json-file to json-object
	theJson = std::stringstream(response);

	obj.clear();
	errs.clear();

	if (!Json::parseFromStream(reader, theJson, &obj, &errs)) {
		wxMessageBox(wxString("Couldn't parse match list    " + errs), wxString("Matches could not be parsed."));
		return;
	}

	for (unsigned int i = 0; i < obj.size(); i++) {
		Json::Value& currMatch = obj[i]["match"];
		if (currMatch["state"].asString() == "complete") {

			std::string scoreString = currMatch["scores_csv"].asString();

			// if the set is not forfeited (if there's a negative value in the results like -1-0, there'll always be more than one '-')
			if (!(currMatch["forfeited"].asBool() 
				|| std::count(scoreString.begin(), scoreString.end(), '-') > 1)) {

				unsigned int p1Id = currMatch["player1_id"].asUInt();
				unsigned int p2Id = currMatch["player2_id"].asUInt();
				if (playerIDByAlias(participantsMap[p1Id]) == -1) {
					assignNewAlias(participantsMap[p1Id]);
				}
				// if id still can't be found (because cancel was clicked), disregard this match
				if (playerIDByAlias(participantsMap[p1Id]) != -1) {
					// translate winId from challonge to internal ID
					p1Id = playerIDByAlias(participantsMap[p1Id]);

					// repeat whole procedure for other player
					if (playerIDByAlias(participantsMap[p2Id]) == -1) {
						assignNewAlias(participantsMap[p2Id]);
					}
					if (playerIDByAlias(participantsMap[p2Id]) != -1) {
						p2Id = playerIDByAlias(participantsMap[p2Id]);

						// continue in here, coz we don't wanna continue if cancel was clicked on losing player
						// create Glicko2-result, add with date to results-set
						// now created by score-string to get winner by points...
						// by here it's safe to assume the result will be entered

						// get scores
						std::string p1Score = scoreString.substr(0, scoreString.find_first_of("-"));
						std::string p2Score = scoreString.substr(scoreString.find_first_of("-") + 1);

						// set winner and loser by scores
						if (std::stoi(p1Score) > std::stoi(p2Score)) {
							// add results to result-collection-thingy(set, a set of sets, ha!)
							Glicko2::Result resultToAdd = Glicko2::Result(p1Id, p2Id, p1Id);

							Result toAdd;
							toAdd.result = resultToAdd;
							toAdd.date = dateOfTourney;
							toAdd.forfeit = false;
							toAdd.desc = descOfTourney;

							results.insert(toAdd);
							matchWindow->addResult(getMainAlias(p1Id), getMainAlias(p2Id), dateOfTourney, false, false, descOfTourney);
						}
						else if (std::stoi(p1Score) < std::stoi(p2Score)) {
							// add results to result-collection-thingy(set, a set of sets, ha!)
							Glicko2::Result resultToAdd = Glicko2::Result(p2Id, p1Id, p2Id);

							Result toAdd;
							toAdd.result = resultToAdd;
							toAdd.date = dateOfTourney;
							toAdd.forfeit = false;
							toAdd.desc = descOfTourney;

							results.insert(toAdd);
							matchWindow->addResult(getMainAlias(p2Id), getMainAlias(p1Id), dateOfTourney, false, false, descOfTourney);
						}
						else {
							// if the scores are equal, it's a tie
							Glicko2::Result resultToAdd = Glicko2::Result(p1Id, p2Id, -1);

							Result toAdd;
							toAdd.result = resultToAdd;
							toAdd.date = dateOfTourney;
							toAdd.forfeit = false;
							toAdd.desc = descOfTourney;

							results.insert(toAdd);
							matchWindow->addResult(getMainAlias(p1Id), getMainAlias(p2Id), dateOfTourney, false, true, descOfTourney);
						}
					}
				}
			}
			// if match is forfeited, add as forfeited match
			else {
				unsigned int winId = currMatch["winner_id"].asUInt();
				unsigned int loseId = currMatch["loser_id"].asUInt();

				if (playerIDByAlias(participantsMap[winId]) == -1) {
					assignNewAlias(participantsMap[winId]);
				}
				// if id still can't be found (because cancel was clicked), disregard this match
				if (playerIDByAlias(participantsMap[winId]) != -1) {
					// translate winId from challonge to internal ID
					winId = playerIDByAlias(participantsMap[winId]);

					// repeat whole procedure for other player
					if (playerIDByAlias(participantsMap[loseId]) == -1) {
						assignNewAlias(participantsMap[loseId]);
					}
					if (playerIDByAlias(participantsMap[loseId]) != -1) {
						loseId = playerIDByAlias(participantsMap[loseId]);

						// same as above just for forfeited match

						// add results to result-collection-thingy(set, a set of sets, ha!)
						Glicko2::Result resultToAdd = Glicko2::Result(winId, loseId, winId);

						Result toAdd;
						toAdd.result = resultToAdd;
						toAdd.date = dateOfTourney;
						toAdd.forfeit = true;
						toAdd.desc = descOfTourney;

						results.insert(toAdd);
						matchWindow->addResult(getMainAlias(winId), getMainAlias(loseId), dateOfTourney, true, false, descOfTourney);
					}
				}
			}
		}
	}

	matchWindow->sortResultTable();
	// for now recalculate all periods... technically results from the same tourney can be in different rating periods, by being reported then...
	// ... maybe just take one date, but tournament infos do not contain a good value for that (like tourney completion_date)
	recalculateAllPeriods();
}

// Player edit tab
void MainWin::OnPlayerEditPlayerChoice(wxCommandEvent& event) {
	std::string* theAlias = (std::string*)event.GetClientData();
	unsigned int id = playerIDByAlias(*theAlias);
	std::vector<std::string> mainAliases = getPlayersAliases(id);
	playerEditWindow->setPlayersAliases(mainAliases);

	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		if (currPlayer->id == id) {
			// the default is that a player is visible and the checkbox is unchecked (which means true for the box means a hidden player...
			// ...but true for a player means shown
			playerEditWindow->setHidden(!currPlayer->visible);
		}
	}

	delete theAlias;
}

void MainWin::OnPlayerEditAliasAddBtn(wxCommandEvent& event) {
	std::pair<std::string, std::string>* theAliases = (std::pair<std::string, std::string>*)event.GetClientData();

	unsigned int toAddAliasId = playerIDByAlias(theAliases->first);

	if (toAddAliasId == -1) { // better than theAliases->first == "<New player>"
		unsigned int id = assignNewAlias(theAliases->second);

		delete theAliases;

		if (id == -1) {
			wxMessageBox(wxString("Assignment was cancelled. Player will not be added."), wxString("Player creation cancelled"));
			return;
		}

		playerEditWindow->setMainAliases(retrieveMainAliases(), getMainAlias(id));
		playerEditWindow->setPlayersAliases(getPlayersAliases(id));
		return;
	}

	if (playerIDByAlias(theAliases->second) != -1) {
		wxMessageBox(wxString("The entered alias is already in use. No change will be made."), wxString("Duplicate alias"));
		delete theAliases;
		return;
	}

	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		if (currPlayer->id == toAddAliasId) {
			currPlayer->aliases.push_back(theAliases->second);
			playerEditWindow->setPlayersAliases(currPlayer->aliases);
			break; // else there could technically be a case where theAliases is not deleted (if I put delete and return in here)
		}
	}
	delete theAliases;
}

void MainWin::OnPlayerEditAliasRemBtn(wxCommandEvent& event) {
	std::string* toRem = (std::string*)event.GetClientData(); // alias to remove

	unsigned int toRemId = playerIDByAlias(*toRem); // associate alias to remove with ID of the player
	if (toRemId == -1) {
		wxMessageBox(wxString("Player to remove alias from not found. No changes made."), wxString("Unable to lookup player"));
		delete toRem;
		return;
	}

	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		// check if current player has the ID of the player whose alias to remove
		if (currPlayer->id == toRemId) {
			std::vector<std::string>& aliasVector = currPlayer->aliases;
			// search through his aliases
			for (auto currAlias = aliasVector.begin(); currAlias != aliasVector.end(); currAlias++) {
				if (*currAlias == *toRem) {
					// if only alias is about to be removed
					if (aliasVector.size() == 1) {
						wxMessageDialog* removePlayerDialog = new wxMessageDialog(this, wxString("This will remove the players' last alias."
							"The player and all associated results will be removed. Continue?"), 
							wxString("Player about to be deleted"), wxYES_NO | wxCANCEL);
						switch (removePlayerDialog->ShowModal()) {
						case wxID_YES:
							removePlayer(currPlayer->id);
							delete toRem;
							return;
						case wxID_CANCEL:
						case wxID_NO:
							delete toRem;
							return;
						}
					
					}
					
					aliasVector.erase(currAlias);
					// if main alias was removed, update mainAliases
					playerEditWindow->setMainAliases(retrieveMainAliases(), aliasVector[0]);
					playerEditWindow->setPlayersAliases(aliasVector);
					delete toRem;
					return;
				}
			}
		}
	}
	delete toRem; // Just to be safe... Normally we shouldn't get to this point
}

void MainWin::OnPlayerEditAliasMainBtn(wxCommandEvent& event) {
	std::string* toMakeMain = (std::string*)event.GetClientData();

	unsigned int id = playerIDByAlias(*toMakeMain);

	if (id != -1) {
		// look for player with the ID
		for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
			if (currPlayer->id == id) {
				// look for the alias to make main
				// save old main alias to not lose it after overwriting with new one
				std::string oldMainAlias = currPlayer->aliases[0];
				for (auto currAlias = currPlayer->aliases.begin(); currAlias != currPlayer->aliases.end(); currAlias++) {
					if (*currAlias == *toMakeMain) {
						*currAlias = oldMainAlias; // put the old main alias where the new one was before
						currPlayer->aliases[0] = *toMakeMain; // now put the new main alias to the main alias spot

						// update match report tab
						matchWindow->updatePlayerDisplayAlias(oldMainAlias, *toMakeMain);
						matchWindow->setMainAliases(retrieveMainAliases()); 
						if (currPlayer->visible) {
							// update rating period tab (if visible)
							periodWindow->updatePlayerDisplayAlias(id, oldMainAlias, currPlayer->aliases[0]);
						}
						// update player edit tab
						playerEditWindow->setMainAliases(retrieveMainAliases(), currPlayer->aliases[0]);
						playerEditWindow->setPlayersAliases(currPlayer->aliases);

						delete toMakeMain;
						return;
					}
				}
			}
		}
	}

	delete toMakeMain;
}

void MainWin::OnPlayerEditToggleVisibility(wxCommandEvent& event) {
	std::string* theData = (std::string*)event.GetClientData();

	unsigned int id = playerIDByAlias(*theData);
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		if (currPlayer->id == id) {
			currPlayer->visible = !(playerEditWindow->getHidden());
			if (currPlayer->visible) {

				// Count wins and losses
				unsigned int wins = 0;
				unsigned int losses = 0;
				unsigned int ties = 0;
				for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {

					std::vector<Glicko2::Result> relevantResults;

					// fill relevantResults vector with Glicko-Results from period
					std::vector<Result> structResults = getResultsInPeriod(currPeriod->first, currPeriod->second, setAbtWindow->getIncludeForfeits());
					for (auto currStructResult = structResults.begin(); currStructResult != structResults.end(); currStructResult++) {
						relevantResults.push_back(currStructResult->result);
					}
					for (auto currResult = relevantResults.begin(); currResult != relevantResults.end(); currResult++) {
						bool isP1 = currResult->getP1Id() == id;
						bool isP2 = currResult->getP1Id() == id;

						if (isP1) {
							if (currResult->getP1Id() == currResult->getWinnerId()) {
								wins++;
							}
							else if (currResult->getP2Id() == currResult->getWinnerId()) {
								losses++;
							}
							else {
								ties++;
							}
						}
						if(isP2) {
							if (currResult->getP2Id() == currResult->getWinnerId()) {
								wins++;
							}
							else if (currResult->getP1Id() == currResult->getWinnerId()) {
								losses++;
							}
							else {
								ties++;
							}
						}
					}
				}
				periodWindow->addPlayer(currPlayer->id, currPlayer->aliases[0]);
				periodWindow->updatePlayer(currPlayer->id, currPlayer->ratings[currPlayer->ratings.size() - 1].rating, currPlayer->wins + wins, currPlayer->losses + losses, currPlayer->ties + ties);
			}
			else {
				periodWindow->removePlayer(currPlayer->id);
			}
			delete theData;
			return;
		}
	}
}

void MainWin::OnPlayerEditPlayerRemBtn(wxCommandEvent& event) {
	std::string* toRem = (std::string*)event.GetClientData(); // player's alias to remove

	unsigned int toRemId = playerIDByAlias(*toRem); // associate alias to remove with ID of the player
	if (toRemId == -1) {
		wxMessageBox(wxString("Player to remove not found. No changes made."), wxString("Unable to lookup player"));
		delete toRem;
		return;
	}
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		if (currPlayer->id == toRemId) {
			wxMessageDialog* removePlayerDialog = new wxMessageDialog(this, wxString(
				"The player and all associated results will be removed. Continue?"),
				wxString("Player about to be deleted"), wxYES_NO | wxCANCEL);
			switch (removePlayerDialog->ShowModal()) {
			case wxID_YES:
				removePlayer(currPlayer->id);
				delete toRem;
				return;
			case wxID_CANCEL:
			case wxID_NO:
				delete toRem;
				return;
			}
		}
	}
}

// Settings/about tab
void MainWin::OnSetAbtIncludeBox(wxCommandEvent& event) {
	recalculateAllPeriods();
}

void MainWin::OnSetAbtTauSpin(wxSpinDoubleEvent& event) {
	Glicko2::setTau(setAbtWindow->getTau());
	// gotta recalculate everything when changing the Tau-constant
	recalculateAllPeriods();
}

void MainWin::OnExit(wxCloseEvent& event)
{	
	saveWorld();
	saveResults();
	saveSettings();
	Glicko2::closeWorld();

	Destroy();
}