#include "Menu.h"

Menu::Menu()
{
	this->longest_title = 0;
	for (std::pair<std::string, std::string> repo : getReposFromConfigFile())
	{
		std::string session_name = repo.second != NO_ALIAS ? repo.second : repo.first;

		Menu::RepositorySession repository_session(session_name, repo.first);
		this->repositorySessions.push_back(repository_session);

		updateLongestTitle(session_name);
	}
}

Menu::~Menu()
{
	// Delete Session objects
	for (auto repository_session : this->repositorySessions)
	{
		if (repository_session.session != nullptr)
		{
			delete repository_session.session;
		}
	}
	// Stop Curses window
	endwin();
}

void Menu::printTitle()
{
	printw("%s\n", TACO_TITLE.c_str());
}

void Menu::sortRepositorySessions()
{
	auto sortByStatus = [](const RepositorySession& rs1, const RepositorySession& rs2) {
		return (rs1.is_active && !rs2.is_active);
	};
	std::sort(this->repositorySessions.begin(), this->repositorySessions.end(), sortByStatus);
}

void Menu::printMenu(WINDOW *menu_win, int highlight)
{
	sortRepositorySessions(); // Put active sessions on top

	int x, y, i;

	x = 2;
	y = 2;
	box(menu_win, 0, 0);
	mvwprintw(menu_win, 0, 2, "%s", "MY REPOSITORIES");
	for (i = 0; i < this->repositorySessions.size(); ++i)
	{
		if (highlight == i) /* High light the present choice */
		{
			wattron(menu_win, A_REVERSE);
			mvwprintw(menu_win, y, x, "%s", getSessionNameWithStatus(this->repositorySessions[i]).c_str());
			wattroff(menu_win, A_REVERSE);
		}
		else
			mvwprintw(menu_win, y, x, "%s", getSessionNameWithStatus(this->repositorySessions[i]).c_str());
		++y;
	}
	wrefresh(menu_win);
}

void Menu::handleSelection(int selection)
{
	Menu::RepositorySession& selected_repository = this->repositorySessions[selection];

	if (this->repositorySessions[selection].session == nullptr)
	{
		// If the repository is not currently associated with a sesion,
		// then create a new session and mark it as active
		const char *alias = selected_repository.session_name.c_str();
		const char *path = selected_repository.path.c_str();

		Session *session = new Session(alias, path);

		selected_repository.session = session;
		selected_repository.is_active = true;
	}
	else
	{
		// Otherwise attach to the already existing session
		selected_repository.session->attach();
	}
}

/*
This method is ugly, needs cleanup in the future
*/
void Menu::openMenu()
{
	WINDOW *menu_win;
	int highlight = 0;
	int choice = -1;
	int c;

	initscr();
	start_color();
	clear();
	noecho();
	cbreak(); // Line buffering disabled. pass on everything
	int startx = (80 - 30) / 2;
	int starty = (24 - 10) / 2;

	curs_set(false);
	menu_win = newwin(this->repositorySessions.size() + 3, 50, 7, 2);
	keypad(menu_win, TRUE);
	printTitle();
	refresh();

	printMenu(menu_win, highlight);

	while (1)
	{
		c = wgetch(menu_win);
		switch (c)
		{
		case 107:
		case 259:
			if (highlight == 0)
				highlight = this->repositorySessions.size() - 1;
			else
				--highlight;
			break;
		case 106:
		case 258:
			if (highlight == this->repositorySessions.size() - 1)
				highlight = 0;
			else
				++highlight;
			break;
		case 10:
		case 115:
			choice = highlight;
			break;
		case 27:
			nodelay(menu_win, true);
			break;
		default:
			refresh();
			break;
		}
		printMenu(menu_win, highlight);
		if (choice != -1) // User did a choice come out of the infinite loop
			break;
	}
	clrtoeol();
	refresh();
	endwin();

	// Attach to the session of the selected repository
	handleSelection(choice);
}
