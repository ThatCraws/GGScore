#ifndef __MAINWIN_H
#define __MAINWIN_H

#include<vector>
#include<set>
#include"glkResult.h"

// forward declarations
class Glicko2::Player;

class WinRatPer;
class WinMatchRep;
class WinPlayerEdit;
class WinSetAbt;

class MainWin : public wxFrame
{
public:
	MainWin();
	//std::vector<std::string>& getAliasesByID(unsigned int id);

private:
	void recalculateAllPeriods();

	/* ------------ recalculateFromPeriod ------------
	Updates the players' rating values with the results from a given rating period.
	This method recreates the Glicko-2-World and may change IDs, but updates them in the playerBase and rating periods tab as well.

	parameters:
		ratingPeriod	The rating period from where to start recalculating (including the period itself)
	*/
	void recalculateFromPeriod(const std::pair<wxDateTime, wxDateTime>& ratingPeriod);
	
	unsigned int addNewPlayer(std::vector<std::string> atLeastOneAlias, std::vector<std::tuple<double, double, double>>* optionalRatingVector = nullptr); 
	void removePlayer(unsigned int id);

	/* ------------ loadWorld ------------
	Fills the playerBase with players loaded from players.json-file.
	The playerBase-entries contain the players' aliases and rating values for all rating periods.
	Fills the ratingPeriods-vector with ratingPeriods loaded from players.json.

	Return:
		true, if the input-stream has set the goodbit after all reading-operations are completed. 
		false, if the input-stream cannot open/read the file or stream state flag not goodbit.
	*/
	bool loadWorld();
	/* ------------ saveWorld ------------
	Creates the .json-files to recreate the current playerBase and ratingPeriods. Will rewrite the whole file (not append).
	
	Return:
		true, if the method successfully reaches its end.
		false, if the output-stream cannot open the file.
	*/
	bool saveWorld();

	/* ------------ loadResults ------------
	Fills the results-vector with results loaded from results.json-file.
	*/
	bool loadResults();

	/* ------------ saveResult ------------
	Creates the .json-files to recreate the currently entered results. Will rewrite the whole file (not append).

	Return:
		true, if the method successfully reaches its end.
		false, if the output-stream cannot open the file.
	*/
	bool saveResults();

	/* ------------ loadSettings ------------
	Loads and sets the default values from settings.json-file.
	*/
	bool loadSettings();

	/* ------------ saveSettings ------------
	Creates the .json-files to load the settings/default values from. Will rewrite the whole file (not append).

	Return:
		true, if the method successfully reaches its end.
		false, if the output-stream cannot open the file.
	*/
	bool saveSettings();

	// Handling adding/removing from playerbase/periods/results

	/* ------------ findPeriod ------------
	Looks for a given period of time in the rating periods-vector and returns it

	Parameter:
		start	start date of the period
		end		end date of the period

	Return:
		The found date as pointer or Nullptr if not found
	*/
	const std::pair<wxDateTime, wxDateTime>* findPeriod(wxDateTime& start, wxDateTime& end);

	std::vector<Glicko2::Result> getResultsInPeriod(const wxDateTime& start, const wxDateTime& end);

	/* ------------ playerIDByAlias ------------
	Looks for a given player's alias and returns its ID or -1 if it cant' be found.

	Parameter:
		alias	The alias of the player to search for

	Return:
		the found player's ID or -1 if it can't be found
	*/
	unsigned int playerIDByAlias(std::string alias);

	std::vector<std::string> getPlayersAliases(unsigned int id);

	std::string getMainAlias(unsigned int id);
	std::vector<std::string> retrieveMainAliases();

	/* ------------ assignNewAlias ------------
	Lets the user assign a new alias to a player or create a new one via dialog.

	Parameter:
		alias	The new alias, not belonging to any player yet (make sure with playerIDByAlias. This method does not check)

	Return:
		the ID of the player the alias was assigned to. Can be a new ID, if a new player was created.
		-1, if the user presses Cancel or closes the dialog any other way than clicking "OK"
	*/
	unsigned int assignNewAlias(std::string alias);

	// --- Bind-methods/event-handling ---
	// Rating period tab
	void OnRatPerAddBtn(wxCommandEvent& event);
	void OnRatPerRemBtn(wxCommandEvent& event);
	// Match report tab
	void OnMatRepAddBtn(wxCommandEvent& event);
	void OnMatRepRemBtn(wxCommandEvent& event);
	void OnMatRepImportBtn(wxCommandEvent& event);
	// Player edit tab
	void OnPlayerEditPlayerChoice(wxCommandEvent& event);
	void OnPlayerEditAliasAddBtn(wxCommandEvent& event);
	void OnPlayerEditAliasRemBtn(wxCommandEvent& event);
	void OnPlayerEditAliasMainBtn(wxCommandEvent& event);
	void OnPlayerEditPlayerRemBtn(wxCommandEvent& event);

	void OnExit(wxCloseEvent& event);

	// Windows for Notebook-Control/Tab-View
	WinRatPer* periodWindow;
	WinMatchRep* matchWindow;
	WinPlayerEdit* playerEditWindow;
	WinSetAbt* setAbtWindow;

	/* ------------ playerBase ------------
	"Maps" player IDs to their known aliases and rating values per period.

	playerBase[Player] -> player-tuple
						0: ID (int)
						1: aliases (vector of strings)
						2: ratingvector[period] -> rating-tuple
							0: rating
							1: deviation
							3: volatility
	*/
	std::vector<std::tuple<unsigned int, std::vector<std::string>, std::vector<std::tuple<double, double, double>>>> playerBase;
	/* ------------ ratingPeriods ------------
	Stores rating periods consisting of start and end date ordered from oldest(first) to newest(last).

	ratingPeriods-element -> period-pair
						first: start date (wxDateTime)
						second: end date (wxDateTime)
	*/
	//std::set<std::pair<wxDateTime, wxDateTime>, bool(*)(const std::pair<wxDateTime, wxDateTime>&, const std::pair<wxDateTime, wxDateTime>&)> ratingPeriods;
	std::vector<std::pair<wxDateTime, wxDateTime>> ratingPeriods;
	/* ------------ results ------------
	Stores the results consisting of the Glicko-result object itself and the date the match happened ordered by the date from oldest(first) to newest(last).

	results-element -> result-pair
						first: Result (Glicko2::Result)
						second: Date (wxDateTime)
	*/
	std::multiset<std::pair<Glicko2::Result, wxDateTime>, bool(*)(const std::pair<Glicko2::Result, wxDateTime >&, const std::pair<Glicko2::Result, wxDateTime >&)> results;
	unsigned int currResultID;

};

#endif
