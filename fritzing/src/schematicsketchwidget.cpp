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

#include "schematicsketchwidget.h"
#include "autorouter1.h"
#include "debugdialog.h"
#include "virtualwire.h"
#include <limits>

static int MAX_INT = std::numeric_limits<int>::max();

static const QString ___viewName___ = QObject::tr("Schematic View");

QHash <ConnectorItem *, int> distances;

bool distanceLessThan(ConnectorItem * end0, ConnectorItem * end1) {
	if (end0->connectorType() == Connector::Male && end1->connectorType() == Connector::Female) {
		return true;
	}
	if (end1->connectorType() == Connector::Male && end0->connectorType() == Connector::Female) {
		return false;
	}

	return distances.value(end0, MAX_INT) <= distances.value(end1, MAX_INT);
}

SchematicSketchWidget::SchematicSketchWidget(ItemBase::ViewIdentifier viewIdentifier, QWidget *parent)
    : PCBSchematicSketchWidget(viewIdentifier, parent)
{
}

void SchematicSketchWidget::setWireVisible(Wire * wire)
{
	wire->setVisible(wire->getRatsnest());
	//wire->setVisible(true);
}

void SchematicSketchWidget::addViewLayers() {
	addSchematicViewLayers();
}

void SchematicSketchWidget::updateRatsnestStatus(CleanUpWiresCommand * command, QUndoCommand * undoCommand) {
	Q_UNUSED(undoCommand);
	if (command) {
		QHash<ConnectorItem *, int> indexer;
		QList< QList<ConnectorItem *>* > allPartConnectorItems;
		Autorouter1::collectAllNets(this, indexer, allPartConnectorItems);
		removeRatsnestWires(allPartConnectorItems, command);
		foreach (QList<ConnectorItem *>* list, allPartConnectorItems) {
			delete list;
		}
	}
}

void SchematicSketchWidget::dealWithRatsnest(long fromID, const QString & fromConnectorID, 
								  long toID, const QString & toConnectorID,
								  bool connect, class RatsnestCommand * ratsnestCommand, bool doEmit)
{
	if (!connect) return;

	QString h1 = QString("%1.%2.%3.%4").arg(fromID).arg(fromConnectorID).arg(toID).arg(toConnectorID);
	DebugDialog::debug(QString("try hash 1 %1").arg(h1));
	ConnectorPair * cp = m_wireHash.value(h1);
	if (cp == NULL) {
		h1 = QString("%3.%4.%1.%2").arg(fromID).arg(fromConnectorID).arg(toID).arg(toConnectorID);
		DebugDialog::debug(QString("try hash 2 %1").arg(h1));
		cp = m_wireHash.value(h1);
	}
	if (cp != NULL) {
		m_wireHash.remove(h1);
		if (cp->connectorItem0 != NULL) {
			makeOneRatsnestWire(cp->connectorItem0, cp->connectorItem1, ratsnestCommand);
		}
		delete cp;
		return;
	}

	ConnectorItem * fromConnectorItem = NULL;
	ConnectorItem * toConnectorItem = NULL;
	if (dealWithRatsnestAux(fromConnectorItem, toConnectorItem, fromID, fromConnectorID, 
							toID, toConnectorID,
							connect, ratsnestCommand, doEmit)) 
	{
		return;
	}

	bool useFrom = false;
	bool useTo = false;
	if ((fromConnectorItem->attachedToItemType() == ModelPart::Part) ||
		(fromConnectorItem->attachedToItemType() == ModelPart::Board))
	{
		useFrom = true;
	}
	if ((toConnectorItem->attachedToItemType() == ModelPart::Part) ||
		(toConnectorItem->attachedToItemType() == ModelPart::Board))
	{
		useTo = true;
	}

	if (useFrom && useTo) {
		makeOneRatsnestWire(fromConnectorItem, toConnectorItem, ratsnestCommand);
		return;
	}

	if (useFrom && toConnectorItem->attachedToItemType() == ModelPart::Wire) {
		ConnectorItem * newTo = tryWire(toConnectorItem, fromConnectorItem);
		if (newTo != NULL) {
			makeOneRatsnestWire(fromConnectorItem, newTo, ratsnestCommand);
			return;
		}
	}
	else if (useTo && fromConnectorItem->attachedToItemType() == ModelPart::Wire) {
		ConnectorItem * newFrom = tryWire(fromConnectorItem, toConnectorItem);
		if (newFrom != NULL) {
			makeOneRatsnestWire(toConnectorItem, newFrom, ratsnestCommand);
			return;
		}
	}

	QList<ConnectorItem *> connectorItems;
	connectorItems << fromConnectorItem << toConnectorItem;
	ConnectorItem::collectEqualPotential(connectorItems);
	QList<ConnectorItem *> partsConnectorItems;
	ConnectorItem::collectParts(connectorItems, partsConnectorItems);
	if (useFrom) {
		ConnectorItem * newTo = tryParts(fromConnectorItem, toConnectorItem, partsConnectorItems);
		if (newTo != NULL) {
			makeOneRatsnestWire(fromConnectorItem, newTo, ratsnestCommand);
			return;
		}
	}
	else if (useTo) {
		ConnectorItem * newFrom = tryParts(toConnectorItem, fromConnectorItem, partsConnectorItems);
		if (newFrom != NULL) {
			makeOneRatsnestWire(toConnectorItem, newFrom, ratsnestCommand);
			return;
		}
	}
	else {
		for (int i = 0; i < partsConnectorItems.count() - 1; i++) {
			ConnectorItem * ci = partsConnectorItems[i];
			for (int j = i + 1; j < partsConnectorItems.count(); j++) {
				ConnectorItem * cj = partsConnectorItems[j];
				if (ci->bus() != NULL && ci->bus() == cj->bus()) continue;
				if (ci->wiredTo(cj, ViewGeometry::RatsnestFlag)) continue;

				if (alreadyOnBus(ci, cj)) continue;
				if (alreadyOnBus(cj, ci)) continue;

				makeOneRatsnestWire(ci, cj, ratsnestCommand);
				return;
			}
		}

		return;
	}
}

