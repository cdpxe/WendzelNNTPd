/* An Open Source Administration GUI for the WendzelNNTPd Usenet Server.
 *
 * This software is based on the free GPLed Qt4 lib. I choosed Qt4
 * because
 *       - I don't like Java too much
 *       - Gtk needs too much code
 *       - its portable
 *       - its Open Source and I can use it for any kind of free App
 *
 * WendzelNNTPGUI is distributed under the following license:
 *
 * Copyright (c) 2007-2009 Steffen Wendzel <steffen(at)ploetner-it(dot)de>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gui.h"

/*=========================================================================== */
/* ====================        MISC                   ======================= */
/* ========================================================================== */

int
get_openfilelen(FILE *fp)
{
	int len;
	
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	return len;
}

/*=========================================================================== */
/* ====================        SQL METHODS            ======================= */
/* ========================================================================== */


SQLite3::SQLite3()
{
	if (sqlite3_open(DBFILE, &db)) {
		sqlite3_close(db);
		opened = false;
	}
	opened = true;
}

bool
SQLite3::Exec(char *cmd, int (*cb)(void *, int, char **, char **), void *arg)
{
	if (!opened) {
		fprintf(stderr, "No connection to SQLite database!!\n");
		return false;
	}
	if (sqlite3_exec(db, cmd, cb, arg, &sqlite_err_msg) != SQLITE_OK) {
		fprintf(stderr, "SQLite3 error: %s\n", sqlite_err_msg);
		QMessageBox::about(NULL, "Error!", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		return false;
	}
	return true;
}

/*=========================================================================== */
/* ====================        SERVER SETTINGS        ======================= */
/* ========================================================================== */

void
ServerSettings::addip()
{
	bool ok;
	QString ipaddr = QInputDialog::getText(this, tr("Add IP"),
			tr("Enter new IPv4/IPv6 address:"), QLineEdit::Normal, NULL, &ok);
	if (ok && !ipaddr.isEmpty()) {
		list->addItem(ipaddr);
	}
}

void
ServerSettings::delip()
{
	QListWidgetItem *item = list->currentItem();
	if (item) {
		list->takeItem(list->currentRow());
	}
}

#define Is_Valid_Char(c)     (((c >= 0x30 && c <= 0x39) /* number */		\
                             ||(c >= 0x41 && c <= 0x5a) /* upper case char */	\
                             ||(c >= 0x61 && c <= 0x7a) /* lower case char */	\
                             || c == ':' || c == '.'    /* IPv4/v6 addr notation */\
			     || c == '/' || c == '\\'	/* path stuff */	\
			     || c == ' '		/* C:\dokumente und e... */\
                              ) ? 1 : 0)

char *
ServerSettings::get_lineval(char *p)
{
	int i;
	char *val;
	
	for (i = 0; Is_Valid_Char(p[i]); i++) {
		/* just increment i ... */
	}
	i--;
	if (!(val = (char *) calloc(i + 2, sizeof(char)))) {
		QMessageBox::about(NULL, "Error!", "Not enough memory to create a string (0x81)!?");
		return NULL;
	}
	strncpy(val, p, i + 1);
	return val;
	
}

