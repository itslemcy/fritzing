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

#ifndef PALETTEITEMBASE_H
#define PALETTEITEMBASE_H

#include <QGraphicsSvgItem>
#include <QGraphicsSceneMouseEvent>
#include <QSet>

#include "../model/modelpart.h"
#include "itembase.h"

class LayerKinPaletteItem;

class PaletteItemBase : public ItemBase
{
	Q_OBJECT

public:
	PaletteItemBase(ModelPart *, ViewIdentifierClass::ViewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu);

	void saveGeometry();
	bool itemMoved();
	virtual void saveInstanceLocation(QXmlStreamWriter &);
	void moveItem(ViewGeometry &);
	virtual void syncKinSelection(bool selected, PaletteItemBase *originator);
	virtual void syncKinMoved(QPointF offset, QPointF loc);
 	void blockItemSelectedChange(bool selected);
 	bool syncSelected();
 	QPointF syncMoved();
	void mousePressConnectorEvent(class ConnectorItem *, QGraphicsSceneMouseEvent *);
	bool acceptsMousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
 	virtual bool setUpImage(ModelPart* modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const LayerHash & viewLayers, ViewLayer::ViewLayerID, ViewLayer::ViewLayerSpec, bool doConnectors, QString & error);
	void connectedMoved(ConnectorItem * from, ConnectorItem * to);
	bool collectFemaleConnectees(QSet<ItemBase *> & items);
	void collectWireConnectees(QSet<class Wire *> & wires);
	virtual void mousePressEvent(PaletteItemBase * originalItem, QGraphicsSceneMouseEvent *event);
	//bool isBuriedConnectorHit(QGraphicsSceneMouseEvent *event);
	QRectF boundingRect() const;
	void setSharedRendererEx(class FSvgRenderer *);
	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, qreal dpi);
	virtual bool canEditPart();
	QPainterPath shape() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget);
	void setProp(const QString & prop, const QString & value);


	/*
	// for debugging
	void setPos(const QPointF & pos);
	void setPos(qreal x, qreal y);
	*/

protected:
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void setUpConnectors(FSvgRenderer *, bool ignoreTerminalPoints);
	void findConnectorsUnder();
	void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
	virtual bool canFindConnectorsUnder();

	virtual LayerKinPaletteItem * newLayerKinPaletteItem(
		PaletteItemBase * chief, ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier,
		const ViewGeometry & viewGeometry, long id,ViewLayer::ViewLayerID, ViewLayer::ViewLayerSpec, QMenu* itemMenu, const LayerHash & viewLayers
	);

protected slots:
	void partPropertyEntry();

protected:
 	bool m_blockItemSelectedChange;
 	bool m_blockItemSelectedValue;
 	QPointF m_offset;
 	bool m_syncSelected;
 	QPointF m_syncMoved;
 	bool m_svg;
};


#endif
