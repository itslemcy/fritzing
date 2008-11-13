/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-08 Fachhochschule Potsdam - http://fh-potsdam.de

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

$Revision: 1485 $:
$Author: cohen@irascible.com $:
$Date: 2008-11-13 12:08:31 +0100 (Thu, 13 Nov 2008) $

********************************************************************/

#ifndef FDOCKWIDGET_H
#define FDOCKWIDGET_H

#include <QDockWidget>
#include <QEvent>
#include <QSettings>

class FDockWidget : public QDockWidget
{
Q_OBJECT
public:
	FDockWidget(const QString & title, QWidget * parent = 0);
	//QSize sizeHint() const;

	void saveState();
	void restoreState();

protected:
	void changeEvent(QEvent *event);

signals:
	void dockChangeActivationSignal(FDockWidget *);

protected:
	bool m_state;
	QString m_name;
};
#endif
