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

#include "viewlayer.h"
#include "debugdialog.h"

qreal ViewLayer::zIncrement = 0.00001;  // 0.000000001;

QHash<ViewLayer::ViewLayerID, StringPair * > ViewLayer::names;


ViewLayer::ViewLayer(ViewLayerID viewLayerID, bool visible, qreal initialZ )
{
	m_viewLayerID = viewLayerID;
	m_visible = visible;
	m_action = NULL;
	m_nextZ = initialZ;
}

void ViewLayer::initNames() {
	if (names.count() == 0) {
		// xmlname, displayname
		names.insert(ViewLayer::Icon, new StringPair("icon", QObject::tr("Icon")));
		names.insert(ViewLayer::BreadboardBreadboard, new StringPair("breadboardBreadboard", QObject::tr("Breadboard")));
		names.insert(ViewLayer::Breadboard, new StringPair("breadboard", QObject::tr("Parts")));
		names.insert(ViewLayer::BreadboardWire,  new StringPair("breadboardWire", QObject::tr("Wires")));
		names.insert(ViewLayer::BreadboardRuler,  new StringPair("breadboardRuler", QObject::tr("Rulers")));

		names.insert(ViewLayer::Schematic,  new StringPair("schematic", QObject::tr("Parts")));
		names.insert(ViewLayer::SchematicWire,  new StringPair("schematicWire",QObject::tr("Nets")));
		names.insert(ViewLayer::SchematicRuler,  new StringPair("schematicRuler", QObject::tr("Rulers")));

		names.insert(ViewLayer::Board,  new StringPair("board", QObject::tr("Board")));
		names.insert(ViewLayer::Silkscreen,  new StringPair("silkscreen", QObject::tr("Silkscreen")));
		names.insert(ViewLayer::Copper0,  new StringPair("copper0", QObject::tr("Copper 0")));
		names.insert(ViewLayer::Copper1,  new StringPair("copper1", QObject::tr("Copper 1")));
		names.insert(ViewLayer::Soldermask,  new StringPair("soldermask",  QObject::tr("Solder mask")));
		names.insert(ViewLayer::Outline,  new StringPair("outline",  QObject::tr("Outline")));
		names.insert(ViewLayer::Vias, new StringPair("vias", QObject::tr("Vias")));
		names.insert(ViewLayer::Keepout, new StringPair("keepout", QObject::tr("Keep out")));
		names.insert(ViewLayer::Ratsnest, new StringPair("ratsnest", QObject::tr("Rat's nest")));
		names.insert(ViewLayer::Jumperwires, new StringPair("jumperwires", QObject::tr("Jumper wires")));
		names.insert(ViewLayer::PcbRuler,  new StringPair("pcbRuler", QObject::tr("Rulers")));

		names.insert(ViewLayer::UnknownLayer,  new StringPair("unknown", QObject::tr("Unknown Layer")));
	}

}

QString & ViewLayer::displayName() {
	return names[m_viewLayerID]->second;
}

void ViewLayer::setAction(QAction * action) {
	m_action = action;
}

QAction* ViewLayer::action() {
	return m_action;
}

bool ViewLayer::visible() {
	return m_visible;
}

void ViewLayer::setVisible(bool visible) {
	m_visible = visible;
	if (m_action) {
		m_action->setChecked(visible);
	}

}

qreal ViewLayer::nextZ() {
	qreal temp = m_nextZ;
	m_nextZ += zIncrement;
	return temp;
}

ViewLayer::ViewLayerID ViewLayer::viewLayerIDFromXmlString(const QString & viewLayerName) {
	QHashIterator<ViewLayerID, StringPair *> i(names);
    while (i.hasNext()) {
        i.next();
		if (viewLayerName.compare(i.value()->first, Qt::CaseInsensitive ) == 0) {
			return i.key();
		}
    }

	return ViewLayer::UnknownLayer;
}

ViewLayer::ViewLayerID ViewLayer::viewLayerIDFromString(const QString & viewLayerName) {
	QHashIterator<ViewLayerID, StringPair *> i(names);
    while (i.hasNext()) {
        i.next();
		if (viewLayerName.compare(i.value()->second, Qt::CaseInsensitive ) == 0) {
			return i.key();
		}
    }

	return ViewLayer::UnknownLayer;
}

ViewLayer::ViewLayerID ViewLayer::viewLayerID() {
	return m_viewLayerID;
}

qreal ViewLayer::incrementZ(qreal z) {
	return (z + zIncrement);
}

qreal ViewLayer::getZIncrement() {
	return zIncrement;
}

const QString & ViewLayer::viewLayerNameFromID(ViewLayerID viewLayerID) {
	StringPair * sp = names.value(viewLayerID);
	if (sp == NULL) return ___emptyString___;

	return sp->second;
}

const QString & ViewLayer::viewLayerXmlNameFromID(ViewLayerID viewLayerID) {
	StringPair * sp = names.value(viewLayerID);
	if (sp == NULL) return ___emptyString___;

	return sp->first;
}
