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

$Revision: 1617 $:
$Author: cohen@irascible.com $:
$Date: 2008-11-22 20:32:44 +0100 (Sat, 22 Nov 2008) $

********************************************************************/



#ifndef BREADBOARDSKETCHWIDGET_H
#define BREADBOARDSKETCHWIDGET_H

#include "sketchwidget.h"

class BreadboardSketchWidget : public SketchWidget
{
	Q_OBJECT

public:
    BreadboardSketchWidget(ItemBase::ViewIdentifier, QWidget *parent=0, int size=500, int minSize=100);

	void addViewLayers();
	const QString & viewName();


protected slots:
	void schematicDisconnectWireSlot(ConnectorPairHash & moveItems, QList<ItemBase *> & deletedItems, QUndoCommand * parentCommand);

protected:
	void cleanUpWire(Wire * wire, QList<Wire *> & wires);
	void collectFemaleConnectees(PaletteItem *);
	void findConnectorsUnder(ItemBase * item);
	bool disconnectFromFemale(ItemBase * item, QSet<ItemBase *> & savedItems, ConnectorPairHash &, QUndoCommand * parentCommand);
	BaseCommand::CrossViewType wireSplitCrossView();
	QPointF calcNewLoc(PaletteItemBase * moveBase, PaletteItemBase * detachFrom);
	bool shareBreadboard(ConnectorItem * fromConnectorItem, ConnectorItem * toConnectorItem);
};

#endif
