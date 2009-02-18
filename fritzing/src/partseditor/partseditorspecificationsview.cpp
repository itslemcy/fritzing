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

$Revision$:
$Author$:
$Date$

********************************************************************/



#include <QFileDialog>
#include <QFrame>
#include <QBuffer>
#include <QSvgGenerator>
#include <QGraphicsProxyWidget>
#include <QMessageBox>
#include <QtDebug>

#include "partseditorspecificationsview.h"
#include "../layerkinpaletteitem.h"
#include "../debugdialog.h"
#include "../waitpushundostack.h"
#include "../fritzingwindow.h"
#include "../connectorshared.h"

QT_BEGIN_NAMESPACE

PartsEditorSpecificationsView::PartsEditorSpecificationsView(ItemBase::ViewIdentifier viewId, QDir tempDir, QGraphicsItem *startItem, QWidget *parent, int size)
	: PartsEditorAbstractView(viewId, tempDir, false/*true*/, parent, size)
{
	m_svgFilePath = new SvgAndPartFilePath;
	m_startItem = startItem;
	if(m_startItem) {
		addFixedToCenterItem(startItem);
		ensureFixedToCenterItems();
	}
}

void PartsEditorSpecificationsView::mousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *){
	DebugDialog::debug("got connector mouse press.  not yet implemented...");
	return;
}

void PartsEditorSpecificationsView::copySvgFileToDestiny() {
	Qt::CaseSensitivity cs = Qt::CaseSensitive;
#ifdef Q_WS_WIN
	cs = Qt::CaseInsensitive;
#endif

	// if the svg file is in the temp folder, then copy it to destiny
	if(m_svgFilePath->absolutePath().startsWith(m_tempFolder.absolutePath(),cs)) {
		QString origFile = svgFilePath();
		QString destFile = getApplicationSubFolderPath("parts")+"/svg/user/"+m_svgFilePath->relativePath();

		ensureFilePath(origFile);
		QFile tempFile(origFile);
		DebugDialog::debug(QString("copying from %1 to %2")
				.arg(origFile)
				.arg(destFile));
		tempFile.copy(destFile);
	}
}

void PartsEditorSpecificationsView::mousePressEvent(QMouseEvent *event) {
	Q_UNUSED(event);
	loadFile();
}

void PartsEditorSpecificationsView::loadFile() {
	QString origPath = QFileDialog::getOpenFileName(this,
		tr("Open Image"),
		m_originalSvgFilePath.isEmpty() ? getApplicationSubFolderPath("parts")+"/parts/svg/" : m_originalSvgFilePath,
		tr("SVG Files (*.svg);;JPEG (*.jpg);;PNG (*.png)"));

	if(origPath.isEmpty()) {
		return; // Cancel pressed
	} else {
		if(!origPath.endsWith(".svg")) {
			origPath = createSvgFromImage(origPath);
		}
		if(origPath != ___emptyString___) {
			if(m_startItem) {
				m_fixedToCenterItems.removeAll(m_startItem);
				scene()->removeItem(m_startItem);
				//delete m_startItem;
				m_startItem = NULL;
			}
			loadSvgFile(origPath);
		}
	}
}

void PartsEditorSpecificationsView::updateModelPart(const QString& origPath) {
	m_undoStack->push(new QUndoCommand("Dummy parts editor command"));

	setSvgFilePath(origPath);
	copyToTempAndRenameIfNecessary(m_svgFilePath);
	m_item->setSvgFilePath(m_svgFilePath);

	ModelPart *mp = createFakeModelPart(m_svgFilePath);
	m_item->setModelPart(mp);
}

void PartsEditorSpecificationsView::loadSvgFile(const QString& origPath) {
	m_undoStack->push(new QUndoCommand("Dummy parts editor command"));

	setSvgFilePath(origPath);

	ModelPart * mp = createFakeModelPart(m_svgFilePath);
	loadSvgFile(mp);
}

void PartsEditorSpecificationsView::loadSvgFile(ModelPart * modelPart) {
	addItemInPartsEditor(modelPart, m_svgFilePath);
	emit itemAddedToSymbols(modelPart, m_svgFilePath);

	copyToTempAndRenameIfNecessary(m_svgFilePath);

	m_item->setSvgFilePath(m_svgFilePath);
}

void PartsEditorSpecificationsView::loadFromModel(PaletteModel *paletteModel, ModelPart * modelPart) {
	if(m_startItem) {
		m_fixedToCenterItems.removeAll(m_startItem);
		m_startItem = NULL;
	}

	PartsEditorAbstractView::loadFromModel(paletteModel, modelPart);

	SvgAndPartFilePath *sp = m_item->svgFilePath();

	copyToTempAndRenameIfNecessary(sp);
	delete sp;
	m_item->setSvgFilePath(m_svgFilePath);

	emit loadedFromModel(paletteModel, modelPart);
}

