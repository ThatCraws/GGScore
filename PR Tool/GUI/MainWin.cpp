#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/notebook.h>
#include <wx/protocol/http.h>
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


// Helper functions (has to be up here to be read before ratingPeriods-set is initialized)
bool comparePeriods(const std::pair<wxDateTime, wxDateTime>& periodOne, const std::pair<wxDateTime, wxDateTime>& periodTwo) {
	return (periodOne.first.IsEarlierThan(periodTwo.first));
}
// to sort results by date
bool compareResultDates(const std::pair<Glicko2::Result, wxDateTime >& resultOne, const std::pair<Glicko2::Result, wxDateTime >& resultTwo) {
	return resultOne.second.IsEarlierThan(resultTwo.second);
}

MainWin::MainWin()
	: wxFrame(NULL, wxID_ANY, "PR Tool")
{
	// initialize set of rating periods and results with compare-function for sorting by date
	//ratingPeriods = std::set<std::pair<wxDateTime, wxDateTime>, bool(*)(const std::pair<wxDateTime, wxDateTime>& periodOne, const std::pair<wxDateTime, wxDateTime>& periodTwo)>(&comparePeriods);

	results = std::multiset<std::pair<Glicko2::Result, wxDateTime>, bool(*)(const std::pair<Glicko2::Result, wxDateTime >&, const std::pair<Glicko2::Result, wxDateTime >&)>(&compareResultDates);

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
		matchWindow->addResult(getMainAlias(currResult->first.getWinner().getID()), getMainAlias(currResult->first.getLoser().getID()), currResult->second);
	}
	matchWindow->sortResultTable();

	recalculateAllPeriods();

	SetSizerAndFit(mainSizer);

	// Binding functions to the events of the main window ("system calls" (close, mini-/maximize)).
	Bind(wxEVT_CLOSE_WINDOW, &MainWin::OnExit, this);
	// Events propagated from Rating Period tab
	Bind(wxEVT_BUTTON, &MainWin::OnRatPerAddBtn, this, ID_RAT_PER_ADD_BTN);
	Bind(wxEVT_BUTTON, &MainWin::OnRatPerRemBtn, this, ID_RAT_PER_REM_BTN);
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

}

