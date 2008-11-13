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



#include <QMessageBox>
#include <QMenu>

#include "partseditorpaletteitem.h"
#include "partseditorconnectoritem.h"
#include "../rendererviewthing.h"
#include "../debugdialog.h"
#include "../layerattributes.h"
#include "../layerkinpaletteitem.h"

PartsEditorPaletteItem::PartsEditorPaletteItem(ModelPart * modelPart, ItemBase::ViewIdentifier viewIdentifier, StringPair *path, QString layer) :
	PaletteItem(modelPart, viewIdentifier, m_viewGeometry, ItemBase::getNextID(), NULL)
{
	createSvgFile(path->first+"/"+path->second);
	m_svgStrings = new StringTriple();

	m_svgStrings->first = layer;
	m_svgStrings->second = path->first;
	m_svgStrings->third = path->second;

	m_connectors = NULL;

	setAcceptHoverEvents(false);
	setSelected(false);
}

PartsEditorPaletteItem::PartsEditorPaletteItem(ModelPart * modelPart, QDomDocument *svgFile, ItemBase::ViewIdentifier viewIdentifier, StringPair *path, QString layer) :
	PaletteItem(modelPart, viewIdentifier, m_viewGeometry, ItemBase::getNextID(), NULL)
{
	m_svgfile = svgFile;
	m_svgStrings = new StringTriple();

	m_svgStrings->first = layer;
	m_svgStrings->second = path->first;
	m_svgStrings->third = path->second;

	m_connectors = NULL;

	setAcceptHoverEvents(false);
	setSelected(false);
}

PartsEditorPaletteItem::PartsEditorPaletteItem(ModelPart * modelPart, ItemBase::ViewIdentifier viewIdentifier) :
	PaletteItem(modelPart, viewIdentifier, m_viewGeometry, ItemBase::getNextID(), NULL)
{
	m_connectors = NULL;
	m_svgStrings = NULL;
}

void PartsEditorPaletteItem::createSvgFile(QString path) {
    m_svgfile = new QDomDocument();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    if (!m_svgfile->setContent(&file)) {
        file.close();
        return;
    }
    file.close();
}

void PartsEditorPaletteItem::writeXmlLocation(QXmlStreamWriter & /*streamWriter*/) {
	return;
}

void PartsEditorPaletteItem::writeXml(QXmlStreamWriter & streamWriter) {
	streamWriter.writeStartElement(names[m_viewIdentifier]->first);
	streamWriter.writeStartElement("layers");

		streamWriter.writeStartElement("layer");
		streamWriter.writeAttribute("layerId",ViewLayer::viewLayerXmlNameFromID(m_viewLayerID));
		streamWriter.writeAttribute("image",m_svgStrings->third);
		streamWriter.writeEndElement();
	foreach (LayerKinPaletteItem * lkpi, m_layerKin) {
		streamWriter.writeStartElement("layer");
		streamWriter.writeAttribute("layerId",ViewLayer::viewLayerXmlNameFromID(lkpi->viewLayerID()));
		streamWriter.writeAttribute("image",m_svgStrings->third);
		streamWriter.writeEndElement();
	}

	streamWriter.writeEndElement();
	streamWriter.writeEndElement();
}

const QList<Connector *> &PartsEditorPaletteItem::connectors() {
	if(!m_connectors) {
		m_connectors = new QList<Connector*>;
		foreach(Connector *conn, modelPart()->connectors().values()) {
			*m_connectors << conn;
		}
	}
	return *m_connectors;
}

void PartsEditorPaletteItem::setConnector(const QString &id, Connector *connector) {
	Q_UNUSED(id);
	Q_UNUSED(connector);
/*delete modelPart()->connectors()[id];
	modelPart()->connectors()[id] = conn;*/

	/*ConnectorItem * connectorItem = newConnectorItem(connector);

	RendererViewThing * viewThing = dynamic_cast<RendererViewThing *>(modelPartStuff()->viewThing());
	QSvgRenderer * renderer = viewThing->get((long)m_viewIdentifier);

	if (connector == NULL) return;

	QRectF connectorRect;
	QPointF terminalPoint;
	bool result = connector->setUpConnector(renderer, m_viewIdentifier, m_viewLayerID, connectorRect, terminalPoint, true);
	if (!result) return;*/

	//ConnectorItem * connectorItem = newConnectorItem(connector);

/*	for (int i = 0; i < childItems().count(); i++) {
		PartsEditorConnectorItem * connectorItem = dynamic_cast<PartsEditorConnectorItem *>(childItems()[i]);
		if (connectorItem == NULL) continue;

		if(!connectorItem->connector() || (connectorItem->connector() && connectorItem->connector()->connectorStuffID() == id)) {
			if(connectorItem->connector() && connectorItem->connector()->connectorStuffID() == id) {
				delete connectorItem->connector();
			}
			connectorItem->setConnector(conn);
		}
	}*/
}

