#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "WinPlayerEdit.h"

#include "GlobalVars.h"

#include "wx/listctrl.h"
#include <cctype>

WinPlayerEdit::WinPlayerEdit(wxWindow* parent, wxWindowID winid, wxArrayString choices)
	:wxPanel(parent, winid), mainAliases(choices) {

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Alias management
	// modification buttons
	wxBoxSizer* aliasManSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* aliasModSizer = new wxBoxSizer(wxVERTICAL);
	
	mainAliases.Add(wxString("<New player>"), 0);
	aliasChoice = new wxChoice(this, ID_PLA_EDIT_PLA_CHOICE, wxDefaultPosition, wxDefaultSize, mainAliases); 
	aliasChoice->SetSelection(0);

	wxButton* addAliasBtn = new wxButton(this, ID_PLA_EDIT_ADD_ALIAS_BTN, wxString("Add alias"));
	wxButton* remAliasBtn = new wxButton(this, ID_PLA_EDIT_REM_ALIAS_BTN, wxString("Remove alias"));
	wxButton* mainAliasBtn = new wxButton(this, ID_PLA_EDIT_MAIN_ALIAS_BTN, wxString("Make main alias"));
	hidePlayerCheck = new wxCheckBox(this, ID_PLA_EDIT_HIDE_PLA_BTN, wxString("Hide in ranking"));
	hidePlayerCheck->Disable();

	aliasModSizer->Add(aliasChoice, 0, wxEXPAND);
	aliasModSizer->AddSpacer(10);
	aliasModSizer->Add(addAliasBtn, 0, wxEXPAND);
	aliasModSizer->Add(remAliasBtn, 0, wxEXPAND);
	aliasModSizer->Add(mainAliasBtn, 0, wxEXPAND);
	aliasModSizer->Add(hidePlayerCheck, 0, wxEXPAND); // Not exactly alias modifying, but better here
	// list of aliases of selected player
	aliasListView = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL);
	aliasListView->InsertColumn(0, wxEmptyString);

	aliasManSizer->Add(aliasModSizer);
	aliasManSizer->Add(aliasListView, 1, wxEXPAND);

	// Player management
	wxButton* remBtn = new wxButton(this, ID_PLA_EDIT_REM_BTN, wxString("Remove player"));

	Bind(wxEVT_CHOICE, &WinPlayerEdit::OnPlayerChoice, this, ID_PLA_EDIT_PLA_CHOICE);
	Bind(wxEVT_BUTTON, &WinPlayerEdit::OnAliasAddBtn, this, ID_PLA_EDIT_ADD_ALIAS_BTN);
	Bind(wxEVT_BUTTON, &WinPlayerEdit::OnAliasMainBtn, this, ID_PLA_EDIT_MAIN_ALIAS_BTN);
	Bind(wxEVT_BUTTON, &WinPlayerEdit::OnAliasRemBtn, this, ID_PLA_EDIT_REM_ALIAS_BTN);
	Bind(wxEVT_CHECKBOX, &WinPlayerEdit::OnPlayerToggleVis, this, ID_PLA_EDIT_HIDE_PLA_BTN);
	Bind(wxEVT_BUTTON, &WinPlayerEdit::OnPlayerRemBtn, this, ID_PLA_EDIT_REM_BTN);

	mainSizer->Add(aliasManSizer, 1, wxEXPAND);
	mainSizer->Add(remBtn, 0, wxALIGN_CENTER);

	SetSizerAndFit(mainSizer);
}

void WinPlayerEdit::OnPlayerChoice(wxCommandEvent& event) {

	// Only skip to get the aliases to display if the current selection is valid and not a new player
	if (aliasChoice->GetStringSelection().ToStdString() != "<New player>" && aliasChoice->GetStringSelection().ToStdString() != "") {
		std::string* theData = new std::string(aliasChoice->GetStringSelection().ToStdString());

		if (!hidePlayerCheck->IsEnabled()) {
			hidePlayerCheck->Enable();
		}

		event.SetClientData(theData);
		event.Skip();
	}
	else {
		aliasListView->DeleteAllItems(); // New player is selected(or invalid selection), so don't display aliases 
		if (hidePlayerCheck->IsEnabled()) {
			hidePlayerCheck->Disable();
		}
	}
}

