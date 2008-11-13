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

/*
 *  layerattributes.cpp
 *  Fritzing
 *
 *  Created by Jonathan Cohen on 9/20/08.
 *  Copyright 2008 FHP. All rights reserved.
 *
 */

#include "itemdrag.h"
#include "debugdialog.h"

ItemDrag * ItemDrag::singleton = new ItemDrag();

ItemDrag::ItemDrag(QObject * parent) :
	QObject(parent)
{
}

QHash<QObject *, QObject *> & ItemDrag::cache() {
	return m_cache;
}

void ItemDrag::dragIsDone() {
	m_cache.clear();
	emit dragIsDoneSignal(this);
}

QHash<QObject *, QObject *> & ItemDrag::_cache() {
	return singleton->m_cache;
}


ItemDrag * ItemDrag::_itemDrag() {
	return singleton;
}

void ItemDrag::_dragIsDone() {
	singleton->dragIsDone();
}
