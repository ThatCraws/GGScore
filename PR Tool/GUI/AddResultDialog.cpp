#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "AddResultDialog.h"

#include <wx/calctrl.h>

AddResultDialog::AddResultDialog(wxWindow* parent, wxWindowID id, const wxString& title, const wxArrayString mainAliases)
	:wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER) 
{
	// ------------ Main Sizer ------------ 
	wxBoxSizer* resSizer = new wxBoxSizer(wxVERTICAL);

	// -==========- Player and Winner selection -==========-
	wxBoxSizer* playerSelSizer = new wxBoxSizer(wxHORIZONTAL);

	// ------------ Player 1 Selection ------------ 

	wxBoxSizer* player1Sizer = new wxBoxSizer(wxVERTICAL);
	player1 = new wxComboBox(this, wxID_ANY, wxString("Player 1"), wxDefaultPosition, wxSize(100, -1), mainAliases);
	p1Won = new wxRadioButton(this, wxID_ANY, wxString("Won"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);

	player1Sizer->Add(player1, wxEXPAND);
	player1Sizer->Add(p1Won);

	// ------------ Player 2 Selection ------------ 
	wxBoxSizer* player2Sizer = new wxBoxSizer(wxVERTICAL);

	player2 = new wxComboBox(this, wxID_ANY, wxString("Player 2"), wxDefaultPosition, wxSize(100, -1), mainAliases);
	p2Won = new wxRadioButton(this, wxID_ANY, wxString("Won"));

	player2Sizer->Add(player2, wxEXPAND | wxALIGN_CENTRE_VERTICAL);
	player2Sizer->Add(p2Won);

	playerSelSizer->Add(player1Sizer);
	playerSelSizer->Add(player2Sizer);

	resSizer->Add(playerSelSizer, 0, wxCENTER);

	// -=========== Match settings ===========-
	wxBoxSizer* matchSetSizer = new wxBoxSizer(wxHORIZONTAL);
	// ------------ Forfeit checkbox ------------
	forfeit = new wxCheckBox(this, wxID_ANY, wxString("Win/loss by Forfeit"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	forfeit->SetValue(false);
	matchSetSizer->Add(forfeit);

	// ------------ Tie checkbox ------------
	tieBox = new wxCheckBox(this, wxID_ANY, wxString("Tie"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	tieBox->SetValue(false);
	matchSetSizer->Add(tieBox);

	resSizer->AddSpacer(5);
	resSizer->Add(matchSetSizer, 0, wxALIGN_CENTER);

	// ------------ Date Picker ------------ 

	datePick = new wxCalendarCtrl(this, wxID_ANY);

	resSizer->Add(datePick, 1, wxEXPAND);

	// ------------ Description ------------ 

	descEdit = new wxTextCtrl(this, wxID_ANY, wxString("Description (optional)"));
	descEdit->SetForegroundColour(wxColour(96, 96, 96));
	//descEdit->SetDefaultStyle(wxTextAttr(*wxRED));
	//descEdit->AppendText(wxString("Description (optional)"));


	resSizer->Add(descEdit, 0, wxEXPAND);

	// ------------ OK/Cancel Buttons ------------ 
	wxBoxSizer* naviSizer = new wxBoxSizer(wxHORIZONTAL);

	wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
	okBtn->SetDefault();
	wxButton* cancBtn = new wxButton(this, wxID_CANCEL, "Cancel");

	naviSizer->Add(okBtn);
	naviSizer->Add(cancBtn);

	resSizer->Add(naviSizer, 0, wxCENTER);

	SetSizerAndFit(resSizer);


	/*
		addRes -> resSizer	-> playerSelSizer	-> player1Sizer	-> player1
																-> p1Won
												-> player2Sizer	-> player2
																-> p2Won
							-> datePicker
							-> naviSizer		-> okBtn
												-> cancBtn
	*/

	Bind(wxEVT_CHECKBOX, &AddResultDialog::OnForfeitCheck, this, forfeit->GetId());
	Bind(wxEVT_CHECKBOX, &AddResultDialog::OnTieCheck, this, tieBox->GetId());
	// Description field
	descEdit->Bind(wxEVT_SET_FOCUS, &AddResultDialog::OnDescEditFocus, this);
	descEdit->Bind(wxEVT_KILL_FOCUS, &AddResultDialog::OnDescEditUnfocus, this);
}

AddResultDialog::~AddResultDialog()
{
	Unbind(wxEVT_CHECKBOX, &AddResultDialog::OnForfeitCheck, this, forfeit->GetId());
	Unbind(wxEVT_CHECKBOX, &AddResultDialog::OnTieCheck, this, tieBox->GetId());
	descEdit->Unbind(wxEVT_SET_FOCUS, &AddResultDialog::OnDescEditFocus, this);
	descEdit->Unbind(wxEVT_KILL_FOCUS, &AddResultDialog::OnDescEditUnfocus, this);
}

wxString AddResultDialog::getPlayer1Alias() {
	return player1->GetValue();
}

bool AddResultDialog::getP1Won() {
	return p1Won->GetValue();
}

wxString AddResultDialog::getPlayer2Alias() {
	return player2->GetValue();
}

bool AddResultDialog::getP2Won() {
	return p2Won->GetValue();
}

bool AddResultDialog::isForfeit() {
	return forfeit->GetValue();
}

bool AddResultDialog::isTie() {
	return tieBox->GetValue();
}

wxDateTime AddResultDialog::getDate() {
	return datePick->GetDate();
}

wxString AddResultDialog::getDesc() {
	if (descEdit->GetForegroundColour() == wxColour(96, 96, 96)) {
		return wxEmptyString;
	}
	else {
		return descEdit->GetValue();
	}
}

void AddResultDialog::OnForfeitCheck(wxCommandEvent& event) {
	if (forfeit->GetValue()) {
		tieBox->SetValue(false);
	}
	if (event.IsChecked()) {
		p1Won->Enable();
		p2Won->Enable();
	}
}

void AddResultDialog::OnTieCheck(wxCommandEvent& event) {
	if (tieBox->GetValue()) {
		forfeit->SetValue(false);
	}
	if (event.IsChecked()) {
		p1Won->Disable();
		p2Won->Disable();
	}
	else {
		p1Won->Enable();
		p2Won->Enable();
	}
}

void AddResultDialog::OnDescEditFocus(wxFocusEvent& event) {
	// checking for grey text-colour instead of "Description (optional)", so you can actually use "Description (optional)" as a description
	if (descEdit->GetForegroundColour() == wxColour(96, 96, 96)) {
		descEdit->Clear();
		descEdit->SetForegroundColour(*wxBLACK);
	}
	event.Skip();
}

void AddResultDialog::OnDescEditUnfocus(wxFocusEvent& event) {
	if (descEdit->GetValue() == wxEmptyString) {
		descEdit->SetForegroundColour(wxColour(96, 96, 96));
		descEdit->SetValue("Description (optional)");
	}
	event.Skip();
}