void WinPlayerEdit::OnAliasAddBtn(wxCommandEvent& event) {
	wxTextEntryDialog* aliasInputDialog = new wxTextEntryDialog(this, wxString("Please enter new alias"), wxString("New alias"));

	std::pair<std::string, std::string>* theAliases;

	switch (aliasInputDialog->ShowModal()) {
	case wxID_OK:
		if (aliasInputDialog->GetValue() == wxEmptyString) {
			wxMessageBox(wxString("No alias entered, player remains unchanged."), wxString("No valid input"));
			return;
		}
		// We'll have to give the main alias of the user to add the alias to as well
		theAliases = new std::pair<std::string, std::string>(aliasChoice->GetStringSelection().ToStdString(), aliasInputDialog->GetValue().ToStdString());
		event.SetClientData(theAliases);
		event.Skip();
		break;
	case wxID_CANCEL:
		break;
	}
}

void WinPlayerEdit::OnAliasRemBtn(wxCommandEvent& event) {
	std::string* aliasToRem;
	
	if (aliasListView->GetFirstSelected() == -1) {
		return;
	}

	aliasToRem = new std::string(aliasListView->GetItemText(aliasListView->GetFirstSelected()).ToStdString());

	event.SetClientData(aliasToRem);
	event.Skip();
}

void WinPlayerEdit::OnAliasMainBtn(wxCommandEvent& event) {
	long item = aliasListView->GetFirstSelected();

	if (item == -1) { return; }

	std::string* theData = new std::string(aliasListView->GetItemText(item));

	event.SetClientData(theData);
	event.Skip();
}

void WinPlayerEdit::OnPlayerToggleVis(wxCommandEvent& event) {
	std::string* theData = new std::string(aliasChoice->GetStringSelection().ToStdString());
	event.SetClientData(theData);
	event.Skip();
}

void WinPlayerEdit::OnPlayerRemBtn(wxCommandEvent& event) {
	std::string* aliasToRem;

	if (aliasChoice->GetStringSelection().IsSameAs(wxString("<New player>"))) {
		return;
	}

	aliasToRem = new std::string(aliasChoice->GetStringSelection().ToStdString());

	event.SetClientData(aliasToRem);
	event.Skip();
}


bool compareAlphabetically(std::string stringOne, std::string stringTwo) {

	std::transform(stringOne.begin(), stringOne.end(), stringOne.begin(), std::tolower);
	std::transform(stringTwo.begin(), stringTwo.end(), stringTwo.begin(), std::tolower);

	return stringOne < stringTwo;
}

void WinPlayerEdit::setMainAliases(std::vector<std::string> newMainAliases, std::string selected) {
	std::sort(newMainAliases.begin(), newMainAliases.end(), compareAlphabetically);

	mainAliases = wxArrayString();
	mainAliases.push_back(wxString("<New player>"));
	for (std::vector<std::string>::iterator currAlias = newMainAliases.begin(); currAlias != newMainAliases.end(); currAlias++) {
		mainAliases.push_back(wxString(*currAlias));
	}
	// Recreate selection with the new aliases
	aliasChoice->Clear();
	aliasChoice->Append(mainAliases);

	// look for the item to select
	int item = -1;
	item = aliasChoice->FindString(wxString(selected));

	if (item != wxNOT_FOUND) {
		aliasChoice->SetSelection(item);
		if (selected != "<New player>") {
			hidePlayerCheck->Enable();
		}
	}
}

void WinPlayerEdit::setPlayersAliases(std::vector<std::string> playersAliases) {
	aliasListView->DeleteAllItems();

	for (unsigned int i = 0; i < playersAliases.size(); i++) {
		aliasListView->InsertItem(i, wxString(playersAliases[i]));
	}
}

void WinPlayerEdit::setHidden(bool isHidden) {
	hidePlayerCheck->SetValue(isHidden);
}

bool WinPlayerEdit::getHidden() const {
	return hidePlayerCheck->GetValue();
}