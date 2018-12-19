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
	void setHidden(bool isHidden);
	bool getHidden() const;
private:
	// controls used in methods
	wxChoice* aliasChoice;
	wxListView* aliasListView;
	wxCheckBox* hidePlayerCheck;

	wxArrayString mainAliases;

	// Bind methods
	void OnPlayerChoice(wxCommandEvent& event);
	void OnAliasAddBtn(wxCommandEvent& event);
	void OnAliasRemBtn(wxCommandEvent& event);
	void OnAliasMainBtn(wxCommandEvent& event);
	void OnPlayerToggleVis(wxCommandEvent& event);
	void OnPlayerRemBtn(wxCommandEvent& event);
};

#endif