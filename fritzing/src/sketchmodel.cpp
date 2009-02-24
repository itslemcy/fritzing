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

#include "sketchmodel.h"
#include "debugdialog.h"

#include <QDir>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QXmlStreamWriter>

SketchModel::SketchModel(bool makeRoot) : ModelBase(makeRoot)
{
}

SketchModel::SketchModel(ModelPart * root) : ModelBase(false) {
	m_root = root;
}

void SketchModel::removeModelPart(ModelPart * modelPart) {
	modelPart->setParent(NULL);
	//DebugDialog::debug(QString("model count %1").arg(root()->children().size()));
}

ModelPart * SketchModel::findModelPart(const QString & moduleID, long id) {
	if (m_root == NULL) return NULL;

	return findModelPartAux(m_root, moduleID, id);
}

ModelPart * SketchModel::findModelPartAux(ModelPart * modelPart, const QString & moduleID, long id) 
{
	if (modelPart->moduleID().compare(moduleID) == 0 && modelPart->hasViewID(id)) {
		return modelPart;
	}

	foreach (QObject * child, modelPart->children()) {
		ModelPart * mp = dynamic_cast<ModelPart *>(child);
		if (mp == NULL) continue;

		mp = findModelPartAux(mp, moduleID, id);
		if (mp != NULL) {
			return mp;
		}
	}

	return NULL;
}

bool SketchModel::paste(ModelBase * refModel, const QString & filename, QList<ModelPart *> & modelParts, long id) 
{
	QList<ModelPart *> already = m_loadedModelParts.values(id);
	if (already.size() > 0) {
		foreach (ModelPart * modelPart, already) {
			modelParts.append(modelPart);
		}
		return true;
	}

	QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
		return false;
	}

	QByteArray itemData = file.readAll();
	file.close();

	return ModelBase::paste(refModel, itemData, modelParts);
}