void PartsEditorSpecificationsView::copyToTempAndRenameIfNecessary(SvgAndPartFilePath *filePathOrig) {
	m_originalSvgFilePath = filePathOrig->absolutePath();
	QString svgFolderPath = getApplicationSubFolderPath("parts")+"/svg";

	if(!filePathOrig->absolutePath().startsWith(svgFolderPath)) { // it's outside the parts folder
		DebugDialog::debug(QString("copying from %1").arg(m_originalSvgFilePath));
		QString viewFolder = ItemBase::viewIdentifierNaturalName(m_viewIdentifier);

		if(!m_tempFolder.mkdir(viewFolder)) return;
		if(!m_tempFolder.cd(viewFolder)) return;

		QString destFilePath = FritzingWindow::getRandText()+".svg";
		DebugDialog::debug(QString("dest file: %1").arg(m_tempFolder.absolutePath()+"/"+destFilePath));

		ensureFilePath(m_tempFolder.absolutePath()+"/"+destFilePath);

		QFile tempFile(m_originalSvgFilePath);
		tempFile.copy(m_tempFolder.absolutePath()+"/"+destFilePath);

		if(!m_tempFolder.cd("..")) return; // out of view folder

		m_svgFilePath->setRelativePath(viewFolder+"/"+destFilePath);
		m_svgFilePath->setAbsolutePath(m_tempFolder.absolutePath()+"/"+m_svgFilePath->relativePath());

	} else {
		QString relPathAux = filePathOrig->relativePath();
		m_svgFilePath->setAbsolutePath(m_originalSvgFilePath);
		m_svgFilePath->setRelativePath(
			relPathAux.right(// remove user/core/contrib
				relPathAux.size() -
				relPathAux.indexOf("/") - 1
			)
		);
	}
}

void PartsEditorSpecificationsView::setSvgFilePath(const QString &filePath) {
	ensureFilePath(filePath);
	m_originalSvgFilePath = filePath;

	QString svgFolder = getApplicationSubFolderPath("parts")+"/svg";
	QString tempFolder = m_tempFolder.absolutePath();

	QString relative;
	Qt::CaseSensitivity cs = Qt::CaseSensitive;
	QString filePathAux = filePath;

#ifdef Q_WS_WIN
	// seems to be necessary for Windows: getApplicationSubFolderPath() returns a string starting with "c:"
	// but the file dialog returns a string beginning with "C:"
	cs = Qt::CaseInsensitive;
#endif
	if(filePath.contains(svgFolder, cs)) {
		// is core file
		relative = filePathAux.remove(svgFolder+"/", cs);
	} else if(filePath.startsWith(tempFolder,cs)) {
		// is generated file that currently lives inside the temp folder
		relative = filePathAux.remove(tempFolder+"/", cs);
	} else {
		// generated jpeg/png
		relative = m_svgFilePath->relativePath();
	}

	delete m_svgFilePath;
	m_svgFilePath = new SvgAndPartFilePath(filePath,relative);
}


const QString PartsEditorSpecificationsView::svgFilePath() {
	return m_svgFilePath->absolutePath();
}

const SvgAndPartFilePath& PartsEditorSpecificationsView::svgFileSplit() {
	return *m_svgFilePath;
}

void PartsEditorSpecificationsView::fitCenterAndDeselect() {
	scene()->setSceneRect(0,0,width(),height());
	PartsEditorAbstractView::fitCenterAndDeselect();
}

QString PartsEditorSpecificationsView::createSvgFromImage(const QString &origFilePath) {
	QString viewFolder = getOrCreateViewFolderInTemp();

	QString newFilePath = m_tempFolder.absolutePath()+"/"+viewFolder+"/"+FritzingWindow::getRandText()+".svg";
	ensureFilePath(newFilePath);

/* %1=witdh in mm
 * %2=height in mm
 * %3=width in local coords
 * %4=height in local coords
 * %5=binary data
 */
/*	QString svgTemplate =
"<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
"	<svg width='%1mm' height='%2mm' viewBox='0 0 %3 %4' xmlns='http://www.w3.org/2000/svg'\n"
"		xmlns:xlink='http://www.w3.org/1999/xlink' version='1.2' baseProfile='tiny'>\n"
"		<g fill='none' stroke='black' vector-effect='non-scaling-stroke' stroke-width='1'\n"
"			fill-rule='evenodd' stroke-linecap='square' stroke-linejoin='bevel' >\n"
"			<image x='0' y='0' width='%3' height='%4'\n"
"				xlink:href='data:image/png;base64,%5' />\n"
"		</g>\n"
"	</svg>";

	QPixmap pixmap(origFilePath);
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	pixmap.save(&buffer,"png"); // writes pixmap into bytes in PNG format

	QString svgDom = svgTemplate
		.arg(pixmap.widthMM()).arg(pixmap.heightMM())
		.arg(pixmap.width()).arg(pixmap.height())
		.arg(QString("data:image/png;base64,%2").arg(QString(bytes.toBase64())));

	QFile destFile(newFilePath);
	if(!destFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::information(this, "", "file not created");
		if(!destFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
				QMessageBox::information(this, "", "file not created 2");
			}
	}
	QTextStream out(&destFile);
	out << svgDom;
	destFile.close();
	qDebug() << newFilePath;
	Q_ASSERT(QFileInfo(newFilePath).exists());
*/

	QImage imgOrig(origFilePath);

	QSvgGenerator svgGenerator;
	svgGenerator.setFileName(newFilePath);
    svgGenerator.setSize(imgOrig.size());
	QPainter svgPainter(&svgGenerator);
	svgPainter.drawImage(QPoint(0,0), imgOrig);
	svgPainter.end();

	return newFilePath;
}