void
ServerSettings::read_cfg()
{
	FILE *fp;
	int filesize;
	int i;
	char *buf;
	char *ptr;
	char *val;
	bool in_comment, in_listen, in_port, in_xmlfile;
	
	if (!(fp = fopen(CONFIGFILE, "r"))) {
		QMessageBox::about(NULL, "Error!", "Unable to open " CONFIGFILE);
		return;
	}
	
	filesize = get_openfilelen(fp);
	if (!filesize) {
		fclose(fp);
		return;
	}
	
	if (!(buf = (char *) calloc(filesize + 1, sizeof(char)))) {
		QMessageBox::about(NULL, "Error!", "Not enough memory to load configuration file!?");
		fclose(fp);
		return;
	}
	

	/* If fread() was unsuccessful, the rest of this function will not
	 * bring any results. Win32 makes problems here (fread() always returns
	 * zero; I have to fix this.). I hate Windoze. */
	if (!fread(buf, filesize, 1, fp)) {
#ifndef __WIN32__
		QMessageBox::about(NULL, "Error!", "Unable to read " CONFIGFILE);
		fclose(fp);
		return;
#endif
	}
	
	for (i = 0, in_comment = in_listen = in_port = in_xmlfile = false; i < filesize; i++) {
		if (buf[i] == ';') {
			in_comment = true;
			continue;
		}
		if (in_comment) {
			if (buf[i] == '\n')
				in_comment = false;
			continue;
		}
		if (in_listen || in_port || in_xmlfile) {
			/* whitespace */
			if (buf[i] == '\t')
				continue;
			/* value */
			ptr = buf + i;
			val = get_lineval(ptr);
			if (val) {
				if (in_listen)
					list->addItem(val);
				else if (in_xmlfile)
					xmlfile->setText(val);
				else
					port->setText(val);
			}
			in_port = in_listen = in_xmlfile = false;
			free(val);
		}
		if (strncmp(buf + i, "listen", 6) == 0) {
			in_listen = true;
			i += strlen("listen");
			continue;
		}
		if (strncmp(buf + i, "port", 4) == 0) {
			in_port = true;
			i += strlen("port");
			continue;
		}
		if (strncmp(buf + i, "xml-file", 8) == 0) {
			in_xmlfile = true;
			i += strlen("xml-file");
			continue;
		}
		if (strncmp(buf + i, "use-authentication", strlen("use-authentication")) == 0) {
			auth_chkbox->setCheckState(Qt::Checked);
			i += strlen("use-authentication");
			continue;
		}
		if (strncmp(buf + i, "use-xml-output", strlen("use-xml-output")) == 0) {
			xml_chkbox->setCheckState(Qt::Checked);
			i += strlen("use-xml-output");
			continue;
		}
		if (strncmp(buf + i, "verbose-mode", strlen("verbose-mode")) == 0) {
			verbose_chkbox->setCheckState(Qt::Checked);
			i += strlen("verbose-mode");
			continue;
		}
		if (strncmp(buf + i, "enable-anonym-mids", strlen("enable-anonym-mids")) == 0) {
			anon_msgid_chkbox->setCheckState(Qt::Checked);
			i += strlen("enable-anonym-mids");
			continue;
		}
	}
	fclose(fp);
}

void
ServerSettings::savecfg()
{
	FILE *fp;
	int i;
	char s_header[] = "; This configuration file was generated by WendzelNNTPGUI\n\n";
	char s_listen[] = "listen ";
	char s_port[] = "port ";
	char s_use_auth[] = "use-authentication\n";
	char s_be_verbose[] = "verbose-mode\n";
	char s_use_anon_msgids[] = "enable-anonym-mids\n";
	char s_use_xml[] = "use-xml-output\n";
	char s_xmlfile[] = "xml-file ";
	char s_newline[] = "\n";
	
	if (!(fp = fopen(CONFIGFILE, "w+"))) {
		QMessageBox::about(NULL, "Error!", "Unable to write " CONFIGFILE);
		return;
	}
	fwrite(s_header, strlen(s_header), 1, fp);
	
	/* port */
	{
		QString p_text = port->text();
		std::string s = p_text.toStdString();
		fwrite(s_port, strlen(s_port), 1, fp);
		fwrite(s.c_str(), strlen(s.c_str()), 1, fp);
		fwrite(s_newline, strlen(s_newline), 1, fp);
	}

	/* use xml? */
	if (xml_chkbox->checkState() == Qt::Checked) {
		fwrite(s_use_xml, strlen(s_use_xml), 1, fp);
	}
	
	/* xml-file */
	{
		QString p_text = xmlfile->text();
		std::string s = p_text.toStdString();
		/* only write xml-file line if there is some data available */
		if (strlen(s.c_str()) != 0) {
			fwrite(s_xmlfile, strlen(s_xmlfile), 1, fp);
			fwrite(s.c_str(), strlen(s.c_str()), 1, fp);
			fwrite(s_newline, strlen(s_newline), 1, fp);
		}
	}
	
	/* use auth? */
	if (auth_chkbox->checkState() == Qt::Checked) {
		fwrite(s_use_auth, strlen(s_use_auth), 1, fp);
	}
	
	/* be verbose? */
	if (verbose_chkbox->checkState() == Qt::Checked) {
		fwrite(s_be_verbose, strlen(s_be_verbose), 1, fp);
	}
	
	/* use anonymous message IDs? */
	if (anon_msgid_chkbox->checkState() == Qt::Checked) {
		fwrite(s_use_anon_msgids, strlen(s_use_anon_msgids), 1, fp);
	}
	
	/* walk trough the listbox to write the 'listen' elements to the file */
	for (i = 0; i < list->count(); i++) {
		QListWidgetItem *item = list->item(i);
		if (item) {
			QString ip_text = item->text();
			std::string s = ip_text.toStdString();
			fwrite(s_listen, strlen(s_listen), 1, fp);
			fwrite(s.c_str(), strlen(s.c_str()), 1, fp);
			fwrite(s_newline, strlen(s_newline), 1, fp);
		}
	}
	
	fwrite(s_newline, strlen(s_newline), 1, fp);
	fclose(fp);
	QMessageBox::about(NULL, "Saved!", "Successfully wrote the config file!");
}

