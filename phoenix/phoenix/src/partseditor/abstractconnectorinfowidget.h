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



#ifndef ABSTRACTCONNECTORINFOWIDGET_H_
#define ABSTRACTCONNECTORINFOWIDGET_H_

#include <QFrame>
#include <QFile>

class AbstractConnectorInfoWidget : public QFrame {
	Q_OBJECT
	public:
		AbstractConnectorInfoWidget(QWidget *parent=0);
		virtual void setSelected(bool selected, bool doEmitChange=true);
		bool isSelected();
		QSize sizeHint();

	signals:
		void tellSistersImNewSelected(AbstractConnectorInfoWidget*); // Meant to be used in the info context
		void tellViewsMyConnectorIsNewSelected(const QString&); // Meant to be used in the info context

	protected:
		void reapplyStyle();

		volatile bool m_isSelected;
};

#endif /* ABSTRACTCONNECTORINFOWIDGET_H_ */