void MainWin::recalculateAllPeriods() {
	// first: old ID, second: new ID
	std::vector<std::pair<unsigned int, unsigned int>> oldNewIdMap;

	Glicko2::closeWorld();
	Glicko2::createWorld();

	for (unsigned int i = 0; i < playerBase.size(); i++) {

		// Create new player in Glicko2-world with starting values and update local ID
		std::tuple<double, double, double>& startVals = std::get<2>(playerBase[i])[0];

		// remember old ID to update ID in rankingTable in Period-tab and recreate results
		oldNewIdMap.push_back(std::pair<unsigned int, unsigned int>(std::get<0>(playerBase[i]), -1));

		std::get<0>(playerBase[i]) = Glicko2::createPlayer(std::get<0>(startVals), std::get<1>(startVals), std::get<2>(startVals));
		// update to map to new ID
		oldNewIdMap[i].second = std::get<0>(playerBase[i]);
		periodWindow->updatePlayerID(oldNewIdMap[i].first, std::get<1>(playerBase[i])[0], oldNewIdMap[i].second);

	} // World with all players on their starting values created

		// recreate results with new IDs
	auto newResults = std::multiset<std::pair<Glicko2::Result, wxDateTime>, bool(*)(const std::pair<Glicko2::Result, wxDateTime >&, const std::pair<Glicko2::Result, wxDateTime >&)>(&compareResultDates);
	// Go through results
	for (auto currResult = results.begin(); currResult != results.end(); currResult++) {
		// remember old winner's and loser's ID
		unsigned int oldNewWinId = currResult->first.getWinner().getID();
		unsigned int oldNewLoseId = currResult->first.getLoser().getID();
		// assign new ID
		for (auto currId = oldNewIdMap.begin(); currId != oldNewIdMap.end(); currId++) {
			if (currId->first == oldNewWinId) {
				oldNewWinId = currId->second;
			}
			else if (currId->first == oldNewLoseId) {
				oldNewLoseId = currId->second;
			}
		}

		Glicko2::Player* winner = Glicko2::playerByID(oldNewWinId);
		Glicko2::Player* loser = Glicko2::playerByID(oldNewLoseId);

		Glicko2::Result newResult = Glicko2::Result(*winner, *loser);

		newResults.insert(std::pair<Glicko2::Result, wxDateTime>(newResult, currResult->second));
	}

	results.swap(newResults);

	// Apply algorithm to all results in rating-periods
	for (unsigned int i = 0; i < ratingPeriods.size() ; i++) { // Iterate over rating periods
		std::vector<Glicko2::Result> relevantResults = getResultsInPeriod(ratingPeriods[i].first, ratingPeriods[i].second); // collect all results in current rating period
		Glicko2::glicko(relevantResults); // use Glicko-2 algorithm for the rating period

		// Now update local playerBase by updating the rating values
		Glicko2::Player* realPlayer;
		for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) { // Iterate over playerBase
			realPlayer = Glicko2::playerByID(std::get<0>(*currPlayer)); // get Player-object from Glicko2-world
			//if(nullptr)????
			std::vector<std::tuple<double, double, double>>& ratingVector = std::get<2>(*currPlayer);
			std::get<0>(ratingVector[i + 1]) = realPlayer->getRating();
			std::get<1>(ratingVector[i + 1]) = realPlayer->getRD();
			std::get<2>(ratingVector[i + 1]) = realPlayer->getVolatility();
		}
	}
	// Update rating period-tab rating table
	// Count wins and losses per player
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		unsigned int wins = 0;
		unsigned int losses = 0;
		for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
			std::vector<Glicko2::Result> relevantResults = getResultsInPeriod(currPeriod->first, currPeriod->second);
			for (auto currResult = relevantResults.begin(); currResult != relevantResults.end(); currResult++) {
				if (std::get<0>(*currPlayer) == currResult->getWinner().getID()) {
					wins++;
				}
				else if (std::get<0>(*currPlayer) == currResult->getLoser().getID()) {
					losses++;
				}
			}
		}

		std::vector<std::tuple<double, double, double>>& currRatingVector = std::get<2>(*currPlayer);
		periodWindow->updatePlayer(std::get<0>(*currPlayer), std::get<0>(currRatingVector[currRatingVector.size() - 1]), wins, losses);
	}
	periodWindow->sortMatchTable();
}

