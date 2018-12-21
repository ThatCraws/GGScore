#ifndef __WINRATPER_H
#define __WINRATPER_H

#include <wx/listctrl.h> // because column wxListEvent

class WinRatPer : public wxPanel
{
public:
	WinRatPer(wxWindow* parent, wxWindowID winid = wxID_ANY);

	void addRatingPeriod(const wxDateTime& start, const wxDateTime& end);
	void removeRatingPeriod(wxDateTime& start, wxDateTime& end);

	void addPlayer(unsigned int id, std::string displayAlias); // rating, sets, wins, losses?? ID to set ItemData(won't be duplicate) not to mix up with setID tho
	void updatePlayer(unsigned int id, double rating, unsigned int wins, unsigned int losses);
	void updatePlayerDisplayAlias(unsigned int id, std::string oldAlias, std::string newAlias);
	// This method is used to assign new IDs, when the world is recreated. In this process there might temporarily be duplicate IDs, so we use the shown alias to distinguish
	void updatePlayerID(unsigned int oldId, std::string displayAlias, unsigned int newId); 
	void removePlayer(unsigned int id);

	void sortMatchTable();
private:
	// Controls used in methods outside the constructor and variables to handle them.
	wxListView* ratingTable;
	unsigned int ratingViewItemID;
	int ratingViewSortedColumn; // column to sort the rating view by
	bool ratingViewDescending;

	wxListView* periodTable;
	unsigned int periodViewItemID;

	// Bind-methods
	void OnBtnAddPer(wxCommandEvent& event);
	void OnBtnRemPer(wxCommandEvent& event);
	void OnBtnFinalize(wxCommandEvent& event);
	void OnColumnClick(wxListEvent& event);
};

#endif