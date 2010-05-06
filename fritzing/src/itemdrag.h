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



#ifndef ITEMDRAG_H
#define ITEMDRAG_H

#include <QDrag>
#include <QHash>
#include <QPixmap>


class ItemDrag : public QObject {
	
Q_OBJECT
	
	
protected:	
	ItemDrag(QObject * parent = 0);
	QHash<QObject *, QObject *> & cache();
	void dragIsDone();
	
public:
	static ItemDrag * _itemDrag();
	static QHash<QObject *, QObject *> & _cache();
	static void _dragIsDone();
	static void cleanup();

signals:
	void dragIsDoneSignal(ItemDrag *);

protected:
	QHash<QObject *, QObject *> m_cache;
	
protected:
	static ItemDrag * singleton;
};

#endif