void MainWin::recalculateFromPeriod(const std::pair<wxDateTime, wxDateTime>& ratingPeriod) {
	// first: old ID, second: new ID
	std::vector<std::pair<unsigned int, unsigned int>> oldNewIdMap;

	// Recreate world
	Glicko2::closeWorld();
	Glicko2::createWorld();

	// get results in the given rating period
	std::vector<Glicko2::Result> relevantResults;

	// get index from where to update players' ratings
	unsigned int index = 0;
	for (unsigned int i = 0; ratingPeriods[i] != ratingPeriod; i++) { // look for the given ratingPeriod to update values from(including the results of that period)
		index = i;
	} // Index is now the ratingvector-index of the last value NOT to change (but these values are used to calculate the values for the following period)

	// Create the Glicko-players with values from before the given period and update ID in playerBase
	for (unsigned int i = 0; i < playerBase.size(); i++) {
		std::vector<std::tuple<double, double, double>>& currRatingVector = std::get<2>(playerBase[i]);
		// remember old ID to update ID in rankingTable in Period-tab and recreate results
		oldNewIdMap.push_back(std::pair<unsigned int, unsigned int>(std::get<0>(playerBase[i]), -1));
		std::get<0>(playerBase[i]) = Glicko2::createPlayer(std::get<0>(currRatingVector[index]), std::get<1>(currRatingVector[index]), std::get<2>(currRatingVector[index]));
		oldNewIdMap[i].second = std::get<0>(playerBase[i]);
		periodWindow->updatePlayerID(oldNewIdMap[i].first, std::get<1>(playerBase[i])[0], oldNewIdMap[i].second);
	}

	// recreate results with new IDs
	auto newResults = std::multiset<std::pair<Glicko2::Result, wxDateTime>, bool(*)(const std::pair<Glicko2::Result, wxDateTime >&, const std::pair<Glicko2::Result, wxDateTime >&)>(&compareResultDates);
	// Go through results
	for (auto currResult = results.begin(); currResult != results.end(); currResult++) {
		// remember old winner's and loser's ID
		unsigned int oldNewWinId = currResult->first.getWinner().getID();
		unsigned int oldNewLoseId = currResult->first.getLoser().getID();
		// assign new ID
		for (auto currId = oldNewIdMap.begin(); currId != oldNewIdMap.end(); currId++) {
			if (currId->first == oldNewWinId) {
				oldNewWinId = currId->second;
			}
			else if (currId->first == oldNewLoseId) {
				oldNewLoseId = currId->second;
			}
		}

		Glicko2::Player* winner = Glicko2::playerByID(oldNewWinId);
		Glicko2::Player* loser = Glicko2::playerByID(oldNewLoseId);

		Glicko2::Result newResult = Glicko2::Result(*winner, *loser);

		newResults.insert(std::pair<Glicko2::Result, wxDateTime>(newResult, currResult->second));
	}

	results.swap(newResults);

	// Now we wanna access the values after the given period (the fact that a ratingPeriod was passed means, there is at least one. In which case...
	// ...the starting values were read and now index is 0, meaning the values before the first period (increments in following for-loop to...
	// ...get the get the values after the given period, which are the first values to be changed)

	// Iterate over ratingPeriods-set leaving the periods before this one
	for (auto currPeriod = ratingPeriods.begin() + (index++); currPeriod != ratingPeriods.end(); currPeriod++) {
		relevantResults = getResultsInPeriod(currPeriod->first, currPeriod->second);
		// Apply glicko for those results
		if (!relevantResults.empty()) {
			Glicko2::glicko(relevantResults); // TODO recreate results. IDs could be wrong at this point
		}

		// Update playerBase with those results
		Glicko2::Player* currGlkPlayer;
		for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
			std::vector<std::tuple<double, double, double>>& currRatingVector = std::get<2>(*currPlayer);
			currGlkPlayer = Glicko2::playerByID(std::get<0>(*currPlayer));
			std::get<0>(currRatingVector[index]) = currGlkPlayer->getRating();
			std::get<1>(currRatingVector[index]) = currGlkPlayer->getRD();
			std::get<2>(currRatingVector[index]) = currGlkPlayer->getVolatility();
		}
		index++;
	}
	// Update rating period-tab rating table
	// Count wins and losses per player
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		unsigned int wins = 0;
		unsigned int losses = 0;
		for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
			std::vector<Glicko2::Result> relevantResults = getResultsInPeriod(currPeriod->first, currPeriod->second);
			for (auto currResult = relevantResults.begin(); currResult != relevantResults.end(); currResult++) {
				if (std::get<0>(*currPlayer) == currResult->getWinner().getID()) {
					wins++;
				}
				else if (std::get<0>(*currPlayer) == currResult->getLoser().getID()) {
					losses++;
				}
			}
		}
		std::vector<std::tuple<double, double, double>>& currRatingVector = std::get<2>(*currPlayer);
		periodWindow->updatePlayer(std::get<0>(*currPlayer), std::get<0>(currRatingVector[currRatingVector.size() - 1]), wins, losses);
	}
	periodWindow->sortMatchTable();
}

