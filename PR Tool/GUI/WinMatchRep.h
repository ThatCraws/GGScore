#ifndef __WINMATCHREP_H
#define __WINMATCHREP_H

#include<vector>

#include <wx/listctrl.h> // for wxListEvent

class WinMatchRep : public wxPanel
{
public:
	WinMatchRep(wxWindow* parent, wxWindowID id = wxID_ANY);

	void addResult(std::string winnerAlias, std::string loserAlias, const wxDateTime date);
	void removeResult();
	void removeResult(std::string winnerAlias, std::string loserAlias, const wxDateTime& date);
	void clearResultTable();

	void setMainAliases(std::vector<std::string> newList);
	void updatePlayerDisplayAlias(std::string oldAlias, std::string newAlias);

	void sortResultTable();
private:
	wxArrayString mainAliases;

	// Controls used in methods outside the constructor and variables to handle them.
	wxListView* matchTable;
	unsigned int listViewItemID;
	int resultViewSortedColumn; // column to sort the result view by
	bool resultViewDescending;

	void OnBtnAdd(wxCommandEvent& event);
	void OnBtnRem(wxCommandEvent& event);
	void OnColumnClick(wxListEvent& event);
};
#endif
