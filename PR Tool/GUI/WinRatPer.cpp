#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "WinRatPer.h"
#include "GlobalVars.h"

#include <wx/calctrl.h>
#include <wx/numformatter.h>

#include <locale>

WinRatPer::WinRatPer(wxWindow* parent, wxWindowID winid)
	:wxPanel(parent, winid)
{

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// ------------ Rating Table ------------
	ratingTable = new wxListView(this, ID_RAT_PER_PLA_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
	float onePart = (winMinWidth - 30) / 21;
	ratingTable->InsertColumn(0, wxString("Name"), 0, (int)onePart * 5);
	ratingTable->InsertColumn(1, wxString("Rating"), 0, (int)onePart * 4);
	ratingTable->InsertColumn(2, wxString("Sets"), 0,(int)onePart * 3);		
	ratingTable->InsertColumn(3, wxString("Wins"), 0,(int)onePart * 3);		
	ratingTable->InsertColumn(4, wxString("Losses"), 0, (int)onePart * 3);	
	ratingTable->InsertColumn(5, wxString("Win %"), 0, (int)onePart * 3);	
	ratingViewItemID = 0; // will be used to assign IDs to the added items. Could probably just use getItemCount()...
	ratingViewSortedColumn = 1; // sort by rating by default
	ratingViewDescending = true; // sort descending by default

	mainSizer->Add(ratingTable, 1, wxEXPAND);

	// ------------ Rating Periods Table ------------
	periodTable = new wxListView(this, wxID_ANY, wxDefaultPosition, wxSize(20, 120), wxLC_REPORT);
	periodTable->InsertColumn(0, "Rating Period");
	periodTable->InsertColumn(1, "Start");
	periodTable->InsertColumn(2, "End");
	periodViewItemID = 0; // will be used to assign IDs to the added items

	mainSizer->Add(periodTable, 0, wxEXPAND);

	wxBoxSizer* periodBtnSizer = new wxBoxSizer(wxHORIZONTAL);

	wxButton* addPerBtn = new wxButton(this, ID_RAT_PER_ADD_BTN, "Add rating period...");
	addPerBtn->SetToolTip("Adds a period of time in which sets are considered for one iteration. Should include at least 10-15 sets.");
	addPerBtn->SetDefault();

	wxButton* remPerBtn = new wxButton(this, ID_RAT_PER_REM_BTN, "Remove rating period");
	remPerBtn->SetToolTip("Removes selected rating period, updating ratings to exclude sets from that time.");

	Bind(wxEVT_BUTTON, &WinRatPer::OnBtnAddPer, this, ID_RAT_PER_ADD_BTN);
	Bind(wxEVT_BUTTON, &WinRatPer::OnBtnRemPer, this, ID_RAT_PER_REM_BTN);
	Bind(wxEVT_LIST_COL_CLICK, &WinRatPer::OnColumnClick, this, ID_RAT_PER_PLA_LIST);

	periodBtnSizer->Add(addPerBtn);
	periodBtnSizer->Add(remPerBtn);

	mainSizer->Add(periodBtnSizer, 0, wxALIGN_CENTER);
	/*
		this -> mainSizer	-> ratingTable
							-> periodTable
							-> periodBtnSizer	-> addPerBtn
												-> remPerBtn
	*/
	SetSizer(mainSizer);
}

void WinRatPer::OnBtnAddPer(wxCommandEvent& event) {
	// ------------ Creating Dialog ------------ 
	wxDialog* addPerDialog = new wxDialog(this, wxID_ANY, "Add rating period", wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER);

	// ------------ Main Sizer ------------
	wxBoxSizer* perSizer = new wxBoxSizer(wxVERTICAL);

	// -==========- Period Selection -==========-
	wxFlexGridSizer* dateSizer = new wxFlexGridSizer(2, wxSize(5, 15));
	dateSizer->AddGrowableCol(0, 1);
	dateSizer->AddGrowableCol(1, 1);
	dateSizer->AddGrowableRow(1, 1);

	wxStaticText* startText = new wxStaticText(addPerDialog, wxID_ANY, wxString("Start date:"));
	wxCalendarCtrl* startDate = new wxCalendarCtrl(addPerDialog, wxID_ANY);
	wxStaticText* endText = new wxStaticText(addPerDialog, wxID_ANY, wxString("End date:"));
	wxCalendarCtrl* endDate = new wxCalendarCtrl(addPerDialog, wxID_ANY);

	// add to period sizer
	dateSizer->Add(startText);
	dateSizer->Add(endText);
	dateSizer->Add(startDate, 1, wxEXPAND);
	dateSizer->Add(endDate, 1, wxEXPAND);

	// add to main sizer
	perSizer->Add(dateSizer, 1, wxEXPAND);

	// -==========- confirm/cancel buttons -==========-
	wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);

	wxButton* okBtn = new wxButton(addPerDialog, wxID_OK, wxString("OK"));
	wxButton* cancBtn = new wxButton(addPerDialog, wxID_CANCEL, wxString("Cancel"));

	// add to button sizer
	btnSizer->Add(okBtn);
	btnSizer->Add(cancBtn);
	
	okBtn->SetDefault();
	// add to main sizer
	perSizer->Add(btnSizer, 0, wxALIGN_CENTER);

	addPerDialog->SetSizerAndFit(perSizer);


	switch (addPerDialog->ShowModal()) {
	case wxID_OK:
		if (endDate->GetDate().IsEarlierThan(startDate->GetDate()) || startDate->GetDate().IsEqualTo(endDate->GetDate())) {
			wxMessageBox(wxString("Selected end date was earlier or equal to start date."), wxString("Invalid date"), wxOK | wxICON_WARNING);
		}
		else {
			std::pair<wxDateTime, wxDateTime>* periodTime = new std::pair<wxDateTime, wxDateTime>(startDate->GetDate(), endDate->GetDate()); // Dead when leaving scope??

			event.SetClientData(periodTime);
			event.Skip();
		}
		break;
	case wxID_CANCEL:
		break;
	}

	addPerDialog->Destroy();
}

