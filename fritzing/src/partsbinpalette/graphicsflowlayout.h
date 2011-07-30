/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2010 Fachhochschule Potsdam - http://fh-potsdam.de

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


#ifndef GRAPHICSFLOWLAYOUT_H_
#define GRAPHICSFLOWLAYOUT_H_

#include <QGraphicsLayoutItem>
#include <QGraphicsLinearLayout>
#include <QRect>
#include <QWidgetItem>

class GraphicsFlowLayout : public QGraphicsLinearLayout {
	public:
		GraphicsFlowLayout(QGraphicsLayoutItem *parent, int spacing);
		void setGeometry(const QRectF &rect);
		int heightForWidth(int width);
		void clear();
		int indexOf(const QGraphicsWidget *item);

	protected:
		void widgetEvent(QEvent * e);
		int doLayout(const QRectF &rect);

		double m_lastWidth;
};

#endif /* GRAPHICSFLOWLAYOUT_H_ */
