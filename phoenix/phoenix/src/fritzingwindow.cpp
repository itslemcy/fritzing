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



#include <QFileInfo>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFileDialog>
#include <QUuid>
#include <QCryptographicHash>
#include <QIcon>

#include "fritzingwindow.h"
#include "debugdialog.h"
#include "misc.h"

//#include "lib/quazip/quazip.h"
//#include "lib/quazip/quazipfile.h"

const QString FritzingWindow::FritzingExtension = ".fz";
QString FritzingWindow::QtFunkyPlaceholder("[*]");  // this is some wierd hack Qt uses in window titles as a placeholder to setr the modified state
const QString FritzingWindow::CoreBinLocation = ":/resources/bins/bin.fz";

FritzingWindow::FritzingWindow(const QString &untitledFileName, int &untitledFileCount, QString fileExt, QWidget * parent, Qt::WFlags f)
	: QMainWindow(parent, f)
{
	// Let's set the icon
	this->setWindowIcon(QIcon(QPixmap(":resources/images/fritzing_icon.png")));

	m_fileName = untitledFileName;

	if(untitledFileCount > 1) {
		m_fileName += " " + QString::number(untitledFileCount);
	}
	m_fileName += fileExt;
	untitledFileCount++;

	setTitle();

	m_undoStack = new WaitPushUndoStack(this);
	connect(m_undoStack, SIGNAL(cleanChanged(bool)), this, SLOT(undoStackCleanChanged(bool)) );
}

void FritzingWindow::setTitle() {
	setWindowTitle(tr("%1 - %2")
		.arg(QFileInfo(m_fileName).fileName() + QtFunkyPlaceholder)
		.arg(tr("Fritzing")));
}

// returns true if the user wanted to save the file
bool FritzingWindow::save() {
	if (isEmptyFileName(m_fileName,untitledFileName())) {
		return saveAs();
	} else {
		saveAsAux(m_fileName);
		return true;
	}
}

bool FritzingWindow::isEmptyFileName(const QString &fileName, const QString &untitledFileName) {
	return (fileName.isEmpty() || fileName.isNull() || fileName.startsWith(untitledFileName));
}

bool FritzingWindow::saveAs() {
	DebugDialog::debug(tr("current path: %1").arg(QDir::currentPath()));
	QString fileExt;
	QString path;

	if(m_fileName == tr("Untitled Sketch.fz")){
		path = defaultSaveFolder() + "/" + m_fileName;
	}
	else if(m_fileName.isNull() || m_fileName.isEmpty()){
		path = defaultSaveFolder();
	}
	else {
		path = m_fileName;
	}
	DebugDialog::debug(tr("current file: %1").arg(m_fileName));
    QString fileName = QFileDialog::getSaveFileName(
						this,
                        tr("Choose a file name"),
                        path,
                        tr("Fritzing (*%1)").arg(fileExtension()),
                        &fileExt
                      );

    if (fileName.isEmpty()) return false; // Cancel pressed

    if(!alreadyHasExtension(fileName)) {
		fileExt = getExtFromFileDialog(fileExt);
		fileName += fileExt;
	}
    saveAsAux(fileName);
    return true;
}

void FritzingWindow::undoStackCleanChanged(bool isClean) {
	setWindowModified(!isClean);
}

void FritzingWindow::replicateDir(QDir srcDir, QDir targDir) {
	// copy all files from template source to new random temp dir
	QStringList files = srcDir.entryList();
	for(int i=0; i < files.size(); i++) {
		QFile tempFile(srcDir.path() + "/" +files.at(i));
		DebugDialog::debug("copying " + tempFile.fileName());
		QFileInfo fi(files.at(i));
		QString newFilePath = targDir.path() + "/" + fi.fileName();
		if(QFileInfo(tempFile.fileName()).isDir()) {
			QDir newTargDir = QDir(newFilePath);
			newTargDir.mkpath(newTargDir.absolutePath());
			newTargDir.cd(files.at(i));
			replicateDir(QDir(tempFile.fileName()),newTargDir);
		} else {
			tempFile.copy(newFilePath);
		}
	}
}

bool FritzingWindow::alreadyHasExtension(const QString &fileName) {
	// TODO: Make something preattier to manage all the supported formats at once
	return fileName.indexOf(FritzingExtension)  != -1 || fileName.indexOf(".pdf")  != -1 || fileName.indexOf(".ps")  != -1 || fileName.indexOf(".png")  != -1 || fileName.indexOf(".jpg")  != -1;
}

