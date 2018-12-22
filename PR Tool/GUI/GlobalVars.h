#ifndef __CONTROL_IDS_H
#define __CONTROL_IDS_H

enum {
	// Rating periods tab
	ID_RAT_PER_ADD_BTN,
	ID_RAT_PER_REM_BTN,
	ID_RAT_PER_PLA_LIST,
	ID_RAT_PER_FIN_BTN,

	// Match report tab
	ID_MAT_REP_ADD_BTN,
	ID_MAT_REP_REM_BTN,
	ID_MAT_REP_IMP_BTN,
	ID_MAT_REP_RES_LIST,

	// Player edit tab
	ID_PLA_EDIT_PLA_CHOICE,
	ID_PLA_EDIT_ADD_ALIAS_BTN,
	ID_PLA_EDIT_REM_ALIAS_BTN,
	ID_PLA_EDIT_MAIN_ALIAS_BTN,
	ID_PLA_EDIT_HIDE_PLA_BTN,
	ID_PLA_EDIT_REM_BTN,

	// Settings/About tab
	ID_SET_ABT_INC_BOX
};

const wxString defaultFormatString = wxString("%d.%m.%Y");
const int winMinWidth = 400;
const int winMinHeight = 330;

#endif