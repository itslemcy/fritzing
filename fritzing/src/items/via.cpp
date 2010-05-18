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

#include "via.h"
#include "../utils/graphicsutils.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../svg/svgfilesplitter.h"
#include "../utils/textutils.h"
#include "../viewlayer.h"
#include "../connectors/connectoritem.h"
#include "../connectors/svgidlayer.h"


Via::Via( ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: Hole(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel)
{
}

Via::~Via() {
}

void Via::setBoth(const QString & holeDiameter, const QString & ringThickness) {
	if (this->m_viewIdentifier != ViewIdentifierClass::PCBView) return;

	QString svg = makeSvg(holeDiameter, ringThickness);
	if (m_renderer == NULL) {
		m_renderer = new FSvgRenderer(this);
	}

	QString setColor;
	QStringList noIDs;
	bool result = m_renderer->loadSvg(svg.toLatin1(), m_filename, noIDs, noIDs, "", "", true);
	if (result) {
		setSharedRenderer(m_renderer);
	}

	// there's only one connector
	foreach (Connector * connector, m_modelPart->connectors().values()) {
		if (connector == NULL) continue;

		connector->unprocess(m_viewIdentifier, m_viewLayerID);
		SvgIdLayer * svgIdLayer = connector->fullPinInfo(m_viewIdentifier, m_viewLayerID);
		if (svgIdLayer == NULL) continue;

		bool result = m_renderer->setUpConnector(svgIdLayer, false, true);
		if (!result) continue;

		foreach (QGraphicsItem * child, childItems()) {
			ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(child);
			if (connectorItem == NULL) continue;

			connectorItem->setRect(svgIdLayer->m_rect);
			connectorItem->setTerminalPoint(svgIdLayer->m_point);
			connectorItem->setWeirdOffset(svgIdLayer->m_weirdOffset);
			connectorItem->setRadius(svgIdLayer->m_radius, svgIdLayer->m_strokeWidth);
			connectorItem->attachedMoved();
			break;
		}
	}
}

QString Via::makeID() {
	return "connector0pin";
}

QPointF Via::ringThicknessRange() {
	QPointF p(.001, 1.0);
	return p;
}

QPointF Via::holeDiameterRange() {
	QPointF p(.001, 1.0);
	return p;
}

