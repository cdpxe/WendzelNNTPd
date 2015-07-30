/* An Open Source Administration GUI for the WendzelNNTPd Usenet Server.
 *
 * This software is based on the free GPLed Qt4 lib. I choosed Qt4
 * because
 *       - I hate Java
 *       - Gtk needs too much code
 *       - its portable
 *       - its Open Source and I can use it for any kind of free App
 *
 * WendzelNNTPGUI is distributed under the following license:
 *
 * Copyright (c) 2007-2009 Steffen Wendzel <steffen (at) ploetner-it (dot) de>
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

#include <QApplication>
#include <QFont>
#include <QLCDNumber>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>
#include <QMenuBar>
#include <QTabWidget>
#include <QMessageBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QCheckBox>
#include <QGridLayout>
#include <qlineedit.h>
#include <QLabel>
#include <QPoint>

#include <iostream>

#include <stdio.h>
#include <sqlite3.h>

#include "../../src/include/wendzelnntpdpath.h"

#define WENDZELNNTPGUIVERSION	"1.0.1"

using namespace std;

class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow(QWidget *parent = 0);
public slots:
	void about();
};

class ServerSettings : public QWidget
{
	Q_OBJECT
private:
	QListWidget *list;
	QCheckBox   *auth_chkbox;
	QCheckBox   *verbose_chkbox;
	QCheckBox   *anon_msgid_chkbox;
	QLineEdit   *port;
	QCheckBox   *xml_chkbox;
	QLineEdit   *xmlfile;
public:
	ServerSettings(QWidget *parent = 0);
public slots:
	/* read info from file */
	char *get_lineval(char *);
	void read_cfg();
	/* add/remove info from GUI */
	void addip();
	void delip();
	/* create new config file */
	void savecfg();
};

class Authentication : public QWidget
{
	Q_OBJECT
private:
	QListWidget *list;
public:
	Authentication(QWidget *parent = 0);
public slots:
	void addusr();
	void delusr();
};

class Newsgroups : public QWidget
{
	Q_OBJECT
private:
	QListWidget *list;
public:
	Newsgroups(QWidget *parent = 0);
public slots:
	void addgrp();
	void delgrp();
};

class Logging : public QWidget
{
	Q_OBJECT
private:
	QListWidget *list;
public:
	Logging(QWidget *parent = 0);
public slots:
	void clearlog();
};

class SQLite3
{
public:
	SQLite3();
	bool Exec(char *cmd, int (*cb)(void *, int, char **, char **), void *arg);
	bool		 opened;
private:
	sqlite3		*db;
	char		*sqlite_err_msg;
};

