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

#ifndef GRAPHUTILS_H
#define GRAPHUTILS_H

#include "../connectors/connectoritem.h"
#include "../routingstatus.h"

class GraphUtils
{

public:
	static bool chooseRatsnestGraph(const QList<ConnectorItem *> & equipotentials, ConnectorPairHash & result);
	static bool scoreOneNet(QList<ConnectorItem *> & partConnectorItems, RoutingStatus & routingStatus);

};

#endif