void WinRatPer::OnBtnRemPer(wxCommandEvent& event) {
	long id = periodTable->GetFirstSelected();

	if (id == -1) {
		//wxMessageBox(wxString("No Period to remove selected!"), wxString("Invalid operation"));
		return;
	}

	wxDateTime start;
	wxDateTime end;

	wxString::const_iterator end_iterator; // will be set to where the parsing stops, so should be at the end then

	if (!start.ParseFormat(periodTable->GetItemText(id, 1), defaultFormatString, &end_iterator)) {
		wxMessageBox(wxString("The string: " + periodTable->GetItemText(id, 1) + " could not be parsed to a date. Please report"), wxString("Unsuccessful parsing of date"));
		return;
	}

	if (!end.ParseFormat(periodTable->GetItemText(id, 2), defaultFormatString, &end_iterator)) {
		wxMessageBox(wxString("The string: " + periodTable->GetItemText(id, 2) + " could not be parsed to a date. Please report"), wxString("Unsuccessful parsing of date"));
		return;
	}

	std::pair<wxDateTime, wxDateTime>* data = new std::pair<wxDateTime, wxDateTime>(start, end);
	event.SetClientData(data);
	event.Skip();
}

int wxCALLBACK sortMyPlayers(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortData) {

	std::tuple<wxListView*, int, bool>* realSortData = (std::tuple<wxListView*, int, bool>*) sortData;

	wxListView* theTable = std::get<0>(*realSortData);
	int sortColumn = std::get<1>(*realSortData);
	bool descend = std::get<2>(*realSortData);

	long item1ID = theTable->FindItem(-1, item1);
	long item2ID = theTable->FindItem(-1, item2);

	std::string item1String = theTable->GetItemText(item1ID, sortColumn).ToStdString();
	std::string item2String = theTable->GetItemText(item2ID, sortColumn).ToStdString();

	// if the column represents numbers try to compare those as ints
	int item1Int;
	int item2Int;
	try {
		item1Int = std::stoi(item1String);
		item2Int = std::stoi(item2String);
	}
	catch (const std::invalid_argument) {
		// if it doesn't work use the strings

		// make it case-insensitive
		std::locale loc;
		for (auto currChar = item1String.begin(); currChar != item1String.end(); currChar++) {
			*currChar = std::tolower(*currChar, loc);
		}
		for (auto currChar = item2String.begin(); currChar != item2String.end(); currChar++) {
			*currChar = std::tolower(*currChar, loc);
		}

		// compare
		if (descend) {
			return item1String.compare(item2String);
		}
		else {
			return item1String.compare(item2String) * (-1);
		}
	}

	if (item1Int < item2Int) {
		if (descend) {
			return 1;
		}
		else {
			return -1;
		}
	}
	else if (item1Int > item2Int) {
		if (descend) {
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

void WinRatPer::OnColumnClick(wxListEvent& event) {
	if (event.GetColumn() != -1) {
		if (event.GetColumn() == ratingViewSortedColumn) {
			ratingViewDescending = !ratingViewDescending;
		}
		else {
			ratingViewSortedColumn = event.GetColumn();
		}
	}

	std::tuple<wxListView*, int, bool>* sortData = new std::tuple<wxListView*, int, bool>(ratingTable, ratingViewSortedColumn, ratingViewDescending);
	ratingTable->SortItems(&sortMyPlayers, (wxIntPtr)sortData);
	delete sortData;
}

// helper function
int wxCALLBACK sortMyItems(wxIntPtr item1, wxIntPtr item2, wxIntPtr WXUNUSED(sortData)) {
	if (item1 < item2) {
		return -1;
	}
	if (item1 > item2) {
		return 1;
	}
	else {
		return 0;
	}
}

void WinRatPer::addRatingPeriod(const wxDateTime& start, const wxDateTime& end) {
	periodTable->InsertItem(periodViewItemID, wxString(std::to_string(periodViewItemID)));
	periodTable->SetItem(periodViewItemID, 1, wxString(start.Format(defaultFormatString)));
	periodTable->SetItem(periodViewItemID, 2, wxString(end.Format(defaultFormatString)));

	// sorting
	periodTable->SetItemData(periodViewItemID, start.GetTicks());
	periodTable->SortItems(&sortMyItems, 0);
	periodViewItemID++;

	// renaming the first column of each entry to correctly show their current index (because sorting may have destroyed the order)
	long item = -1;
	item = periodTable->GetNextItem(item);
	while (item != -1) {
		periodTable->SetItemText(item, wxString(std::to_string(item)));
		item = periodTable->GetNextItem(item);
	}
}

void WinRatPer::removeRatingPeriod(wxDateTime& start, wxDateTime& end) {
	long item = -1;
	item = periodTable->GetNextItem(item);

	while (!(start.Format(defaultFormatString).IsSameAs(periodTable->GetItemText(item, 1)) 
		  && end.Format(defaultFormatString).IsSameAs(periodTable->GetItemText(item, 2)))) {

		item = periodTable->GetNextItem(item);
	}
	periodTable->DeleteItem(item);

	// Select item that was in the place of the deleted one or the previous if the last item got deleted or none if the only one got deleted
	if (periodTable->GetItemCount() == 0) {
		periodViewItemID--;
		return;
	}
	else if (periodTable->GetItemCount() == item) {
		periodTable->Select(item - 1);
	}
	else {
		periodTable->Select(item);
	}

	item = -1;
	item = periodTable->GetNextItem(item);

	// renaming the first column of each entry to correctly show their current index
	while (item != -1) {
		periodTable->SetItemText(item, wxString(std::to_string(item)));
		item = periodTable->GetNextItem(item);
	}

	periodViewItemID--;
}

void WinRatPer::addPlayer(unsigned int id, std::string displayAlias) {
	ratingTable->InsertItem(ratingViewItemID, wxString(displayAlias));
	ratingTable->SetItemData(ratingViewItemID, id);

	ratingViewItemID++;
}

void WinRatPer::updatePlayer(unsigned int id, double rating, unsigned int wins, unsigned int losses) {
	long item = -1;
	item = ratingTable->GetNextItem(item);
	while (ratingTable->GetItemData(item) != id) {
		item = ratingTable->GetNextItem(item);
		if (item == -1) {
			wxMessageBox(wxString("Player not found. Scores not updated in rating table."), wxString("ID not found"));
			return;
		}
	}

	float winPercent = 0;
	if (wins + losses != 0) {
		winPercent = (float)wins / (((float)wins + (float)losses) / 100);
	}

	ratingTable->SetItem(item, 1, wxString(std::to_string(rating).substr(0, std::to_string(rating).find_last_of('.'))));
	ratingTable->SetItem(item, 2, wxString(std::to_string(wins + losses)));
	ratingTable->SetItem(item, 3, wxString(std::to_string(wins)));
	ratingTable->SetItem(item, 4, wxString(std::to_string(losses)));
	ratingTable->SetItem(item, 5, wxString(std::to_string((int)winPercent)));

	/* This gets called after every player added slowing down startup... Supply method to call once when player adding/updating is done...
	std::tuple<wxListView*, int, bool>* sortData = new std::tuple<wxListView*, int, bool>(ratingTable, ratingViewSortedColumn, ratingViewDescending);
	ratingTable->SortItems(&sortMyPlayers, (wxIntPtr)sortData);
	delete sortData; */
}

void WinRatPer::updatePlayerID(unsigned int oldId, std::string displayAlias, unsigned int newId) {
	long item = -1;
	item = ratingTable->GetNextItem(item);

	while (ratingTable->GetItemData(item) != oldId
		&& !(ratingTable->GetItemText(item).IsSameAs(wxString(displayAlias)))) {

		item = ratingTable->GetNextItem(item);
		if (item == -1) {
			wxMessageBox(wxString("Player not found. ID not updated in rating table."), wxString("ID not found"));
			return;
		}
	}
	ratingTable->SetItemData(item, newId);
}

void WinRatPer::updatePlayerDisplayAlias(unsigned int id, std::string oldAlias, std::string newAlias) {
	long item = -1;
	item = ratingTable->GetNextItem(item);
	while (ratingTable->GetItemData(item) != id
		&& !ratingTable->GetItemText(item).IsSameAs(wxString(oldAlias))
		&& item != -1) {
		item = ratingTable->GetNextItem(item);
	}
	ratingTable->SetItem(item, 0, wxString(newAlias));

	std::tuple<wxListView*, int, bool>* sortData = new std::tuple<wxListView*, int, bool>(ratingTable, ratingViewSortedColumn, ratingViewDescending);
	ratingTable->SortItems(&sortMyPlayers, (wxIntPtr)sortData);
	delete sortData;
}

void WinRatPer::removePlayer(unsigned int id) {
	long item = -1;
	item = ratingTable->GetNextItem(item);
	while (ratingTable->GetItemData(item) != id && item != -1) {
		item = ratingTable->GetNextItem(item);
	}
	if (item == -1) {
		wxMessageBox(wxString("Player not found. Player not removed from table (that is probably pretty bad)."), wxString("ID not found"));
		return;
	}
	ratingTable->DeleteItem(item);
	ratingViewItemID--;
}

void WinRatPer::sortMatchTable() {
	std::tuple<wxListView*, int, bool>* sortData = new std::tuple<wxListView*, int, bool>(ratingTable, ratingViewSortedColumn, ratingViewDescending);
	ratingTable->SortItems(&sortMyPlayers, (wxIntPtr)sortData);
	delete sortData;
}