ServerSettings::ServerSettings(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *layout = new QVBoxLayout;
	QPushButton *addbtn = new QPushButton("Add IP to listen on");
	QPushButton *delbtn = new QPushButton("Remove selected IP");
	QPushButton *savebtn = new QPushButton("Save configuration");
	QGridLayout *ugrid = new QGridLayout;
	QGridLayout *ugrid_right = new QGridLayout;
	QGridLayout *dgrid = new QGridLayout;
	QGridLayout *dgrid2 = new QGridLayout;
	QLabel *ip_label = new QLabel("IP addresses to listen on: ");
	QLabel *port_label = new QLabel("Port to listen on: ");
	QLabel *xmlfile_label = new QLabel("XML output file (leave empty for default file): ");
	
	auth_chkbox = new QCheckBox("Activate Authentication");
	verbose_chkbox = new QCheckBox("Be Verbose");
	anon_msgid_chkbox = new QCheckBox("Use Anonymized Message IDs");
	xml_chkbox = new QCheckBox("Generate XML output");
	port = new QLineEdit();
	xmlfile = new QLineEdit();
	list = new QListWidget(this);
	
	connect(addbtn, SIGNAL(clicked()), this, SLOT(addip()));
	connect(delbtn, SIGNAL(clicked()), this, SLOT(delip()));
	connect(savebtn,SIGNAL(clicked()), this, SLOT(savecfg()));
	
	ugrid->addWidget(list, 1, 1);
	ugrid->addLayout(ugrid_right, 1, 2);
	
	ugrid_right->addWidget(addbtn, 1, 1);
	ugrid_right->addWidget(delbtn, 2, 1);
	
	dgrid->addWidget(port_label, 1, 1);
	dgrid->addWidget(port, 1, 2);
	dgrid->addWidget(auth_chkbox, 1, 3);
	dgrid->addWidget(verbose_chkbox, 1, 4);
	dgrid->addWidget(anon_msgid_chkbox, 2, 1);

	dgrid2->addWidget(xml_chkbox, 1, 1);
	dgrid2->addWidget(xmlfile_label, 2, 1);
	dgrid2->addWidget(xmlfile, 2, 2);
	
	layout->addWidget(ip_label);
	layout->addLayout(ugrid);
	layout->addLayout(dgrid);
	layout->addLayout(dgrid2);
	layout->addWidget(savebtn);
	setLayout(layout);
	
	read_cfg();
}

/*=========================================================================== */
/* ====================         USER AUTH             ======================= */
/* ========================================================================== */

static int
userlist_cb(void *list, int argc, char **argv, char **ColName)
{
	((QListWidget *)list)->addItem(argv[0]);
	return 0;
}

void
Authentication::addusr()
{
	SQLite3 sqlite;
	char cmd[1024] = { '\0' };
	bool ok;
	QString username = QInputDialog::getText(this, tr("Create a new Account"),
			tr("Username:"), QLineEdit::Normal, NULL, &ok);
	if (ok && !username.isEmpty()) {
		QString password = QInputDialog::getText(this, tr("Now enter the password for this new Username."),
			tr("Password:"), QLineEdit::PasswordEchoOnEdit, NULL, &ok);
		if (ok && !password.isEmpty()) {
			std::string user = username.toStdString();
			std::string pass = password.toStdString();
			
			/* first do some checks */
			if (strlen(user.c_str()) > 100) {
				QMessageBox::about(NULL, "Error!", "username too long (max. 100 chars).");
				return;
			}
			if (strlen(pass.c_str()) < 8) {
				QMessageBox::about(NULL, "Error!", "password too short (min. 8 chars).");
				return;
			}
			if (strlen(pass.c_str()) > 100) {
				QMessageBox::about(NULL, "Error!", "password too long (max. 100 chars).");
				return;
			}
			
			/* now, after all checks are done, add the user */
			snprintf(cmd, sizeof(cmd) - 1,
				"insert into users (name, password) values ('%s', '%s');",
				user.c_str(), pass.c_str());
			if (sqlite.Exec(cmd, NULL, NULL)) {
				/* add the new usr to the list if everything went fine */
				list->addItem(username);
				list->sortItems(Qt::AscendingOrder);
			}
		}
	}
}

