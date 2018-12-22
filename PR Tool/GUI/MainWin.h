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

private:
	// Structs first, so they can be used as parameters/types

		/* ------------ struct Rating ------------
	Represents a rating holding the actual rating value, volatility and deviation.

	fields:
		rating (double)
		deviation (double)
		volatility (double)
*/
	struct Rating;

	/* ------------ struct Player ------------
	Represents a player and holds their IDs, known aliases and rating values per period.

	fields:
		id (unsigned int)
		aliases (vector<string>)
		ratings (vector<struct Rating>)
		visible (bool) (in the ranking table. true: shown, false: hidden)
	*/
	struct Player;


	// TODO save the wins and losses from previous rating periods (before finalizing) per player to set the wins/losses/win%-fields in periodWindow.
	void finalize();

	// actually calculating the ratings

	/* ------------ recalculateAllPeriods ------------
	Completely recalculates all players' ratings. Taking into account all results within a rating period.
	Starting with the starting values of all players (first element in rating-vector) the algorithm is applied for each period (/its results)
	This method recreates the Glicko-2-World and may change IDs, but updates them in the playerBase and rating periods tab as well.
	*/
	void recalculateAllPeriods();

	/* ------------ recalculateFromPeriod ------------
	Recalculates all players' rating values from a given rating period on. 
	Starting with the rating values from before the given period and using the results from the period to get the new ratings.
	Then the next period (if any) will be calculated and so on, untill the ratings are up-to-date (again).
	This method recreates the Glicko-2-World and may change IDs, but updates them in the playerBase and rating periods tab as well.

	parameters:
		ratingPeriod	The rating period from where to start recalculating(including the period itself)
	*/
	void recalculateFromPeriod(const std::pair<wxDateTime, wxDateTime>& ratingPeriod);

	// Handling adding/removing from playerbase/periods/results

	unsigned int addNewPlayer(std::vector<std::string> atLeastOneAlias, std::vector<Rating>* optionalRatingVector = nullptr, unsigned int wins = 0, unsigned int losses = 0, bool visibility = true);
	void removePlayer(unsigned int id);

	/* ------------ findPeriod ------------
Looks for a given period of time in the rating periods-vector and returns it

Parameter:
	start	start date of the period
	end		end date of the period

Return:
	The found date as pointer or Nullptr if not found
*/
	const std::pair<wxDateTime, wxDateTime>* findPeriod(wxDateTime& start, wxDateTime& end);
	
	/* ------------ getResultsInPeriod ------------
Returns all Results that happened within the given time frame(after or on the "start" and before or on "end").

Parameter:
	start	start date of the period
	end		end date of the period

Return:
	A vector of Glicko-2 Results that where entered to occur (or rather added to the results-vector with a date) within the given time frame.
*/
	std::vector<Glicko2::Result> getResultsInPeriod(const wxDateTime& start, const wxDateTime& end);


	/* ------------ playerIDByAlias ------------
Looks for a given player's alias and returns its ID or -1 if it cant' be found.

Parameter:
	alias	The alias of the player to search for

Return:
	the found player's ID or -1 if it can't be found
*/
	unsigned int playerIDByAlias(std::string alias);

	/* ------------ getPlayersAliases ------------
Returns all the known aliases of the player with the given ID.

Parameter:
	id	The ID of the player whose aliases to return.

Return:
	A vector of strings representing the players' aliases or an empty vector, if the ID could not be found.
*/
	std::vector<std::string> getPlayersAliases(unsigned int id);

	/* ------------ getMainAlias ------------
Returns the display alias of the player with the given ID.

Parameter:
	id	The ID of the player whose aliases to return.

Return:
	A string representing the players' main (display) alias or an empty string, if the ID could not be found.
*/
	std::string getMainAlias(unsigned int id);

	/* ------------ retrieveMainAliases ------------
Returns all the known display aliases of the players in the playerBase.
Mainly used to update the choice of aliases in the "Player Edit"-tab.

Return:
	A vector of strings representing the players' display aliases or an empty vector, if no players exist.
*/
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

	// .Json-file handling/saving and loading the "world"
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

	// --- Bind-methods/event-handling ---
	// Rating period tab
	void OnRatPerAddBtn(wxCommandEvent& event);
	void OnRatPerRemBtn(wxCommandEvent& event);
	void OnRatPerFinBtn(wxCommandEvent& event);
	// Match report tab
	void OnMatRepAddBtn(wxCommandEvent& event);
	void OnMatRepRemBtn(wxCommandEvent& event);
	void OnMatRepImportBtn(wxCommandEvent& event);
	// Player edit tab
	void OnPlayerEditPlayerChoice(wxCommandEvent& event);
	void OnPlayerEditAliasAddBtn(wxCommandEvent& event);
	void OnPlayerEditAliasRemBtn(wxCommandEvent& event);
	void OnPlayerEditAliasMainBtn(wxCommandEvent& event);
	void OnPlayerEditToggleVisibility(wxCommandEvent& event);
	void OnPlayerEditPlayerRemBtn(wxCommandEvent& event);

	void OnExit(wxCloseEvent& event);

	// Windows for Notebook-Control/Tab-View
	WinRatPer* periodWindow;
	WinMatchRep* matchWindow;
	WinPlayerEdit* playerEditWindow;
	WinSetAbt* setAbtWindow;


	// Containers to represent the Glicko-2 World

	std::vector<Player> playerBase;

	/* ------------ ratingPeriods ------------
	Stores rating periods consisting of start and end date ordered from oldest(first) to newest(last).

	ratingPeriods-element -> period-pair
						first: start date (wxDateTime)
						second: end date (wxDateTime)
	*/
	std::vector<std::pair<wxDateTime, wxDateTime>> ratingPeriods;
	/* ------------ results ------------
	Stores the results consisting of the Glicko-result object itself and the date the match happened ordered by the date from oldest(first) to newest(last).

	results-element -> result-pair
						first: Result (Glicko2::Result)
						second: Date (wxDateTime)
	*/
	std::multiset<std::pair<Glicko2::Result, wxDateTime>, bool(*)(const std::pair<Glicko2::Result, wxDateTime >&, const std::pair<Glicko2::Result, wxDateTime >&)> results;

};

#endif