unsigned int MainWin::addNewPlayer(std::vector<std::string> atLeastOneAlias, std::vector<std::tuple<double, double, double>>* optionalRatingVector, bool visibility) {
	if (atLeastOneAlias.empty()) {
		wxMessageBox(wxString("Tried to add player without alias for some reason. \n" "No player added."), wxString("Alias-list to initialize was empty"));
		return -1;
	}

	std::vector<std::tuple<double, double, double>> ratingVector;
	if (optionalRatingVector == nullptr) {

		ratingVector.push_back(std::make_tuple(setAbtWindow->getDefaultRating(), setAbtWindow->getDefaultDeviation(), setAbtWindow->getDefaultVolatility()));
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
	std::tuple<double, double, double> currentRatingValues = (*optionalRatingVector)[optionalRatingVector->size() - 1];
	unsigned int id = Glicko2::createPlayer(std::get<0>(currentRatingValues), std::get<1>(currentRatingValues), std::get<2>(currentRatingValues));

	// uniting the stored ID, aliases and ratings to add them to playerBase
	std::tuple<unsigned int, std::vector<std::string>, std::vector<std::tuple<double, double, double>>, bool> currMapEntry(id, atLeastOneAlias, *optionalRatingVector, visibility);

	playerBase.push_back(currMapEntry);;

	// Update match reports alias-vector
	matchWindow->setMainAliases(retrieveMainAliases());

	// Add player to the rating-periods rating table
	periodWindow->addPlayer(id, atLeastOneAlias[0]);
	periodWindow->updatePlayer(id, std::get<0>((*optionalRatingVector)[optionalRatingVector->size() - 1]), 0, 0);
	periodWindow->sortMatchTable();

	// Add Alias to the alias selection in the player edit window
	playerEditWindow->setMainAliases(retrieveMainAliases());

	return id;
}

void MainWin::removePlayer(unsigned int id) {
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		// find player in playerBase
		if (std::get<0>(*currPlayer) == id) {
			// remove all results including the player
			auto currResult = results.begin();
			for (; currResult != results.end(); ) {
				if (currResult->first.getWinner().getID() == id || currResult->first.getLoser().getID() == id) {
					std::string winnerAlias = getMainAlias(currResult->first.getWinner().getID());
					std::string loserAlias = getMainAlias(currResult->first.getLoser().getID());

					matchWindow->removeResult(winnerAlias, loserAlias, currResult->second);

					currResult = results.erase(currResult);
				}
				else {
					currResult++;
				}
			}
			// remove from rating table window
			periodWindow->removePlayer(id);
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

// Rating period bind-methods
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
		std::vector<std::tuple<double, double, double>>& currRatingVector = std::get<2>(*currPlayer);

		if (ratingPeriods.size() + 1 == currRatingVector.size()) {
			wxMessageBox(wxString("Rating-vector already has the right amount of entries."));
		}
		else {
			// just copy the last right rating values
			std::tuple<double, double, double> ratingBeforePeriod = currRatingVector[index];
			currRatingVector.insert(currRatingVector.begin() + index, ratingBeforePeriod);
		}
	}

	recalculateFromPeriod(*thePeriod);

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
			std::get<2>(*currPlayer).pop_back();
		}

		if (prevPeriod != nullptr) { // if this is nullptr the first period got removed
			recalculateFromPeriod(*prevPeriod);
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

// Match report bind-methods
void MainWin::OnMatRepAddBtn(wxCommandEvent& event) {
	// resultTuple: 0: winner, 1: loser, 2: date
	std::tuple<std::string, std::string, wxDateTime>* resultTuple = (std::tuple<std::string, std::string, wxDateTime>*)event.GetClientData();
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

	// Retrieve pointer to the player-object for use with result
	Glicko2::Player* winPlayer = Glicko2::playerByID(winID);
	Glicko2::Player* losePlayer = Glicko2::playerByID(loseID);

	Glicko2::Result resultToAdd = Glicko2::Result(*winPlayer, *losePlayer);

	results.insert(std::pair<Glicko2::Result, wxDateTime>(resultToAdd, std::get<2>(*resultTuple)));
	matchWindow->addResult(getMainAlias(winID), getMainAlias(loseID), std::get<2>(*resultTuple));
	matchWindow->sortResultTable();

	for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
		if ((std::get<2>(*resultTuple).IsLaterThan(currPeriod->first) || std::get<2>(*resultTuple).IsEqualTo(currPeriod->first)) 
			&& (std::get<2>(*resultTuple).IsEarlierThan(currPeriod->second) || std::get<2>(*resultTuple).IsEqualTo(currPeriod->second))) {
			recalculateFromPeriod(*currPeriod);
		}
	}

	delete resultTuple;
}

void MainWin::OnMatRepRemBtn(wxCommandEvent& event) {
	// event data is a tuple consisting of <winnerAlias, loserAlias, matchDate>
	std::tuple <std::string, std::string, wxDateTime>* data = (std::tuple <std::string, std::string, wxDateTime>*)event.GetClientData();

	// Go through results until result to be removed is found
	for(auto currResult = results.begin(); currResult != results.end(); currResult++) {
		// compare Dates first
		if (currResult->second.IsSameDate(std::get<2>(*data))) {
			// compare winner alias
			if (getMainAlias(currResult->first.getWinner().getID()) == std::get<0>(*data)) {
				// compare loser alias
				if (getMainAlias(currResult->first.getLoser().getID()) == std::get<1>(*data)) {
					results.erase(currResult);
					matchWindow->removeResult();

					// If match was inside of an existing rating period recalculate from that period on
					for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
						if ((std::get<2>(*data).IsLaterThan(currPeriod->first) || std::get<2>(*data).IsEqualTo(currPeriod->first))
							&& (std::get<2>(*data).IsEarlierThan(currPeriod->second) || std::get<2>(*data).IsEqualTo(currPeriod->first))) {
							recalculateFromPeriod(*currPeriod);
						}
					}

					delete data;
					return;
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

			// to store the players and create the result in the end
			Glicko2::Player* winner;
			Glicko2::Player* loser;
			// to store date and create result in the end
			wxDateTime dateOfMatch;

			// if the set is not forfeited include it (if there's a negative value in the results like -1-0, there'll always be more than one '-')
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

						// get date
						wxDateTime dateOfMatch;
						dateOfMatch.ParseFormat(wxString(currMatch["completed_at"].asString().substr(0, 10)), wxString("%Y-%m-%d"));

						// get players
						Glicko2::Player* winner;
						Glicko2::Player* loser;

						// get scores
						std::string p1Score = scoreString.substr(0, scoreString.find_first_of("-"));
						std::string p2Score = scoreString.substr(scoreString.find_first_of("-") + 1);

						// set winner and loser by scores
						if (std::stoi(p1Score) > std::stoi(p2Score)) {
							winner = Glicko2::playerByID(p1Id);
							loser = Glicko2::playerByID(p2Id);
							// add results to result-collection-thingy(set, a set of sets, ha!)
							Glicko2::Result toAdd = Glicko2::Result(*winner, *loser);
							results.insert(std::pair<Glicko2::Result, wxDateTime>(toAdd, dateOfMatch));
							matchWindow->addResult(getMainAlias(p1Id), getMainAlias(p2Id), dateOfMatch);
						}
						else {
							winner = Glicko2::playerByID(p2Id);
							loser = Glicko2::playerByID(p1Id);
							// add results to result-collection-thingy(set, a set of sets, ha!)
							Glicko2::Result toAdd = Glicko2::Result(*winner, *loser);
							results.insert(std::pair<Glicko2::Result, wxDateTime>(toAdd, dateOfMatch));
							matchWindow->addResult(getMainAlias(p2Id), getMainAlias(p1Id), dateOfMatch);
						}
					}
				}
			}
			// if match is forfeited and we wanna include forfeits
			else if (setAbtWindow->getIncludeForfeits()) {
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

						dateOfMatch.ParseFormat(wxString(currMatch["completed_at"].asString().substr(0, 10)), wxString("%Y-%m-%d"));

						// set winner and loser by scores
						winner = Glicko2::playerByID(winId);
						loser = Glicko2::playerByID(loseId);

						// add results to result-collection-thingy(set, a set of sets, ha!)
						Glicko2::Result toAdd = Glicko2::Result(*winner, *loser);
						results.insert(std::pair<Glicko2::Result, wxDateTime>(toAdd, dateOfMatch));
						matchWindow->addResult(getMainAlias(winId), getMainAlias(loseId), dateOfMatch);
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

// Player edit bind-methods
void MainWin::OnPlayerEditPlayerChoice(wxCommandEvent& event) {
	std::string* theAlias = (std::string*)event.GetClientData();
	unsigned int id = playerIDByAlias(*theAlias);
	std::vector<std::string> mainAliases = getPlayersAliases(id);
	playerEditWindow->setPlayersAliases(mainAliases);

	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		if (std::get<0>(*currPlayer) == id) {
			// the default is that a player is visible and the checkbox is unchecked (which means true for the box means a hidden player...
			// ...but true for a player means shown
			playerEditWindow->setHidden(!std::get<3>(*currPlayer));
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
		if (std::get<0>(*currPlayer) == toAddAliasId) {
			std::get<1>(*currPlayer).push_back(theAliases->second);
			playerEditWindow->setPlayersAliases(std::get<1>(*currPlayer));
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
		if (std::get<0>(*currPlayer) == toRemId) {
			std::vector<std::string>& aliasVector = std::get<1>(*currPlayer);
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
							removePlayer(std::get<0>(*currPlayer));
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
			if (std::get<0>(*currPlayer) == id) {
				// look for the alias to make main
				std::vector<std::string>& currPlayerAliases = std::get<1>(*currPlayer);
				// save old main alias to not lose it after overwriting with new one
				std::string oldMainAlias = currPlayerAliases[0];
				for (auto currAlias = currPlayerAliases.begin(); currAlias != currPlayerAliases.end(); currAlias++) {
					if (*currAlias == *toMakeMain) {
						*currAlias = oldMainAlias; // put the old main alias where the new one was before
						currPlayerAliases[0] = *toMakeMain; // now put the new main alias to the main alias spot

						// update match report tab
						matchWindow->updatePlayerDisplayAlias(oldMainAlias, *toMakeMain);
						matchWindow->setMainAliases(retrieveMainAliases()); 
						// update rating period tab
						periodWindow->updatePlayerDisplayAlias(id, oldMainAlias, std::get<1>(*currPlayer)[0]);
						// update player edit tab
						playerEditWindow->setMainAliases(retrieveMainAliases(), currPlayerAliases[0]);
						playerEditWindow->setPlayersAliases(currPlayerAliases);

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
		if (std::get<0>(*currPlayer) == id) {
			std::get<3>(*currPlayer) = !(playerEditWindow->getHidden());
			if (std::get<3>(*currPlayer)) {

				// Count wins and losses
				unsigned int wins = 0;
				unsigned int losses = 0;
				for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
					std::vector<Glicko2::Result> relevantResults = getResultsInPeriod(currPeriod->first, currPeriod->second);
					for (auto currResult = relevantResults.begin(); currResult != relevantResults.end(); currResult++) {
						if (std::get<0>(*currPlayer) == currResult->getWinner().getID()) {
							wins++;
						}
						else if (std::get<0>(*currPlayer) == currResult->getLoser().getID()) {
							losses++;
						}
					}
				}
				periodWindow->addPlayer(std::get<0>(*currPlayer), std::get<1>(*currPlayer)[0]);
				periodWindow->updatePlayer(std::get<0>(*currPlayer), std::get<0>(std::get<2>(*currPlayer)[std::get<2>(*currPlayer).size() - 1]), wins, losses);
			}
			else {
				periodWindow->removePlayer(std::get<0>(*currPlayer));
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
		if (std::get<0>(*currPlayer) == toRemId) {
			removePlayer(std::get<0>(*currPlayer));
			delete toRem;
			return;
		}
	}
}

unsigned int MainWin::playerIDByAlias(std::string alias) {
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		for (auto currAlias = std::get<1>(*currPlayer).begin(); currAlias != std::get<1>(*currPlayer).end(); currAlias++) {
			if (alias == *currAlias) {
				return std::get<0>(*currPlayer);
			}
		}
	}
	return -1;
}

std::vector<std::string> MainWin::getPlayersAliases(unsigned int id) {
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		if (std::get<0>(*currPlayer) == id) {
			return std::get<1>(*currPlayer);
		}
	}
	return std::vector<std::string>();
}

std::string MainWin::getMainAlias(unsigned int id) {
	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		if (std::get<0>(*currPlayer) == id) {
			return std::get<1>(*currPlayer)[0];
		}
	}
	return "";
}

std::vector<std::string> MainWin::retrieveMainAliases() {
	std::vector<std::string> toRet;

	for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
		toRet.push_back(std::get<1>(*currPlayer)[0]);
	}

	return toRet;
}

unsigned int MainWin::assignNewAlias(std::string aliasToAssign) {
	// Convert existing aliases to wxArrayString to show in choice-CTRL
	wxArrayString existingMainAliases = wxArrayString();

	std::vector<std::string> toMakeStringArray = retrieveMainAliases();

	for (auto currAlias = toMakeStringArray.begin(); currAlias != toMakeStringArray.end(); currAlias++) {
		existingMainAliases.push_back(*currAlias);
	}

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
			
			std::vector<std::tuple<double, double, double>> startValues;
			startValues.push_back(std::tuple<double, double, double>(assignDialog->getRating(), assignDialog->getDeviation(), assignDialog->getVolatility()));

			return addNewPlayer(newPlayerAliases, &startValues);
		}
		// if the alias is to be added to another player (by here it should already have been checked if it's a duplicate (non-main alias))
		else {
			// Assign alias to the chosen player
			for (auto currPlayer = playerBase.begin(); currPlayer != playerBase.end(); currPlayer++) {
				for (auto currAlias = (std::get<1>(*currPlayer)).begin(); currAlias != (std::get<1>(*currPlayer)).end(); currAlias++) {
					if (*currAlias == assignDialog->getAliasToAssignTo()) {
						(std::get<1>(*currPlayer)).push_back(aliasToAssign);

						assignDialog->Destroy();
						return std::get<0>(*currPlayer);
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

bool MainWin::loadWorld() {
	std::ifstream ifs("players.json");

	if (ifs.fail()) { // If the file does not exist yet(first time running app), it will be created on exiting the app, when saving the settings.
		//wxMessageBox(wxString("Can not open or read the file players.json. Playerbase will not be loaded.\n" "File will be created upon exiting.\n"), wxString("File could not be opened")); // 
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
			//wxMessageBox(wxString("    alias: " + players[i]["aliases"][j].asString() + " pushed into alias-vector"));
		}

		std::vector<std::tuple<double, double, double>> ratingVector; // vector containing tuple (rating, deviation, volatility)... 
																	  //for each rating period (start values -> current)

		const Json::Value& ratingVals = players[i]["rating values"]; // array of current players' rating values (or rather rating values-objects)

		// storing the players' rating values per rating period
		for (unsigned int j = 0; j < ratingVals.size(); j++) {
			ratingVector.push_back(std::make_tuple(ratingVals[j]["rating"].asDouble(), ratingVals[j]["deviation"].asDouble(), ratingVals[j]["volatility"].asDouble()));
		}

		if (players[i].isMember("visible")) {
			addNewPlayer(aliases, &ratingVector, players[i]["visible"].asBool());
		}
		else {
			addNewPlayer(aliases, &ratingVector);
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

		for (std::vector<std::string>::iterator currAlias = std::get<1>(*currPlayerBasePlayer).begin(); currAlias != std::get<1>(*currPlayerBasePlayer).end(); currAlias++) { // iterate through aliases of current player
			currAliases.append(Json::Value(*currAlias)); // filling the aliases section with the aliases of the current player
		}

		Json::Value currPlayerRatings(Json::arrayValue); // equal to the "rating values" in players.json (to save all rating values for every period)

		for (auto currRatingTuple = std::get<2>(*currPlayerBasePlayer).begin(); currRatingTuple != std::get<2>(*currPlayerBasePlayer).end(); currRatingTuple++) {
			//wxMessageBox(wxString("Adding ratings to player with first alias " + std::get<1>(*currPlayerBasePlayer)[0]));

			Json::Value ratingValsToAdd; // Contains the rating values of the currently saved period
			ratingValsToAdd["rating"] = std::get<0>(*currRatingTuple);
			ratingValsToAdd["deviation"] = std::get<1>(*currRatingTuple);
			ratingValsToAdd["volatility"] = std::get<2>(*currRatingTuple);
			currPlayerRatings.append(ratingValsToAdd);
		}

		currEntry["rating values"] = currPlayerRatings;
		currEntry["aliases"] = currAliases;
		currEntry["visible"] = std::get<3>(*currPlayerBasePlayer);

		allPlayers.append(currEntry);

	}
	playersJson["players"] = allPlayers;

	Json::Value ratingPeriodDates(Json::arrayValue);

	Json::Value currPeriodToAdd;

	for (auto currPeriod = ratingPeriods.begin(); currPeriod != ratingPeriods.end(); currPeriod++) {
		//wxMessageBox(wxString("Adding rating period starting on: " + (*currPeriod).first.FormatISODate().ToStdString()));

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
		unsigned int winnerID = playerIDByAlias((*currResult)["winner"].asString());
		unsigned int loserID = playerIDByAlias((*currResult)["loser"].asString());

		if (winnerID == -1) {
			wxMessageBox(wxString("Unable to assign player with the following alias to a player from the playerbase: " +
				(*currResult)["winner"].asString() + ". \n" "Please assign manually or result will be disregarded"),
				wxString("Unexpected alias while reading results.json"));
			winnerID = assignNewAlias((*currResult)["winner"].asString());
		}

		// We do not wanna show the assignPlayerDialog again, if it was shown for the winner and the user clicked cancel (winnerID still -1)
		if (loserID == -1 && winnerID != -1) {
			wxMessageBox(wxString("Unable to assign player with the following alias to a player from the playerbase: " +
				(*currResult)["loser"].asString() + ". \n" "Please assign manually or result will be disregarded"),
				wxString("Unexpected alias while reading results.json"));
			loserID = assignNewAlias((*currResult)["loser"].asString());
		}

		// if one of the IDs is still -1 here or both players got assigned to the same name, skip adding the result
		if (!(winnerID == -1 || loserID == -1 || winnerID == loserID)) {
			Glicko2::Player* winningPlayer = Glicko2::playerByID(winnerID);
			Glicko2::Player* losingPlayer = Glicko2::playerByID(loserID);

			Glicko2::Result toAdd = Glicko2::Result(*winningPlayer, *losingPlayer);
			wxDateTime resultsDate;
			if (!resultsDate.ParseISODate((*currResult)["date"].asString())) {
				wxMessageBox(wxString("Unable to parse following date: " + (*currResult)["date"].asString() + "\n" "Result will be disregarded"),
					wxString("Unexpected date string while reading results.json"));
			}
			else {
				//wxMessageBox(wxString("Result added: \n" "Winner: " + getMainAlias(winnerID) +
				//	"\n" "Loser: " + getMainAlias(loserID) + "\n" "Date: " + resultsDate.Format(defaultFormatString)));
				results.insert(std::pair<Glicko2::Result, wxDateTime>(toAdd, resultsDate));
			}
		}
		else if(winnerID == loserID && winnerID != -1) {
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
		resultToAdd["winner"] = getMainAlias(currResult->first.getWinner().getID());
		resultToAdd["loser"] = getMainAlias(currResult->first.getLoser().getID());
		resultToAdd["date"] = currResult->second.FormatISODate().ToStdString();

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

std::vector<Glicko2::Result> MainWin::getResultsInPeriod(const wxDateTime& start, const wxDateTime& end) {
	std::vector<Glicko2::Result> toRet;

	for (auto currRes = results.begin(); currRes!= results.end(); currRes++) {
		if ((currRes->second.IsLaterThan(start) || currRes->second.IsEqualTo(start)) &&
			(currRes->second.IsEarlierThan(end) || currRes->second.IsEqualTo(end))) {
			toRet.push_back(currRes->first);
		}
	}

	return toRet;
}

void MainWin::OnExit(wxCloseEvent& event)
{	
	saveWorld();
	saveResults();
	saveSettings();
	Glicko2::closeWorld();

	Destroy();
}