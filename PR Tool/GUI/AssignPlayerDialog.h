#ifndef __ASSIGN_PLAYER_DIALOG_H
#define __ASSIGN_PLAYER_DIALOG_H

// forward declarations
class wxChoice;

class AssignPlayerDialog : public wxDialog {
public:
	AssignPlayerDialog(wxWindow* parent, wxWindowID id, const wxString& title, const std::string aliasToAssign, wxArrayString assignables,
		double defaultRating, double defaultDeviation, double defaultVolatility);

	/* ------------ getAliasToAssignTo ------------
	Returns the alias of the existing player this new Alias should be assigned to.
	If it is the alias of a new player it will be empty.

	Return:
		the player this new alias should be assigned to or "", if the alias belongs to a new player to be created.
	*/
	std::string getAliasToAssignTo();

	// getters for the start values of a newly created player, return -1 if in assignMode
	double getRating() const;
	double getDeviation() const;
	double getVolatility() const;
private:
	// Needed to transform to assign-/createMode
	wxBoxSizer* mainSizer;
	wxStaticBoxSizer* newPlayerValuesSizer;
	wxChoice* aliasChoice;

	wxSpinCtrlDouble* ratingVal;
	wxSpinCtrlDouble* deviationVal;
	wxSpinCtrlDouble* volatilityVal;

	void OnChoiceSelection(wxCommandEvent& event);

	void setToAssignMode();
	void setToCreateMode();
	bool createMode;
};

#endif