bool SchematicSketchWidget::alreadyOnBus(ConnectorItem * busCandidate, ConnectorItem * otherCandidate) {
	if (busCandidate->bus() == NULL) return false;

	foreach (ConnectorItem * toConnectorItem, otherCandidate->connectedToItems()) {
		if (toConnectorItem->bus() == busCandidate->bus()) return true;
	}

	return false;
}

ConnectorItem * SchematicSketchWidget::tryParts(ConnectorItem * otherConnectorItem, ConnectorItem * wireConnectorItem, QList<ConnectorItem *> partsConnectorItems)
{
	Q_UNUSED(wireConnectorItem);
	foreach (ConnectorItem * connectorItem, partsConnectorItems) {
		if (connectorItem == otherConnectorItem) continue;
		if (connectorItem->attachedTo() == otherConnectorItem->attachedTo() &&
			connectorItem->bus() != NULL &&
			connectorItem->bus() == otherConnectorItem->bus()) continue;
		if (alreadyOnBus(otherConnectorItem, connectorItem)) continue;
		if (alreadyOnBus(connectorItem, otherConnectorItem)) continue;


		return connectorItem;
	}

	return NULL;

}

ConnectorItem * SchematicSketchWidget::tryWire(ConnectorItem * wireConnectorItem, ConnectorItem * otherConnectorItem)
{
	QList<Wire *> chained;
	QList<ConnectorItem *> ends;
	QList<ConnectorItem *> uniqueEnds;
	dynamic_cast<Wire *>(wireConnectorItem->attachedTo())->collectChained(chained, ends, uniqueEnds);
	foreach (ConnectorItem * end, ends) {
		if (end == wireConnectorItem) continue;
		if (end == otherConnectorItem) continue;
		if (end->attachedToItemType() == ModelPart::Breadboard) continue;
		if (end->attachedTo() == otherConnectorItem->attachedTo() &&
			end->bus() != NULL &&
			end->bus() == otherConnectorItem->bus()) continue;
		if (alreadyOnBus(otherConnectorItem, end)) continue;
		if (alreadyOnBus(end, otherConnectorItem)) continue;


		return end;
	}

	return NULL;
}

void SchematicSketchWidget::makeWires(QList<ConnectorItem *> & partsConnectorItems, QList <Wire *> & ratsnestWires, Wire * & modelWire, RatsnestCommand * ratsnestCommand)
{
	Q_UNUSED(partsConnectorItems);
	Q_UNUSED(modelWire);
	Q_UNUSED(ratsnestWires);
	Q_UNUSED(ratsnestCommand);

	return;
}

bool SchematicSketchWidget::canDeleteItem(QGraphicsItem * item)
{
	VirtualWire * wire = dynamic_cast<VirtualWire *>(item);
	if (wire != NULL) return true;

	return SketchWidget::canDeleteItem(item);
}