void
Authentication::delusr()
{
	SQLite3 sqlite;
	char cmd[1024] = { '\0' };
	
	QListWidgetItem *item = list->currentItem();
	if (item) {
		QString q = item->text();
		std::string s = q.toStdString();
		snprintf(cmd, sizeof(cmd) - 1, "delete from users where name='%s';", s.c_str());
		if(sqlite.Exec(cmd, NULL, NULL)) {
			/* remove from list */
			list->takeItem(list->currentRow());
		}
	}
}

Authentication::Authentication(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *layout = new QVBoxLayout;
	QPushButton *addbtn = new QPushButton("Add");
	QPushButton *delbtn = new QPushButton("Delete");
	QLabel *auth_label = new QLabel("List of users that have access to this system: ");
	SQLite3 sqlite;
	char buf_selall[] = "select * from users;";
	
	list = new QListWidget(this);
	
	if (!sqlite.opened) {
		QMessageBox::about(this, "WendzelNNTPGUI", "Unable to Connect to SQLite3 Database.");
	} else {
		sqlite.Exec(buf_selall, userlist_cb, list);
	}
	
	list->sortItems(Qt::AscendingOrder);
	
	connect(addbtn, SIGNAL(clicked()), this, SLOT(addusr()));
	connect(delbtn, SIGNAL(clicked()), this, SLOT(delusr()));
	
	layout->addWidget(auth_label);
	layout->addWidget(list);
	layout->addWidget(addbtn);
	layout->addWidget(delbtn);
	setLayout(layout);
}

/*=========================================================================== */
/* ====================         NEWSGROUPS            ======================= */
/* ========================================================================== */


static int
nglist_cb(void *list, int argc, char **argv, char **ColName)
{
	((QListWidget *)list)->addItem(argv[1]);
	return 0;
}

void
Newsgroups::addgrp()
{
	SQLite3 sqlite;
	char cmd[1024] = { '\0' };
	bool ok;
	QString groupname = QInputDialog::getText(this, tr("Create a new Newsgroup"),
			tr("Newsgroup name:"), QLineEdit::Normal, NULL, &ok);
	if (ok && !groupname.isEmpty()) {
		std::string s = groupname.toStdString();
		snprintf(cmd, sizeof(cmd) - 1,
			"insert into newsgroups (name, pflag, high) values ('%s', 'y', '0');",
			s.c_str());
		if (sqlite.Exec(cmd, NULL, NULL)) {
			/* add the new grp to the list if everything went fine */
			list->addItem(groupname);
			list->sortItems(Qt::AscendingOrder);
		}
	}
}

void
Newsgroups::delgrp()
{
	SQLite3 sqlite;
	char cmd[1024] = { '\0' };
	
	QListWidgetItem *item = list->currentItem();
	if (item) {
		QString q = item->text();
		std::string s = q.toStdString();
		snprintf(cmd, sizeof(cmd) - 1, "delete from newsgroups where name='%s';", s.c_str());
		if(sqlite.Exec(cmd, NULL, NULL)) {
			/* remove from list */
			list->takeItem(list->currentRow());
		}
	}
}

Newsgroups::Newsgroups(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *layout = new QVBoxLayout;
	QPushButton *addbtn = new QPushButton("Add");
	QPushButton *delbtn = new QPushButton("Delete");
	QLabel *ng_label = new QLabel("Available newsgroups: ");
	SQLite3 sqlite;
	char buf_selall[] = "select * from newsgroups;";
	
	
	list = new QListWidget(this);
	
	if (!sqlite.opened) {
		QMessageBox::about(this, "WendzelNNTPGUI", "Unable to Connect to SQLite3 Database.");
	} else {
		sqlite.Exec(buf_selall, nglist_cb, list);
	}
	list->sortItems(Qt::AscendingOrder);
	
	connect(addbtn, SIGNAL(clicked()), this, SLOT(addgrp()));
	connect(delbtn, SIGNAL(clicked()), this, SLOT(delgrp()));
	
	layout->addWidget(ng_label);
	layout->addWidget(list);
	layout->addWidget(addbtn);
	layout->addWidget(delbtn);
	setLayout(layout);
}

/*=========================================================================== */
/* ====================         LOGGING               ======================= */
/* ========================================================================== */

