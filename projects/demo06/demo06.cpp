#include "inkview.h"


// Seems like DialogSynchro() is blocking : code execution will only continue after
// a button has been clicked
static void ui_dialog_synchro_01()
{
	int result = DialogSynchro(ICON_QUESTION, "My Tile", "Content of the dialog", "1st button", "2nd button", NULL);
	char buffer[2048];
	snprintf(buffer, sizeof(buffer), "Result of DialogSynchro() = %d", result);
	Message(ICON_INFORMATION, "Debug", buffer, 3*1000);
}


static void *ui_simple_dialog_handler(int button)
{
	char buffer[2048];
	snprintf(buffer, sizeof(buffer), "Dialog: button %d has been selected!", button);
	Message(ICON_INFORMATION, "Debug", buffer, 3*1000);
	return NULL;
}

// Seems Dialog() is non-blocking : code execution will continue immediately
// and a callback will be called when a button is clicked
static void ui_dialog_01()
{
	const char *title = "This is a dialog";
	const char *text = "This is the text of the dialog";
	const char *button1 = "1st button!";
	const char *button2 = "And 2nd one ;-)";
	Dialog(ICON_INFORMATION, title, text, button1, button2, (iv_dialoghandler)ui_simple_dialog_handler);
	Message(ICON_INFORMATION, "Debug", "Dialog() is non-blocking ;-)", 3*1000);
}


static void menu_01_handler(int index)
{
	char buffer[2048];
	snprintf(buffer, sizeof(buffer), "Menu: index %d has been selected!", index);
	Message(ICON_INFORMATION, "Debug", buffer, 3*1000);
}

static void menu_01()
{
	static imenu submenu3 = {3, 3, (char *)"Text of entry 3 (type=3)", NULL};
	static imenu submenu2 = {2, 2, (char *)"Text of entry 2 (type=2)", &submenu3};
	static imenu submenu1 = {1, 1, (char *)"Text of entry 1 (type=1)", &submenu2};
	static imenu menu     = {1, 0, (char *)"Text of menu (type=1)",    &submenu1};

	OpenMenu(&menu, 0, 100, 200, (iv_menuhandler)menu_01_handler);
}



int progressbar_01_percent;

static void progressbar_01_timer()
{
	char buffer[2048];

	progressbar_01_percent += 5;
	snprintf(buffer, sizeof(buffer), "Progress is now %d%%", progressbar_01_percent);
	UpdateProgressbar(buffer, progressbar_01_percent);

	if (progressbar_01_percent < 100) {
		SetWeakTimer("My 1st timer", progressbar_01_timer, 250);
	}
	else {
		CloseProgressbar();
	}
}

static void progressbar_01_handler(int button)
{
	char buffer[2048];

	CloseProgressbar();

	snprintf(buffer, sizeof(buffer), "Progress: button %d has been selected!\nStopping progress!", button);
	Message(ICON_INFORMATION, "Debug", buffer, 3*1000);
}

static void progressbar_01()
{
	const char *title = "Title of progress";
	const char *text = "There is a lot of progress going on, here!";
	progressbar_01_percent = 0;

	OpenProgressbar(ICON_WARNING, title, text, progressbar_01_percent, (iv_dialoghandler)progressbar_01_handler);

	SetWeakTimer("My 1st timer", progressbar_01_timer, 250);
}


static int main_handler(int event_type, int param_one, int param_two)
{
	int result = 0;

	static int step = 0;

	switch (event_type) {
	case EVT_INIT:
		break;
	case EVT_SHOW:
		break;
	case EVT_KEYPRESS:
		if (param_one == KEY_PREV) {
			CloseApp();
			return 1;
		}
		else if (param_one == KEY_NEXT) {
			if (step == 0) {
				Message(ICON_INFORMATION, "Debug", "Key right!\n(message will disappear after a while...)", 3*1000);
			}
			else if (step == 1) {
				ui_dialog_synchro_01();
			}
			else if (step == 2) {
				ui_dialog_01();
			}
			else if (step == 3) {
				menu_01();
			}
			else if (step == 4) {
				progressbar_01();
			}
			else {
				CloseApp();
			}

			step++;
			return 1;
		}

		break;
	case EVT_EXIT:
		break;
	default:
		break;
	}

	return result;
}


int main(int argc, char* argv[])
{
	InkViewMain(main_handler);
	return 0;
}