bool SchematicSketchWidget::canCopyItem(QGraphicsItem * item)
{
	VirtualWire * wire = dynamic_cast<VirtualWire *>(item);
	if (wire != NULL) return false;

	return SketchWidget::canDeleteItem(item);
}

void SchematicSketchWidget::reviewDeletedConnections(QList<ItemBase *> & deletedItems, QHash<ItemBase *, ConnectorPairHash *> & deletedConnections, QUndoCommand * parentCommand)
{
	// don't forget to take the virtualwires out of the list:
	PCBSchematicSketchWidget::reviewDeletedConnections(deletedItems, deletedConnections, parentCommand);

	QSet<Wire *> deleteWires;
	QSet<Wire *> undeleteWires;
	ConnectorPairHash moveItems;
	foreach (ItemBase * itemBase, deletedItems) {
		Wire * wire = dynamic_cast<Wire *>(itemBase);
		if (wire == NULL) continue;
		if (!wire->getRatsnest()) continue;

		undeleteWires.insert(wire);
		m_deleteStash.append(wire);
		QList<Wire *> chained;
		QList<ConnectorItem *> ends;
		QList<ConnectorItem *> uniqueEnds;
		wire->collectChained(chained, ends, uniqueEnds);
		if (ends.count() < 2) continue;

		Wire * extraWire = NULL;
		for (int i = 0; i < ends.count() - 1; i++) {
			for (int j = i + 1; j < ends.count(); j++) {
				extraWire = ends[i]->wiredTo(ends[j], ViewGeometry::NormalFlag);
				if (extraWire != NULL) break;
			}
			if (extraWire != NULL) break;
		}
		if (extraWire != NULL) {
			// need to delete the real wire from breadboard view
			deleteWires.insert(extraWire);
			continue;
		}

		ConnectorItem * end0 = NULL;
		ConnectorItem * end1 = NULL;

		// if in earlier runs through this loop we've already chosen to move a particular part,
		// then choose to disconnect other connectors on the same part
		foreach (ConnectorItem * move, moveItems.uniqueKeys()) {
			foreach (ConnectorItem * end, ends) {
				if (end->attachedTo() == move->attachedTo()) {
					end0 = end;
					break;
				}
			}
			if (end0 != NULL) break;
		}

		distances.clear();
		QList<Wire *> distanceWires;
		foreach (ConnectorItem * end, ends) {
			int distance = calcDistance(wire, end, 0, distanceWires);
			distances.insert(end, distance);
		}
		qSort(ends.begin(), ends.end(), distanceLessThan);
		if (end0 == NULL) {
			end0 = ends[0];
			end1 = ends[1];
		}
		else {
			foreach (ConnectorItem * end, ends) {
				if (end != end0) {
					end1 = end;
					break;
				}
			}
		}
		moveItems.insert(end0, end1);
	}

	if (moveItems.count() > 0) {
		emit schematicDisconnectWireSignal(moveItems, deletedItems, deletedConnections, parentCommand);
	}

	foreach (Wire * wire, undeleteWires) {
		deletedItems.removeOne(wire);
	}

	foreach (Wire * wire, deleteWires) {
		if (!deletedItems.contains(wire)) {
			deletedItems.append(wire);
		}
	}
}

bool SchematicSketchWidget::canChainMultiple() {
	return true;
}


const QString & SchematicSketchWidget::viewName() {
	return ___viewName___;
}


