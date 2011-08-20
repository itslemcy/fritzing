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

$Revision: 4353 $:
$Author: cohen@irascible.com $:
$Date: 2010-07-12 10:57:33 +0200 (Mon, 12 Jul 2010) $

********************************************************************/

#ifndef GERBERGENERATOR_H
#define GERBERGENERATOR_H

#include <QString>

#include "../viewlayer.h"
#include "svg2gerber.h"

class GerberGenerator
{

public:
	static void exportToGerber(const QString & filename, const QString & exportDir, class ItemBase * board, class PCBSketchWidget *, bool displayMessageBoxes);

protected:
	static int doSilk(LayerList silkLayerIDs, const QString & silkName, const QString & gerberSuffix, ItemBase * board, PCBSketchWidget * sketchWidget, const QString & filename, const QString & exportDir, bool displayMessageBoxes);
	static int doMask(LayerList maskLayerIDs, const QString & maskName, const QString & gerberSuffix, ItemBase * board, PCBSketchWidget * sketchWidget, const QString & filename, const QString & exportDir, bool displayMessageBoxes);
	static int doCopper(ItemBase * board, PCBSketchWidget * sketchWidget, LayerList & viewLayerIDs, const QString & copperName, const QString & copperSuffix, const QString & filename, const QString & exportDir, bool displayMessageBoxes);
	static int doDrill(ItemBase * board, PCBSketchWidget * sketchWidget, const QString & filename, const QString & exportDir, bool displayMessageBoxes);
	static QString clipToBoard(QString svgString, ItemBase * board, const QString & layerName, SVG2gerber::ForWhy);
	static void displayMessage(const QString & message, bool displayMessageBoxes);

};

#endif // GERBERGENERATOR_H