bool PartsEditorPaletteItem::setUpImage(ModelPart * modelPart, ItemBase::ViewIdentifier viewIdentifier, const LayerHash & viewLayers, ViewLayer::ViewLayerID viewLayerID, bool doConnectors)
{
    ModelPartStuff * modelPartStuff = modelPart->modelPartStuff();
    if (modelPartStuff == NULL) return false;
    if (modelPartStuff->domDocument() == NULL) return false;

	modelPartStuff->setViewThing(new RendererViewThing());

	RendererViewThing * viewThing = dynamic_cast<RendererViewThing *>(modelPartStuff->viewThing());
	setViewLayerID(viewLayerID, viewLayers);
	QSvgRenderer * renderer = viewThing->get((long)m_viewIdentifier);
	if (renderer == NULL) {
    	renderer = new QSvgRenderer();
    	if (m_svgStrings == NULL) {
    		// TODO Mariano: Copied from paletteitembase::setUpImage (extract what's in common)
			LayerAttributes layerAttributes;
			if (modelPartStuff->domDocument() ) {
				bool result = layerAttributes.getSvgElementID(modelPartStuff->domDocument(), viewIdentifier, viewLayerID);
				if (!result) return false;
			}
    		QDir dir(modelPartStuff->path());			// is a path to a filename
			dir.cdUp();									// lop off the filename
			dir.cdUp();									// parts root
			StringPair tempPath;
			tempPath.first = dir.absolutePath() + "/" + PaletteItemBase::SvgFilesDir;
			tempPath.second = "%1/" + layerAttributes.filename();

			QStringList possibleFolders;
			possibleFolders << "core" << "contrib" << "user";
			bool gotOne = false;
			for(int i=0; i < possibleFolders.size(); i++) {
				if (QFileInfo( tempPath.first+"/"+tempPath.second.arg(possibleFolders[i]) ).exists()) {
					m_svgStrings = new StringTriple();
					m_svgStrings->first = layerAttributes.layerName();
					m_svgStrings->second = tempPath.first;
					m_svgStrings->third = tempPath.second.arg(possibleFolders[i]);
					gotOne = true;
					break;
				}
			}

			if(!gotOne) {
				//QMessageBox::information( NULL, QObject::tr("Fritzing"),
					//					 QObject::tr("The file %1 is not a Fritzing file (6).").arg(tempPath.arg(possibleFolders[0])));
				delete renderer;
				return false;
			}
    	}
		if (!renderer->load(m_svgStrings->second+(!m_svgStrings->third.isEmpty()?"/"+m_svgStrings->third:""))) {
			QMessageBox::information( NULL, QObject::tr("Fritzing"),
					QObject::tr("The file %1 is not a Fritzing file (6).").arg(m_svgStrings->second+"/"+m_svgStrings->third));
			delete renderer;
			return false;
		} else {
			createSvgFile(m_svgStrings->second+"/"+m_svgStrings->third);
		}

		viewThing->set(m_viewLayerID, renderer);
	}

	this->setZValue(this->z());

	this->setSharedRenderer(renderer);


	m_size = renderer->defaultSize();

	m_svg = true;

	if (doConnectors) {
		setUpConnectors(renderer, modelPartStuff->ignoreTerminalPoints());
	}

	return true;
}

StringPair* PartsEditorPaletteItem::svgFilePath() {
	return new StringPair(m_svgStrings->second, m_svgStrings->third);
}

void PartsEditorPaletteItem::setSvgFilePath(StringPair *sp) {
	m_svgStrings->second = sp->first;
	m_svgStrings->third = sp->second;
}

ConnectorItem* PartsEditorPaletteItem::newConnectorItem(Connector *connector) {
	return new PartsEditorConnectorItem(connector,this);
}

void PartsEditorPaletteItem::highlightConnectors(const QString &connId) {
	for (int i = 0; i < childItems().count(); i++) {
		PartsEditorConnectorItem * connectorItem = dynamic_cast<PartsEditorConnectorItem *>(childItems()[i]);
		if (connectorItem == NULL) continue;

		connectorItem->highlight(connId);
	}
}

void PartsEditorPaletteItem::removeFromModel() {
	if(m_modelPart) {
		m_modelPart->removeViewItem(this);
	}
}
