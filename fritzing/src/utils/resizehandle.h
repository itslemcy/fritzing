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

$Revision: 2676 $:
$Author: cohen@irascible.com $:
$Date: 2009-03-21 03:10:39 +0100 (Sat, 21 Mar 2009) $

********************************************************************/

#ifndef RESIZEHANDLE_H
#define RESIZEHANDLE_H

#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

class ResizeHandle : public QObject, public QGraphicsPixmapItem 
{
Q_OBJECT

public:
	ResizeHandle(const QPixmap & pixmap, QGraphicsItem * parent = 0);
	~ResizeHandle();

	QPointF resizeOffset();
	void setResizeOffset(QPointF);
	qreal currentScale();

public slots:
	void zoomChangedSlot(qreal scale);

protected:
	void mousePressEvent ( QGraphicsSceneMouseEvent * event );
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);

signals:
	void mousePressSignal(QGraphicsSceneMouseEvent * event, ResizeHandle *);
	void zoomChangedSignal(qreal scale);

protected:
	QPointF m_resizeOffset;

};

#endif
