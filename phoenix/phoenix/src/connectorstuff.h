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

#ifndef CONNECTORSTUFF_H
#define CONNECTORSTUFF_H

#include <QString>
#include <QDomElement>
#include <QMultiHash>

#include "viewthing.h"
#include "itembase.h"
#include "connector.h"


struct SvgIdLayer {
	QString m_svgId;
	QString m_terminalId;
	ViewLayer::ViewLayerID m_viewLayerID;
};


class ConnectorStuff
{

public:
	ConnectorStuff();
	ConnectorStuff(const QDomElement & domElement);

	const QString & id();
	void setId(QString id);
	const QString & description();
	void setDescription(QString description);
	const QString & name();
	void setName(QString name);
	const QString & connectorTypeString();
	void setConnectorType(QString type);
	Connector::ConnectorType connectorType();

	const QMultiHash<ItemBase::ViewIdentifier,SvgIdLayer *> &pins();
	const QString pin(ItemBase::ViewIdentifier, ViewLayer::ViewLayerID viewLayerID);
	const QString terminal(ItemBase::ViewIdentifier viewId, ViewLayer::ViewLayerID viewLayerID);
	const SvgIdLayer * fullPinInfo(ItemBase::ViewIdentifier viewId, ViewLayer::ViewLayerID viewLayerID);
	void addPin(ItemBase::ViewIdentifier layer, QString connectorId, ViewLayer::ViewLayerID, QString terminalId);
	void removePins(ItemBase::ViewIdentifier layer);

	ViewThing * viewThing();
	void setViewThing(ViewThing *);
	class BusStuff * bus();
	void setBus(class BusStuff *);
	const QString & busID();

protected:
	void loadPins(const QDomElement & domElement);
	void loadPin(QDomElement elem, ItemBase::ViewIdentifier viewId);

	QString m_description;
	QString m_id;
	QString m_name;
	QString m_typeString;
	Connector::ConnectorType m_type;

	/*
	 * The connectorId is used to generate the attribute svgId and terminalId in the FZ file,
	 * appending the words "pin" and "terminal" (respectively) to it
	 */
	QMultiHash<ItemBase::ViewIdentifier, SvgIdLayer*> m_pins;

	ViewThing * m_viewThing;
	class BusStuff * m_bus;
};

static QList<ConnectorStuff *> ___emptyConnectorStuffList___;

#endif
