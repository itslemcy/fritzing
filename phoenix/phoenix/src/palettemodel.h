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

#ifndef PALETTEMODEL_H
#define PALETTEMODEL_H

#include "modelpart.h"
#include "modelbase.h"

#include <QDomDocument>
#include <QList>
#include <QDir>
#include <QStringList>
#include <QHash>

class PaletteModel : public ModelBase
{
Q_OBJECT
public:
	PaletteModel();
	PaletteModel(bool makeRoot, bool doInit=true);
	ModelPart * retrieveModelPart(const QString & moduleID);
	void updateOrAddModelPart(const QString & moduleID, ModelPart *modelPart);
	virtual bool containsModelPart(const QString & moduleID);
	virtual ModelPart * loadPart(const QString & path, bool update=false);
	void clear();
	bool loadedFromFile();
	QString loadedFrom();
	bool load(const QString & fileName, ModelBase* refModel, bool doConnections);
	ModelPart * addPart(QString newPartPath, bool addToReference, bool updateIdAlreadyExists=false);

signals:
	void newPartLoaded(ModelPart *);

protected:
	QHash<QString, ModelPart *> partHash;
	bool m_loadedFromFile;
	QString m_loadedFrom; // The file this was loaded from, only if m_loadedFromFile == true

	bool m_loadingCore;

protected:
	virtual void init();
	void loadParts();
	void loadPartsAux(QDir & dir, QStringList & nameFilters);

	void writeAllPartsBinHeader();
	void writeAllPartsBinFooter();
	void writeInstanceInAllPartsBin(const QString &moduleID, const QString &path);
	void writeToAllPartsBinAux(const QString &textToWrite, QIODevice::OpenMode openMode);

	static bool CreateAllPartsBinFile;
	static QString AllPartsBinFilePath;

};
#endif