void SchematicSketchWidget::modifyNewWireConnections(Wire * dragWire, ConnectorItem * fromOnWire, ConnectorItem * & fromConnectorItem, ConnectorItem * & toConnectorItem, QUndoCommand * parentCommand)
{
	// if needed, find or create a new breadboard to make the connection

	if (fromConnectorItem->attachedToItemType() == ModelPart::Wire) {
		fromConnectorItem = lookForBreadboardConnection(fromConnectorItem);		
		if (fromConnectorItem->attachedToItemType() == ModelPart::Breadboard) {
			// cache this for drawing the ratsnest wire back in the same place when we get the dealWithRatsnest call
			cacheWire(dragWire, fromOnWire, m_connectorDragConnector, fromConnectorItem, toConnectorItem, toConnectorItem);
		}
		else {
			// find an empty bus on a breadboard
			fromConnectorItem = lookForNewBreadboardConnection(fromConnectorItem);
			if (fromConnectorItem == NULL) return;

			// find the nearest part to m_connectorDragConnector
			Wire * wire = dynamic_cast<Wire *>(m_connectorDragConnector->attachedTo());
			QList<ConnectorItem *> ends;
			QList<ConnectorItem *> uniqueEnds;
			QList<Wire *> chained;
			wire->collectChained(chained, ends, uniqueEnds);
			if (ends.count() < 1) return;

			distances.clear();
			QList<Wire *> distanceWires;
			foreach (ConnectorItem * end, ends) {
				int distance = calcDistance(wire, end, 0, distanceWires);
				distances.insert(end, distance);
			}
			qSort(ends.begin(), ends.end(), distanceLessThan);

			// make a wire from that nearest part to the breadboard
			ConnectorItem * otherPartConnectorItem = toConnectorItem;
			toConnectorItem = ends[0];

			cacheWire(dragWire, fromOnWire, m_connectorDragConnector, fromConnectorItem, otherPartConnectorItem, toConnectorItem);

			// draw a wire from that bus on the breadboard to the other part
			ConnectorItem * otherPartBusConnectorItem = findEmptyBusConnectorItem(fromConnectorItem);
			long newID = ItemBase::getNextID();
			ViewGeometry viewGeometry;
			QPointF fromPos = otherPartBusConnectorItem->sceneAdjustedTerminalPoint();
			viewGeometry.setLoc(fromPos);
			QPointF toPos = otherPartConnectorItem->sceneAdjustedTerminalPoint();
			QLineF line(0, 0, toPos.x() - fromPos.x(), toPos.y() - fromPos.y());
			viewGeometry.setLine(line);

			new AddItemCommand(this, BaseCommand::CrossView, wire->modelPart()->moduleID(), viewGeometry, newID, true, -1, parentCommand);
			new ChangeConnectionCommand(this, BaseCommand::CrossView,
										newID, wire->connector0()->connectorStuffID(),
										otherPartBusConnectorItem->attachedToID(), otherPartBusConnectorItem->connectorStuffID(),
										true, true, parentCommand);
			new ChangeConnectionCommand(this, BaseCommand::CrossView,
										newID, wire->connector1()->connectorStuffID(),
										otherPartConnectorItem->attachedToID(), otherPartConnectorItem->connectorStuffID(),
										true, true, parentCommand);

			
			// figure out how to tie all that to one ratsnest wire

		}
	}
	else if (toConnectorItem->attachedToItemType() == ModelPart::Wire) {
		toConnectorItem = lookForBreadboardConnection(toConnectorItem);			// lookForBreadboardConnection may change toConnectorItem
		if (toConnectorItem->attachedToItemType() == ModelPart::Breadboard) {
			// cache this for drawing the ratsnest wire back in the same place
			//m_wireHash.insert(dragWire->id(), m_connectorDragConnector);
		}
		else {
		}
	}
}


ConnectorItem * SchematicSketchWidget::lookForBreadboardConnection(ConnectorItem * connectorItem) {
	Wire * wire = dynamic_cast<Wire *>(connectorItem->attachedTo());

	QList<ConnectorItem *> ends;
	QList<ConnectorItem *> uniqueEnds;
	QList<Wire *> wires;
	wire->collectChained(wires, ends, uniqueEnds);
	foreach (ConnectorItem * end, ends) {
		foreach (ConnectorItem * toConnectorItem, end->connectedToItems()) {
			if (toConnectorItem->attachedToItemType() == ModelPart::Breadboard) {
				return findEmptyBusConnectorItem(toConnectorItem);
			}
		}
	}

	ends.clear();
	ends.append(connectorItem);
	ConnectorItem::collectEqualPotential(ends);
	foreach (ConnectorItem * end, ends) {
		if (end->attachedToItemType() == ModelPart::Breadboard) {
			return findEmptyBusConnectorItem(end);
		}
	}

	return connectorItem;
}

ConnectorItem * SchematicSketchWidget::findEmptyBusConnectorItem(ConnectorItem * busConnectorItem) {
	Bus * bus = busConnectorItem->bus();
	if (bus == NULL) return busConnectorItem;

	QList<ConnectorItem *> connectorItems;
	busConnectorItem->attachedTo()->busConnectorItems(bus, connectorItems);
	foreach (ConnectorItem * connectorItem, connectorItems) {
		if (connectorItem == busConnectorItem) continue;

		if (connectorItem->connectionsCount() == 0) {
			return connectorItem;
		}
	}

	return busConnectorItem;
}

int SchematicSketchWidget::calcDistance(Wire * wire, ConnectorItem * end, int distance, QList<Wire *> & distanceWires) {
	distanceWires.append(wire);
	int d0 = calcDistanceAux(wire->connector0(), end, distance, distanceWires);
	if (d0 == distance) return d0;

	int d1 = calcDistanceAux(wire->connector1(), end, distance, distanceWires);

	return qMin(d0, d1);
}

