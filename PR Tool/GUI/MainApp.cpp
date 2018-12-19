#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "MainApp.h"
#include "MainWin.h"

// This class only starts our main GUI which is implemented in MainWin
wxIMPLEMENT_APP(MainApp);

bool MainApp::OnInit()
{
	MainWin* frame = new MainWin();
	frame->Show();
	return true;
}

int MainApp::OnExit() {
	return 1;
}