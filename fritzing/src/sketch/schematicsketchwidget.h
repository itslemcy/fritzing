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

$Revision: 1617 $:
$Author: cohen@irascible.com $:
$Date: 2008-11-22 20:32:44 +0100 (Sat, 22 Nov 2008) $

********************************************************************/



#ifndef SCHEMATICSKETCHWIDGET_H
#define SCHEMATICSKETCHWIDGET_H

#include "pcbsketchwidget.h"

class SchematicSketchWidget : public PCBSketchWidget
{
	Q_OBJECT

public:
    SchematicSketchWidget(ViewIdentifierClass::ViewIdentifier, QWidget *parent=0);

	void addViewLayers();
	ViewLayer::ViewLayerID getWireViewLayerID(const ViewGeometry & viewGeometry);
	ViewLayer::ViewLayerID getDragWireViewLayerID();
	void initWire(Wire *, int penWidth);
	bool autorouteNeedsBounds();
	bool autorouteCheckWires();
	bool autorouteCheckConnectors();
	bool autorouteCheckParts();
	void tidyWires();
	void ensureTraceLayersVisible();
	void ensureTraceLayerVisible();
	void ensureJumperLayerVisible();
	void setJumperFlags(ViewGeometry & vg);
	bool usesJumperItem();
	void setClipEnds(ClipableWire * vw, bool);
	void getBendpointWidths(class Wire *, qreal w, qreal & w1, qreal & w2);
	void getLabelFont(QFont &, QColor &);
	void setNewPartVisible(ItemBase *);
	bool canDropModelPart(ModelPart * modelPart); 
	bool includeSymbols();
	bool hasBigDots();
	void changeConnection(long fromID,
						  const QString & fromConnectorID,
						  long toID, const QString & toConnectorID,
						  bool connect, bool doEmit, bool seekLayerKin,
						  bool updateConnections);

public slots:
	void setVoltage(qreal voltage, bool doEmit);

protected slots:
	void updateBigDots();

protected:
	qreal getRatsnestOpacity(Wire *);
	AddItemCommand * newAddItemCommand(BaseCommand::CrossViewType crossViewType, 
										QString moduleID, ViewGeometry & viewGeometry, qint64 id, 
										bool updateInfoView, long modelIndex, long originalModelIndex, QUndoCommand *parent);


protected:
	QTimer m_updateDotsTimer;

};

#endif