int SchematicSketchWidget::calcDistanceAux(ConnectorItem * from, ConnectorItem * to, int distance, QList<Wire *> & distanceWires) {
	foreach (ConnectorItem * toConnectorItem, from->connectedToItems()) {
		if (toConnectorItem == to) return distance;
	}

	int result = MAX_INT;
	foreach (ConnectorItem * toConnectorItem, from->connectedToItems()) {
		if (toConnectorItem->attachedToItemType() != ModelPart::Wire) continue;

		Wire * w = dynamic_cast<Wire *>(toConnectorItem->attachedTo());
		if (distanceWires.contains(w)) continue;

		int temp = calcDistance(w, to, distance + 1, distanceWires);
		if (temp < result) {
			result = temp;
		}
	}

	return result;
}

void SchematicSketchWidget::removeRatsnestWires(QList< QList<ConnectorItem *>* > & allPartConnectorItems, CleanUpWiresCommand * command)
{
	if (m_deleteStash.count() > 0) {
		foreach(Wire * wire, m_deleteStash) {
			command->addWire(this, wire);
			deleteItem(wire, true, false);
		}
		m_deleteStash.clear();
		return;
	}

	PCBSchematicSketchWidget::removeRatsnestWires(allPartConnectorItems, command);
}

void SchematicSketchWidget::chainVisible(ConnectorItem * fromConnectorItem, ConnectorItem * toConnectorItem, bool connect) {
	if (fromConnectorItem->attachedToItemType() != ModelPart::Wire) return;
	if (toConnectorItem->attachedToItemType() != ModelPart::Wire) return;

	fromConnectorItem->setHidden(!connect);
	toConnectorItem->setHidden(!connect);
}

ConnectorItem * SchematicSketchWidget::lookForNewBreadboardConnection(ConnectorItem * connectorItem) {
	QList<ItemBase *> breadboards;
	foreach (QGraphicsItem * item, scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;

		if (itemBase->itemType() == ModelPart::Breadboard) {
			breadboards.append(itemBase);
			break;
		}
	}

	ConnectorItem * busConnectorItem = NULL;
	foreach (ItemBase * breadboard, breadboards) {
		busConnectorItem = findEmptyBus(breadboard);
		if (busConnectorItem != NULL) return busConnectorItem;
	}


	// addItem(breadboard)
	// busConnectorItem = findEmptyBus(breadboard);
	// return busConnectorItem

	return connectorItem;
}

ConnectorItem * SchematicSketchWidget::findEmptyBus(ItemBase * breadboard) {
	foreach (Bus * bus, breadboard->buses()) {
		QList<ConnectorItem *> busConnectorItems;
		breadboard->busConnectorItems(bus, busConnectorItems);
		bool allEmpty = true;
		foreach (ConnectorItem * busConnectorItem, busConnectorItems) {
			if (busConnectorItem->connectionsCount() > 0) {
				allEmpty = false;
				break;
			}
		}
		if (allEmpty && busConnectorItems.count() > 0) {
			return busConnectorItems[0];
		}
	}
	return NULL;
}

void SchematicSketchWidget::cacheWire(Wire * wire, ConnectorItem * fromOnWire, 
									  ConnectorItem * oldFromConnectorItem, ConnectorItem * newFromConnectorItem, 
									  ConnectorItem * oldToConnectorItem, ConnectorItem * newToConnectorItem) {
	ConnectorPair * cp = new ConnectorPair;
	cp->connectorItem0 = oldFromConnectorItem;
	cp->connectorItem1 = oldToConnectorItem;
	QString h1 = QString("%1.%2.%3.%4")
						.arg(wire->id()).arg(wire->otherConnector(fromOnWire)->connectorStuffID())
						.arg(newFromConnectorItem->attachedToID()).arg(newFromConnectorItem->connectorStuffID());
	m_wireHash.insert(h1, cp);
	DebugDialog::debug(QString("hashing h1 %1").arg(h1));
	cp = new ConnectorPair;
	cp->connectorItem1 = cp->connectorItem0 = NULL;
	QString h2 = QString("%1.%2.%3.%4")
						.arg(wire->id()).arg(fromOnWire->connectorStuffID())
						.arg(newToConnectorItem->attachedToID()).arg(newToConnectorItem->connectorStuffID());
	m_wireHash.insert(h2, cp);	
	DebugDialog::debug(QString("hashing h2 %1").arg(h2));
}
