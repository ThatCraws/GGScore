#ifndef __ADD_RESULT_DIALOG_H
#define __ADD_RESULT_DIALOG_H

// forward declaration(s)
class wxCalendarCtrl;

class AddResultDialog : public wxDialog {
public:
	AddResultDialog(wxWindow* parent, wxWindowID id, const wxString& title, const wxArrayString mainAliases);
	~AddResultDialog();

	wxString getPlayer1Alias();
	bool getP1Won();
	wxString getPlayer2Alias();
	bool getP2Won();
	bool isForfeit();
	bool isTie();
	wxDateTime getDate();
	wxString getDesc();
private:
	wxComboBox* player1;
	wxRadioButton* p1Won;
	wxComboBox* player2;
	wxRadioButton* p2Won;
	wxCheckBox* forfeit;
	wxCheckBox* tieBox;
	wxCalendarCtrl* datePick;
	wxTextCtrl* descEdit;

	void OnForfeitCheck(wxCommandEvent& event);
	void OnTieCheck(wxCommandEvent& event);

	// For the description field
	void OnDescEditFocus(wxFocusEvent& event);
	void OnDescEditUnfocus(wxFocusEvent& event);
};


#endif