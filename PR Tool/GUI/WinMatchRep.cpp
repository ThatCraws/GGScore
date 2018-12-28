#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "WinMatchRep.h"
#include "GlobalVars.h"

#include <wx/calctrl.h>

#include <cctype>
#include <locale>

WinMatchRep::WinMatchRep(wxWindow* parent, wxWindowID id) 
	: wxPanel(parent, id) {

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	float onePart = (winMinWidth - 30) / 5;
	matchTable = new wxListView(this, ID_MAT_REP_RES_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	matchTable->InsertColumn(0, "Date", 0, (int)onePart);
	matchTable->InsertColumn(1, "Winner", 0, (int)onePart * 2);
	matchTable->InsertColumn(2, "Loser", 0, (int)onePart * 2);
	listViewItemID = 0; // used to give every item unique itemData (NOT index of item)
	resultViewSortedColumn = 0; // sort by date by default
	resultViewDescending = true; // sort descending by default

	mainSizer->Add(matchTable, 1, wxEXPAND);

	wxBoxSizer* resultBtnSizer = new wxBoxSizer(wxHORIZONTAL);

	// ------------ Add Button ------------
	wxButton* addResultBtn = new wxButton(this, ID_MAT_REP_ADD_BTN, wxString("Add Result..."));
	addResultBtn->SetToolTip(wxString("Report a set manually"));
	addResultBtn->SetDefault();
	// ------------ Remove Button ------------
	wxButton* remResultBtn = new wxButton(this, ID_MAT_REP_REM_BTN, wxString("Remove Result"));
	remResultBtn->SetToolTip(wxString("Remove the selected entry"));

	resultBtnSizer->Add(addResultBtn);
	resultBtnSizer->Add(remResultBtn);

	mainSizer->Add(resultBtnSizer, 0, wxALIGN_CENTER);

	wxBoxSizer* importBtnSizer = new wxBoxSizer(wxHORIZONTAL);

	wxButton* importBtn = new wxButton(this, ID_MAT_REP_IMP_BTN, wxString("Import Challonge bracket..."));
	importBtn->SetToolTip(wxString("Import a challonge bracket by URL"));

	importBtnSizer->Add(importBtn);
	mainSizer->Add(importBtnSizer, 0, wxALIGN_CENTER);

	// ------------ Info text, colored forfeits ------------
	wxBoxSizer* infoSizer = new wxBoxSizer(wxHORIZONTAL);
	wxWindow* forfeitColor = new wxWindow(this, wxID_ANY, wxDefaultPosition, wxSize(20, 20));
	forfeitColor->SetBackgroundColour(wxColour(208, 208, 208));
	forfeitColor->Refresh();
	wxStaticText* forfeitTxt = new wxStaticText(this, wxID_ANY, wxString(" = Forfeit "));

	wxWindow* tieColor = new wxWindow(this, wxID_ANY, wxDefaultPosition, wxSize(20, 20));
	tieColor->SetBackgroundColour(wxColour(144, 144, 144));
	tieColor->Refresh();
	wxStaticText* tieTxt = new wxStaticText(this, wxID_ANY, wxString(" = Tie "));

	infoSizer->Add(forfeitColor);
	infoSizer->Add(forfeitTxt);

	infoSizer->Add(tieColor);
	infoSizer->Add(tieTxt);

	mainSizer->Add(infoSizer, 0, wxALIGN_RIGHT);

	// bind methods
	Bind(wxEVT_BUTTON, &WinMatchRep::OnBtnAdd, this, ID_MAT_REP_ADD_BTN);
	Bind(wxEVT_BUTTON, &WinMatchRep::OnBtnRem, this, ID_MAT_REP_REM_BTN);
	// Bind of import-btn only in mainWin (no additional data from this tab needed)
	Bind(wxEVT_LIST_COL_CLICK, &WinMatchRep::OnColumnClick, this, ID_MAT_REP_RES_LIST);

	SetSizer(mainSizer);

	/*
		this -> mainSizer	-> MatchTable
							-> resultBtnSizer	-> addResultBtn
												-> remResultBtn
							-> importBtnSizer	-> importBtn
	*/

}

void WinMatchRep::OnBtnAdd(wxCommandEvent& event) {
	// ------------ Creating Dialog ------------ 
	wxDialog* addResDialog = new wxDialog(this, wxID_ANY, wxString("Manual Match Report"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER);

	// ------------ Main Sizer ------------ 
	wxBoxSizer* resSizer = new wxBoxSizer(wxVERTICAL);

	// -==========- Player and Winner selection -==========-
	wxBoxSizer* playerSelSizer = new wxBoxSizer(wxHORIZONTAL);

	// ------------ Player 1 Selection ------------ 
	
	wxBoxSizer* player1Sizer = new wxBoxSizer(wxVERTICAL);
	wxComboBox* player1 = new wxComboBox(addResDialog, wxID_ANY, wxString("Player 1"), wxDefaultPosition, wxSize(100, -1), mainAliases);
	wxRadioButton* p1Won = new wxRadioButton(addResDialog, wxID_ANY, wxString("Won"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);

	player1Sizer->Add(player1, wxEXPAND);
	player1Sizer->Add(p1Won);

	// ------------ Player 2 Selection ------------ 
	wxBoxSizer* player2Sizer = new wxBoxSizer(wxVERTICAL);

	wxComboBox* player2 = new wxComboBox(addResDialog, wxID_ANY, wxString("Player 2"), wxDefaultPosition, wxSize(100, -1), mainAliases);
	wxRadioButton* p2Won = new wxRadioButton(addResDialog, wxID_ANY, wxString("Won"));

	player2Sizer->Add(player2, wxEXPAND | wxALIGN_CENTRE_VERTICAL);
	player2Sizer->Add(p2Won);

	playerSelSizer->Add(player1Sizer);
	playerSelSizer->Add(player2Sizer);

	resSizer->Add(playerSelSizer, 0, wxCENTER);

	// -=========== Match settings ===========-
	wxBoxSizer* matchSetSizer = new wxBoxSizer(wxHORIZONTAL);
	// ------------ Forfeit checkbox ------------
	wxCheckBox* forfeit = new wxCheckBox(addResDialog, wxID_ANY, wxString("Win/loss by Forfeit"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	forfeit->SetValue(false);
	matchSetSizer->Add(forfeit);

	// ------------ Tie checkbox ------------
	wxCheckBox* tieBox = new wxCheckBox(addResDialog, wxID_ANY, wxString("Tie"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	tieBox->SetValue(false);
	matchSetSizer->Add(tieBox);

	resSizer->AddSpacer(5);
	resSizer->Add(matchSetSizer, 0, wxALIGN_CENTER);

	// ------------ Date Picker ------------ 

	wxCalendarCtrl* datePick = new wxCalendarCtrl(addResDialog, wxID_ANY);

	resSizer->Add(datePick, 1, wxEXPAND);

	// ------------ OK/Cancel Buttons ------------ 
	wxBoxSizer* naviSizer = new wxBoxSizer(wxHORIZONTAL);

	wxButton* okBtn = new wxButton(addResDialog, wxID_OK, "OK");
	okBtn->SetDefault();
	wxButton* cancBtn = new wxButton(addResDialog, wxID_CANCEL, "Cancel");

	naviSizer->Add(okBtn);
	naviSizer->Add(cancBtn);

	resSizer->Add(naviSizer, 0, wxCENTER);

	addResDialog->SetSizerAndFit(resSizer);
	

	/*
		addRes -> resSizer	-> playerSelSizer	-> player1Sizer	-> player1
																-> p1Won
												-> player2Sizer	-> player2
																-> p2Won
							-> datePicker
							-> naviSizer		-> okBtn
												-> cancBtn
	*/

	switch (addResDialog->ShowModal())
	{
	case wxID_OK:
		if (player1->GetValue().IsSameAs(player2->GetValue())) {
			wxMessageBox(wxString("The entered player were the same!"));
			return;
		}
		else if (forfeit->GetValue() && tieBox->GetValue()) {
			wxMessageBox(wxString("The result can not be a forfeit AND a tie!"));
			return;
		}
		break;
	case wxID_CANCEL:
	case wxID_EXIT:
		addResDialog->Destroy();
		return;
	} 

	// String alias for both players and the winning party

	std::tuple<std::string, std::string, wxDateTime, bool, bool>* data = new std::tuple<std::string, std::string, wxDateTime, bool, bool>();
	
	// Winner is first in vector
	if (p1Won->GetValue()) {
		*data = std::make_tuple(player1->GetValue().ToStdString(), player2->GetValue().ToStdString(), datePick->GetDate(), forfeit->GetValue(), tieBox->GetValue());
	}
	else {
		*data = std::make_tuple(player2->GetValue().ToStdString(), player1->GetValue().ToStdString(), datePick->GetDate(), forfeit->GetValue(), tieBox->GetValue());
	}

	addResDialog->Destroy();

	event.SetClientData(data);
	event.Skip();
}

void WinMatchRep::OnBtnRem(wxCommandEvent& event) {
	long item = matchTable->GetFirstSelected();

	if (item == -1) {
		return;
	}

	std::string winnerAlias = matchTable->GetItemText(item, 1).ToStdString();
	std::string loserAlias = matchTable->GetItemText(item, 2).ToStdString();
	wxDateTime matchDate;
	matchDate.ParseFormat(matchTable->GetItemText(item, 0), defaultFormatString);
	bool tie = matchTable->GetItemBackgroundColour(item) == wxColour(144, 144, 144);

	std::tuple<std::string, std::string, wxDateTime, bool>* data = new std::tuple<std::string, std::string, wxDateTime, bool>(winnerAlias, loserAlias, matchDate, tie);

	event.SetClientData(data);
	event.Skip();
}

// helper functions (has to be above addResult and OnColumnClick) (used to sort the table)
int wxCALLBACK sortMyResults(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortData) {

	std::tuple<wxListView*, int, bool>* realSortData = (std::tuple<wxListView*, int, bool>*) sortData;

	wxListView* theTable = std::get<0>(*realSortData);
	int sortColumn = std::get<1>(*realSortData);
	bool descend = std::get<2>(*realSortData);

	std::string item1String = theTable->GetItemText(theTable->FindItem(-1, item1), sortColumn).ToStdString();
	std::string item2String = theTable->GetItemText(theTable->FindItem(-1, item2), sortColumn).ToStdString();

	std::locale loc;
	for (auto currChar = item1String.begin(); currChar != item1String.end(); currChar++) {
		*currChar = std::tolower(*currChar, loc);
	}

	for (auto currChar = item2String.begin(); currChar != item2String.end(); currChar++) {
		*currChar = std::tolower(*currChar, loc);
	}

	if (descend) {
		return item2String.compare(item1String);
	}
	else {
		return item1String.compare(item2String);
	}
}

int wxCALLBACK sortResultsByDate(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortData) {
	std::pair<wxListView*, bool>* realSortData = (std::pair<wxListView*, bool>*)sortData;

	wxListView* theTable = realSortData->first;
	bool descending = realSortData->second;

	wxDateTime item1Date;
	wxDateTime item2Date;

	item1Date.ParseFormat(theTable->GetItemText(theTable->FindItem(-1, item1)), defaultFormatString);
	item2Date.ParseFormat(theTable->GetItemText(theTable->FindItem(-1, item2)), defaultFormatString);

	if (item1Date.IsEarlierThan(item2Date)) {
		if (descending) {
			return 1;
		}
		else {
			return -1;
		}
	}
	if (item1Date.IsLaterThan(item2Date)) {
		if (descending) {
			return -1;
		}
		else {
			return 1;
		}
	}
	else {
		return 0;
	}
}

void WinMatchRep::OnColumnClick(wxListEvent& event) {
	if (event.GetColumn() != -1) {
		if (event.GetColumn() == resultViewSortedColumn) {
			resultViewDescending = !resultViewDescending;
		}
		else {
			resultViewSortedColumn = event.GetColumn();
		}
		sortResultTable();
	}
}

void WinMatchRep::sortResultTable() {
	if (resultViewSortedColumn == 0) {
		std::pair<wxListView*, bool>* sortData = new std::pair<wxListView*, bool>(matchTable, resultViewDescending);
		matchTable->SortItems(&sortResultsByDate, (wxIntPtr)sortData);
		delete sortData;
	}
	else {
		std::tuple<wxListView*, int, bool>* sortData = new std::tuple<wxListView*, int, bool>(matchTable, resultViewSortedColumn, resultViewDescending);
		matchTable->SortItems(&sortMyResults, (wxIntPtr)sortData);
		delete sortData;
	}
}

void WinMatchRep::addResult(std::string winnerAlias, std::string loserAlias, const wxDateTime date, bool forfeit, bool tie) {
	if (forfeit && tie) {
		wxMessageBox(wxString("A forfeit can not be a tie as well!"));
		return;
	}

	matchTable->InsertItem(listViewItemID, wxString(date.Format(defaultFormatString)));
	matchTable->SetItem(listViewItemID, 1, wxString(winnerAlias));
	matchTable->SetItem(listViewItemID, 2, wxString(loserAlias));

	// sorting 
	matchTable->SetItemData(listViewItemID, listViewItemID);

	if (forfeit) {
		matchTable->SetItemBackgroundColour(listViewItemID, wxColour(208, 208, 208));
	}
	else if (tie) {
		matchTable->SetItemBackgroundColour(listViewItemID, wxColour(144, 144, 144));
	}

	listViewItemID++;
}

void WinMatchRep::removeResult() {
	long item = -1;

	item = matchTable->GetFirstSelected();

	matchTable->DeleteItem(item);

	// Select item that was in the place of the deleted one or the previous if the last item got deleted or none if the only one got deleted
	if (matchTable->GetItemCount() == 0) {
	}
	else if (matchTable->GetItemCount() == item) {
		matchTable->Select(item - 1);
	}
	else {
		matchTable->Select(item);
	}

	reSetItemData();

	listViewItemID--;
}

void WinMatchRep::removeResult(std::string winnerAlias, std::string loserAlias, const wxDateTime& date) {
	long item = -1;

	item = matchTable->GetNextItem(item);

	while (!(matchTable->GetItemText(item, 0).IsSameAs(date.Format(defaultFormatString))
		  && matchTable->GetItemText(item, 1).IsSameAs(wxString(winnerAlias))
		  && matchTable->GetItemText(item, 2).IsSameAs(wxString(loserAlias)))) {

		item = matchTable->GetNextItem(item);
	}
	if (item != -1) {
		matchTable->DeleteItem(item);

		// Select item that was in the place of the deleted one or the previous if the last item got deleted or none if the only one got deleted
		if (matchTable->GetItemCount() == 0) {
		}
		else if (matchTable->GetItemCount() == item) {
			matchTable->Select(item - 1);
		}
		else {
			matchTable->Select(item);
		}

		reSetItemData();

		listViewItemID--;
	}
}

void WinMatchRep::reSetItemData() {
	long item = matchTable->GetNextItem(-1);

	while (item != -1) {
		matchTable->SetItemData(item, item);
		item = matchTable->GetNextItem(item);
	}
}

void WinMatchRep::clearResultTable() {
	matchTable->DeleteAllItems();
	listViewItemID = 0;
}

// "Two"-postfix because "compareAlphabetically" is already in playerEdit
// Would have to call this everytime before calling setMainAliases, if I tried to put it in MainWin
bool compareAlphabeticallyTwo(std::string stringOne, std::string stringTwo) {

	std::transform(stringOne.begin(), stringOne.end(), stringOne.begin(), tolower);
	std::transform(stringTwo.begin(), stringTwo.end(), stringTwo.begin(), tolower);

	return stringOne < stringTwo;
}

void WinMatchRep::setMainAliases(std::vector<std::string> newAliases) {
	std::sort(newAliases.begin(), newAliases.end(), compareAlphabeticallyTwo);

	wxArrayString aliasArray = wxArrayString();
	for (auto currAlias = newAliases.begin(); currAlias != newAliases.end(); currAlias++) {
		aliasArray.Add(*currAlias);
	}

	mainAliases = aliasArray;
}

void WinMatchRep::updatePlayerDisplayAlias(std::string oldAlias, std::string newAlias) {
	long item = -1;

	item = matchTable->GetNextItem(item);

	while (item != -1) {
		if (matchTable->GetItemText(item, 1).IsSameAs(wxString(oldAlias))) {
			matchTable->SetItem(item, 1, wxString(newAlias));
		}
		else if (matchTable->GetItemText(item, 2).IsSameAs(wxString(oldAlias))) {
			matchTable->SetItem(item, 2, wxString(newAlias));
		}

		item = matchTable->GetNextItem(item);
	}
	sortResultTable();
}