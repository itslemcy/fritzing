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

$Revision$:
$Author$:
$Date$

********************************************************************/

#ifndef VIRTUALWIRE_H
#define VIRTUALWIRE_H

#include "wire.h"

class VirtualWire : public Wire
{

public:
	VirtualWire( ModelPart * modelPart, ItemBase::ViewIdentifier,  const ViewGeometry & , long id, QMenu* itemMenu  ); 
	
	void setHidden(bool hidden);
	void tempRemoveAllConnections();
	
protected:
	void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget );	
	void connectionChange(ConnectorItem *);
 	class FSvgRenderer * setUpConnectors(class ModelPart *, ItemBase::ViewIdentifier);
	void hideConnectors();	
};

#endif
