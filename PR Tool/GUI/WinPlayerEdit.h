#ifndef __WIN_PLAYER_EDIT_H
#define __WIN_PLAYER_EDIT_H

#include <vector>

//forward declarations
class wxListView;

class WinPlayerEdit : public wxPanel {
public:
	WinPlayerEdit(wxWindow* parent, wxWindowID winid = wxID_ANY, wxArrayString choices = wxArrayString());

	void setMainAliases(std::vector<std::string> newMainAliases, std::string selected = "<New player>");
	void setPlayersAliases(std::vector<std::string> playersAliases);
private:
	// controls used in methods
	wxChoice* aliasChoice;
	wxListView* aliasListView;

	wxArrayString mainAliases;

	// Bind methods
	void OnPlayerChoice(wxCommandEvent& event);
	void OnAliasAddBtn(wxCommandEvent& event);
	void OnAliasMainBtn(wxCommandEvent& event);
	void OnRemAliasBtn(wxCommandEvent& event);
	void OnRemPlayerBtn(wxCommandEvent& event);
};

#endif