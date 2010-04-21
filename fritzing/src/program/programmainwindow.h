/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2009 Fachhochschule Potsdam - http://fh-potsdam.de

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision$:
$Author$:
$Date$

********************************************************************/


#ifndef PROGRAMMAINWINDOW_H_
#define PROGRAMMAINWINDOW_H_

#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QFrame>
#include <QTextEdit>
#include <QProcess>
#include <QTabWidget>
#include <QComboBox>

#include "../fritzingwindow.h"

class PTabWidget : public QTabWidget 
{
	Q_OBJECT
public:
	PTabWidget(QWidget * parent);
	QTabBar * tabBar();
};

class ProgramMainWindow : public FritzingWindow
{
Q_OBJECT

public:
	ProgramMainWindow(QWidget *parent=0);
	~ProgramMainWindow();

	void setup();
	bool save();
	const QString defaultSaveFolder();

public:
	static void initText();

signals:
	void closed();
	void changeActivationSignal(bool activate, QWidget * originator);

protected slots:
	void parentAboutToClose();
	void addTab();

protected:
	bool saveAs();
	bool saveAsAux(const QString & fileName);
	void closeEvent(QCloseEvent *event);
	bool eventFilter(QObject *object, QEvent *event);

	QFrame * createHeader();
	QFrame * createCenter();

	const QString untitledFileName();
	const QString fileExtension();

	void updateSaveButton();

	void cleanUp();
	bool event(QEvent *);
	int &untitledFileCount();
	QStringList getSerialPorts();
	void setTitle();

protected:
	QPointer<PTabWidget> m_tabWidget;
	QPointer<QPushButton> m_addButton;
};

#endif /* PROGRAMMAINWINDOW_H_ */