// automatically generated - do not edit

#include "conf.h"
#include "uimenu.h"
#include "cli.h"
#include "defproto.h"

const struct Menu guitop;
const struct Menu menu_aaaa;
const struct Menu menu_aaag;
const char * arg_aaab[] = { "-ui", "skylight" };
const char * arg_aaac[] = { "-ui", "daylight" };
const char * arg_aaad[] = { "-ui", "flourescent" };
const char * arg_aaae[] = { "-ui", "tungsten" };
const char * arg_aaaf[] = { "-ui", "light bulb" };
const char * argv_empty[] = { "-ui" };

const struct Menu menu_aaaa = {
    "Color", &guitop, 0, {
	{ "skylight", MTYP_FUNC, (void*)ui_f_set_color_byname, sizeof(arg_aaab)/4, arg_aaab },
	{ "daylight", MTYP_FUNC, (void*)ui_f_set_color_byname, sizeof(arg_aaac)/4, arg_aaac },
	{ "flouresc", MTYP_FUNC, (void*)ui_f_set_color_byname, sizeof(arg_aaad)/4, arg_aaad },
	{ "tungsten", MTYP_FUNC, (void*)ui_f_set_color_byname, sizeof(arg_aaae)/4, arg_aaae },
	{ "lt.bulb", MTYP_FUNC, (void*)ui_f_set_color_byname, sizeof(arg_aaaf)/4, arg_aaaf },
	{}
    }
};
const struct Menu menu_aaag = {
    "Diag", &guitop, 0, {
	{ "temp", MTYP_FUNC, (void*)ui_f_testtemp, sizeof(argv_empty)/4, argv_empty },
	{ "color", MTYP_FUNC, (void*)ui_f_testcolor, sizeof(argv_empty)/4, argv_empty },
	{}
    }
};
const struct Menu guitop = {
    "Main Menu", &guitop, 0, {
	{ "power", MTYP_FUNC, (void*)ui_f_set_power_level, sizeof(argv_empty)/4, argv_empty },
	{ "kelvin", MTYP_FUNC, (void*)ui_f_set_white_balance, sizeof(argv_empty)/4, argv_empty },
	{ "color", MTYP_MENU, (void*)&menu_aaaa },
	{ "diag", MTYP_MENU, (void*)&menu_aaag },
	{}
    }
};