QString FritzingWindow::getRandText() {
	QString rand = QUuid::createUuid().toString();
	QString randext = QCryptographicHash::hash(rand.toAscii(),QCryptographicHash::Md4).toHex();
	return randext;
}

/**
 * Is assumed that the options of the possible extensions are defined this way:
 * <Description of the file type> (*.<extension>)
 */
QString FritzingWindow::getExtFromFileDialog(const QString &extOpt) {
	return extOpt.mid(
			extOpt.indexOf("*")+1,
			extOpt.indexOf(")")-extOpt.indexOf("(")-2);
}

bool FritzingWindow::beforeClosing() {
	if (this->isWindowModified()) {
     	QMessageBox::StandardButton reply;
     	reply = QMessageBox::question(this, tr("Save %1").arg(QFileInfo(m_fileName).baseName()),
                                     tr("Do you want to save the changes you made in the document %1? Your changes will be lost if you don't save them")
                                     .arg(QFileInfo(m_fileName).baseName()),
                                     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
     	if (reply == QMessageBox::Yes) {
     		return save();
    	} else if (reply == QMessageBox::No) {
     		return true;
        }
     	else {
         	return false;
        }
	} else {
		return true;
	}
}

bool FritzingWindow::createFolderAnCdIntoIt(QDir &dir, QString newFolder) {
	DebugDialog::debug("<<< creating folder "+newFolder+" into "+dir.path());
	if(!dir.mkdir(newFolder)) return false;
	if(!dir.cd(newFolder)) return false;

	return true;
}

void FritzingWindow::rmdir(const QString &dirPath) {
	QDir dir = QDir(dirPath);
	rmdir(dir);
}

void FritzingWindow::rmdir(QDir & dir) {
	DebugDialog::debug(tr("removing folder: %1").arg(dir.path()));

	QStringList files = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
	for(int i=0; i < files.size(); i++) {
		QFile tempFile(dir.path() + "/" +files.at(i));
		DebugDialog::debug(tr("removing folder inside original: %1").arg(tempFile.fileName()));
		if(QFileInfo(tempFile.fileName()).isDir()) {
			QDir dir = QDir(tempFile.fileName());
			rmdir(dir);
		} else {
			tempFile.remove(tempFile.fileName());
		}
	}
	dir.rmdir(dir.path());
}

bool FritzingWindow::createZipAndSave(const QDir &dirToCompress, const QString &filename) {
	/*
	DebugDialog::debug("<<< zipping "+dirToCompress.path()+" into "+filename);
	  QuaZip zip(filename);
	  if(!zip.open(QuaZip::mdCreate)) {
	    qWarning("testCreate(): zip.open(): %d", zip.getZipError());
	    return false;
	  }
	  QFileInfoList files=dirToCompress.entryInfoList();
	  QFile inFile;
	  QuaZipFile outFile(&zip);
	  char c;
	  foreach(QFileInfo file, files) {
	    if(!file.isFile()||file.fileName()==filename) continue;
	    inFile.setFileName(file.fileName());
	    if(!inFile.open(QIODevice::ReadOnly)) {
	    	DebugDialog::debug("<<< "+file.fileName());
	      qWarning("testCreate(): inFile.open(): %s", inFile.errorString().toLocal8Bit().constData());
	      return false;
	    }
	    if(!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(inFile.fileName(), inFile.fileName()))) {
	      qWarning("testCreate(): outFile.open(): %d", outFile.getZipError());
	      return false;
	    }
	    while(inFile.getChar(&c)&&outFile.putChar(c)){}
	    if(outFile.getZipError()!=UNZ_OK) {
	      qWarning("testCreate(): outFile.putChar(): %d", outFile.getZipError());
	      return false;
	    }
	    outFile.close();
	    if(outFile.getZipError()!=UNZ_OK) {
	      qWarning("testCreate(): outFile.close(): %d", outFile.getZipError());
	      return false;
	    }
	    inFile.close();
	  }
	  zip.close();
	  if(zip.getZipError()!=0) {
	    qWarning("testCreate(): zip.close(): %d", zip.getZipError());
	    return false;
	  }
	  */
	  return true;
}
