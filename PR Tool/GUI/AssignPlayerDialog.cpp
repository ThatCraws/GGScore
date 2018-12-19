#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/spinctrl.h>

#include "AssignPlayerDialog.h"

AssignPlayerDialog::AssignPlayerDialog(wxWindow* parent, wxWindowID id, const wxString& title, const std::string aliasToAssign, wxArrayString assignables,
	double defaultRating, double defaultDeviation, double defaultVolatility)
	: wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER)
{
	// Add the "add new player"-option to the alias array
	assignables.Insert(wxString("<New player>"), 0);
	// Since New player is the starting choice, we start in createMode
	createMode = true;

	mainSizer = new wxBoxSizer(wxVERTICAL);

	// Alias selection to assign new alias to
	wxBoxSizer* aliasSelectionSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* whoIsText = new wxStaticText(this, wxID_ANY, wxString("Who is ''" + aliasToAssign + "''?"));
	aliasChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, assignables);
	aliasChoice->SetSelection(0);

	aliasSelectionSizer->AddSpacer(10);
	aliasSelectionSizer->Add(whoIsText);
	aliasSelectionSizer->AddSpacer(10);
	aliasSelectionSizer->Add(aliasChoice);
	aliasSelectionSizer->AddSpacer(10);

	mainSizer->AddSpacer(10);
	mainSizer->Add(aliasSelectionSizer, 0, wxALIGN_CENTER);
	mainSizer->AddSpacer(10);


	// New player starting values (only shown when "<New player>" is selected in choice (since it's the start value, dialog gets built with it)
	newPlayerValuesSizer = new wxStaticBoxSizer(wxVERTICAL, this, wxString("Starting values"));
	wxGridSizer* valueSizer = new wxGridSizer(2);

	// Rating value
	wxStaticText* ratingText = new wxStaticText(this, wxID_ANY, wxString("Rating: "));
	ratingVal = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, -1), wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0, 2500, defaultRating);
	valueSizer->Add(ratingText);
	valueSizer->Add(ratingVal);

	// Deviation value
	wxStaticText* deviationText = new wxStaticText(this, wxID_ANY, wxString("Deviation: "));
	deviationVal = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, -1), wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0, 2500, defaultDeviation);
	valueSizer->Add(deviationText);
	valueSizer->Add(deviationVal);

	// Deviation value
	wxStaticText* volatilityText = new wxStaticText(this, wxID_ANY, wxString("Volatility: "));
	volatilityVal = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, -1), wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0, 2500, defaultVolatility, 0.001);
	valueSizer->Add(volatilityText);
	valueSizer->Add(volatilityVal);

	// Just for the textbox
	newPlayerValuesSizer->Add(valueSizer, 1, wxEXPAND);

	mainSizer->Add(newPlayerValuesSizer, 1, wxEXPAND);

	// OK and Cancel Buttons
	wxBoxSizer* naviSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* okButton = new wxButton(this, wxID_OK, wxString("OK"));
	wxButton* cancButton = new wxButton(this, wxID_CANCEL, wxString("Cancel"));
	naviSizer->Add(okButton);
	naviSizer->Add(cancButton);

	okButton->SetDefault();

	mainSizer->Add(naviSizer, 0, wxALIGN_CENTER);

	SetSizerAndFit(mainSizer);

	Bind(wxEVT_CHOICE, &AssignPlayerDialog::OnChoiceSelection, this, aliasChoice->GetId());
}

void AssignPlayerDialog::OnChoiceSelection(wxCommandEvent& event) {
	if (aliasChoice->GetStringSelection().IsSameAs("<New player>")) {
		setToCreateMode();
	}
	else {
		setToAssignMode();
	}
}

void AssignPlayerDialog::setToAssignMode() {
	if (createMode) {
		mainSizer->Hide(newPlayerValuesSizer, true);
		mainSizer->Layout();
		this->SetSizerAndFit(mainSizer);
		createMode = false;
	}
}

void AssignPlayerDialog::setToCreateMode() {
	if (!createMode) {
		mainSizer->Show(newPlayerValuesSizer, true, true);
		mainSizer->Layout();
		this->SetSizerAndFit(mainSizer);
		createMode = true;
	}
}

std::string AssignPlayerDialog::getAliasToAssignTo() {
	if (aliasChoice->GetStringSelection().ToStdString() == "<New player>") { return ""; }
	return aliasChoice->GetStringSelection().ToStdString();
}

double AssignPlayerDialog::getRating() const
{
	if (!createMode) {
		return -1;
	}
	return ratingVal->GetValue();
}

double AssignPlayerDialog::getDeviation() const
{
	if (!createMode) {
		return -1;
	}
	return deviationVal->GetValue();
}

double AssignPlayerDialog::getVolatility() const
{
	if (!createMode) {
		return -1;
	}
	return volatilityVal->GetValue();
}

