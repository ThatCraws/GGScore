#ifndef __WINSETABT_H
#define __WINSETABT_H

//forward declaration
class wxSpinCtrlDouble;

class WinSetAbt : public wxPanel
{
public:
	WinSetAbt(wxWindow* parent, wxWindowID winID = wxID_ANY,
		double rating = 1500.0, double deviation = 350.0, double volatility = 0.06, double tau = 0.5);

	std::string getAPIKey() const;
	bool getIncludeForfeits() const;
	double getDefaultRating() const;
	double getDefaultDeviation() const;
	double getDefaultVolatility() const;
	double getTau() const;

	void setAPIKey(std::string newAPIKey);
	void setIncludeForfeits(bool include);
	void setDefaultRating(double newRating);
	void setDefaultDeviation(double newDeviation);
	void setDefaultVolatility(double newVolatility);
	void setTau(double newTau);

private:
	wxTextCtrl* keyVal;
	wxCheckBox* forfeitCheck;
	wxSpinCtrlDouble* ratingVal;
	wxSpinCtrlDouble* deviationVal;
	wxSpinCtrlDouble* volatilityVal;
	wxSpinCtrlDouble* tauVal;
};

#endif