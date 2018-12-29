#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/spinctrl.h>
#include <wx/hyperlink.h>

#include "WinSetAbt.h"
#include "GlobalVars.h"

WinSetAbt::WinSetAbt(wxWindow* parent, wxWindowID winID, double rating, double deviation, double volatility, double tau)
	:wxPanel(parent, winID) {

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// -=========== Challonge Settings ==========-
	wxStaticBoxSizer* challSizerBox = new wxStaticBoxSizer(wxVERTICAL, this, wxString("Challonge Settings"));
	wxGridSizer* challGridSizer = new wxGridSizer(2, wxSize(winMinWidth / 8, -1));

	// ------------ API Key ------------
	wxStaticText* keyTxt = new wxStaticText(this, wxID_ANY, wxString("Developer API Key: "));
	keyVal = new wxTextCtrl(this, wxID_ANY);
	keyTxt->SetToolTip(wxString("Your challonge API Key"));
	keyVal->SetToolTip(wxString("Your challonge API Key"));

	challGridSizer->Add(keyTxt);
	challGridSizer->Add(keyVal);
		
	wxHyperlinkCtrl* apiInfoText = new wxHyperlinkCtrl(this, wxID_ANY, wxString("You can get/generate an API key here"), wxString("https://challonge.com/settings/developer"));

	challSizerBox->Add(challGridSizer, 1, wxEXPAND);
	challSizerBox->Add(apiInfoText);

	// <=========== Rating Calculation ==========>
	wxStaticBoxSizer* calcSizer = new wxStaticBoxSizer(wxVERTICAL, this, wxString("Rating Calculation"));

	// ------------ Include forfeits ------------
	forfeitCheck = new wxCheckBox(this, ID_SET_ABT_INC_BOX, wxString("Include forfeits in rating: "), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	forfeitCheck->SetToolTip(wxString("Include forfeited matches, when calculating rating"));

	calcSizer->AddSpacer(5);
	calcSizer->Add(forfeitCheck);
	calcSizer->AddSpacer(5);

	// -=========== Start values ==========-
	wxStaticBoxSizer* valSizerBox = new wxStaticBoxSizer(wxHORIZONTAL, this, wxString("Start values"));
	wxGridSizer* valGridSizer = new wxGridSizer(2, wxSize(winMinWidth / 8, -1));

	// ------------ Rating ------------
	wxStaticText* ratingTxt = new wxStaticText(this, wxID_ANY, wxString("Rating: "));
	ratingVal = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, -1), wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0, 2500, rating);
	ratingTxt->SetToolTip(wxString("The actual rating value"));
	ratingVal->SetToolTip(wxString("The actual rating value"));
	valGridSizer->Add(ratingTxt);
	valGridSizer->Add(ratingVal);

	// ------------ Deviation ------------
	wxStaticText* deviationTxt = new wxStaticText(this, wxID_ANY, wxString("Deviation: "));
	deviationVal = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, -1), wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0, 600, deviation);
	deviationTxt->SetToolTip(wxString("The higher, the more a single result influences the rating"));
	deviationVal->SetToolTip(wxString("The higher, the more a single result influences the rating"));
	valGridSizer->Add(deviationTxt);
	valGridSizer->Add(deviationVal);


	// ------------ Volatility ------------
	wxStaticText* volatilityTxt = new wxStaticText(this, wxID_ANY, wxString("Volatility: "));
	volatilityVal = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, -1), wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0.0, 0.1, volatility, 0.001);
	volatilityTxt->SetToolTip(wxString("The expected fluctuation in the rating, higher for a player that is involved in upsets a lot"));
	volatilityVal->SetToolTip(wxString("The expected fluctuation in the rating, higher for a player that is involved in upsets a lot"));
	valGridSizer->Add(volatilityTxt);
	valGridSizer->Add(volatilityVal);

	valSizerBox->Add(valGridSizer, 1, wxEXPAND);
	calcSizer->Add(valSizerBox, 0, wxEXPAND);

	// -=========== System constants ==========-
	wxStaticBoxSizer* constSizerBox = new wxStaticBoxSizer(wxHORIZONTAL, this, wxString("System Constant(s)"));
	wxGridSizer* constGridSizer = new wxGridSizer(2, wxSize(winMinWidth / 8, -1));

	// ------------ Tau ------------
	wxStaticText* tauTxt = new wxStaticText(this, wxID_ANY, wxString("Tau: "));
	tauVal = new wxSpinCtrlDouble(this, ID_SET_ABT_TAU_SPIN, wxEmptyString, wxDefaultPosition, wxSize(100, -1), wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0.2, 2.0,  tau, 0.01);
	tauTxt->SetToolTip(wxString("Constrains the change in volatility over time"));
	tauVal->SetToolTip(wxString("Constrains the change in volatility over time"));
	constGridSizer->Add(tauTxt);
	constGridSizer->Add(tauVal);

	constSizerBox->Add(constGridSizer, 0, wxEXPAND);
	calcSizer->Add(constSizerBox, 0, wxEXPAND);

	// -=========== About ==========-
	wxStaticBoxSizer* aboutSizer = new wxStaticBoxSizer(wxVERTICAL, this, wxString("About"));
	wxTextCtrl* aboutText = new wxTextCtrl(this, wxID_ANY, wxString(
		"The PR Tool is designed to create Power Rankings using the Glicko-2 algorithm by Mark E. Glickoman and importing results from challonge-brackets.\n\n"
		"GUI created using wxWidgets\n" 
		".json-file management done using JsonCpp\n"
		"Networking done using libcurl\n"
		"Glicko-2 API created by J.A.K.\n\n"
		"GGScore created by J.A.K."), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxBORDER_NONE);
	aboutSizer->Add(aboutText, 1, wxEXPAND);

	// Add the Sizers to main sizer
	mainSizer->Add(challSizerBox, 0, wxEXPAND);
	mainSizer->AddSpacer(5);
	mainSizer->Add(calcSizer, 0, wxEXPAND);
	mainSizer->AddSpacer(5);
	mainSizer->Add(aboutSizer, 1, wxEXPAND);

	SetSizer(mainSizer);
}

std::string WinSetAbt::getAPIKey() const
{
	return keyVal->GetValue().ToStdString();
}

bool WinSetAbt::getIncludeForfeits() const {
	return forfeitCheck->GetValue();
}

double WinSetAbt::getDefaultRating() const
{
	return ratingVal->GetValue();
}

double WinSetAbt::getDefaultDeviation() const
{
	return deviationVal->GetValue();
}

double WinSetAbt::getDefaultVolatility() const
{
	return volatilityVal->GetValue();
}

double WinSetAbt::getTau() const
{
	return tauVal->GetValue();
}

void WinSetAbt::setAPIKey(std::string newAPIKey)
{
	keyVal->SetValue(wxString(newAPIKey));
}

void WinSetAbt::setIncludeForfeits(bool include) {
	forfeitCheck->SetValue(include);
}

void WinSetAbt::setDefaultRating(double newRating) {
	ratingVal->SetValue(newRating);
}

void WinSetAbt::setDefaultDeviation(double newDeviation) {
	deviationVal->SetValue(newDeviation);
}

void WinSetAbt::setDefaultVolatility(double newVolatility) {
	volatilityVal->SetValue(newVolatility);
}

void WinSetAbt::setTau(double newTau) {
	tauVal->SetValue(newTau);
}