void
Logging::clearlog()
{
	FILE *fp;
	
	if (!(fp = fopen(LOGFILE, "w+"))) {
		perror(LOGFILE);
		QMessageBox::about(NULL, "WendzelNNTPGUI", "Unable to open logfile for writing!");
		return;
	}
	fclose(fp);
	list->clear();
}

Logging::Logging(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *layout = new QVBoxLayout;
	list = new QListWidget(this);
	QPushButton *clrbtn = new QPushButton("Clear Logfile");
	QLabel *log_label = new QLabel("Latest reported logging messages: ");
	FILE *fp;
	char buf[1024] = { '\0' };
	
	if (!(fp = fopen(LOGFILE, "r"))) {
		QMessageBox::about(this, "WendzelNNTPGUI", "Unable to Open Logfile!");
		return;
	}
	while (fgets(buf, sizeof(buf) - 1, fp)) {
		buf[strlen(buf)-2] = '\0'; /* kill \r\n */
		list->addItem(buf);
		memset(buf, 0x0, sizeof(buf));
	}
	fclose(fp);
	
	connect(clrbtn, SIGNAL(clicked()), this, SLOT(clearlog()));
	
	layout->addWidget(log_label);
	layout->addWidget(list);
	layout->addWidget(clrbtn);
	setLayout(layout);
}

/*=========================================================================== */
/* ====================         MAIN STUFF            ======================= */
/* ========================================================================== */

void
MainWindow::about()
{
	QMessageBox::about(NULL, "WendzelNNTPGUI",
	"WendzelNNTPGUI\n\n"
	"An Open Source Administration GUI for the WendzelNNTPd Usenet Server.\n\n"
	"WendzelNNTPGUI is distributed under the following license:\n\n"
	"Copyright (c) 2007-2009 Steffen Wendzel <steffen(at)ploetner-it(dot)de>\n\n"
	"This program is free software; you can redistribute it and/or modify\n"
	"it under the terms of the GNU General Public License as published by\n"
	"the Free Software Foundation; either version 3 of the License, or\n"
	"(at your option) any later version.\n\n"
	"This program is distributed in the hope that it will be useful,\n"
	"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	"GNU General Public License for more details.\n\n"
	"You should have received a copy of the GNU General Public License\n"
	"along with this program.  If not, see <http://www.gnu.org/licenses/>.");
}


MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
	QMenuBar *menu = new QMenuBar();
	QMenu *filemenu, *helpmenu;
	QTabWidget *tabbar = new QTabWidget();
	QAction *act_quit = new QAction(this);
	QAction *act_about = new QAction(this);
	
	this->resize(600,400);

/* ========= Menu ========= */
	/* File Menu */
	filemenu = menu->addMenu(tr("&File"));
	/* item: "Quit" */
        filemenu->addAction(act_quit);
	act_quit->setText("Quit");
	connect(act_quit, SIGNAL(triggered()), qApp, SLOT(quit()));
	/* Help menu */
	helpmenu = menu->addMenu(tr("&Help"));
	/* item: "About" */
	helpmenu->addAction(act_about);
	act_about->setText("About");
	connect(act_about, SIGNAL(triggered()), this, SLOT(about()));


/* ========= TabBar ========= */
	Logging *tab_logging = new Logging(tabbar);
	Newsgroups *tab_groups = new Newsgroups(tabbar);
	Authentication *tab_auth = new Authentication(tabbar);
	ServerSettings *tab_servset = new ServerSettings(tabbar);
	
	tabbar->addTab(tab_servset, "Server Settings");
	tabbar->addTab(tab_auth, "Manage Users");
	tabbar->addTab(tab_groups, "Newsgroups");
	tabbar->addTab(tab_logging, "Logging");

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(menu);
	layout->addWidget(tabbar);
	setLayout(layout);
}

int
#ifdef __WIN32__
 main(int argc, char *argv[])
#else
 main()
#endif
{
#ifdef __WIN32__
	QApplication app(argc, argv);
#else
	char **progname;
	int num = 1;
	
	/* I want my own app name ... there's maybe a better way to do this
	 * but who cares ...
	 */
	progname = (char **) calloc(1, sizeof(char));
	*progname = (char *) calloc(0x10, sizeof(char));
	strncpy(*progname, "WendzelNNTP GUI", 0xf);
	
        QApplication app(num, progname);
#endif
        MainWindow mainwindow;
        mainwindow.show();
        return app.exec();
}

