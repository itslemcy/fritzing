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

#ifndef SVGFLATTENER_H
#define SVGFLATTENER_H

#include "svgfilesplitter.h"
#include <QMatrix>

class SvgFlattener : public SvgFileSplitter
{
public:
    SvgFlattener();

    void flattenChildren(QDomElement & element);
    void unRotateChild(QDomElement & element,QMatrix transform);

public:
    static bool hasTranslate(QDomElement & element);
    static bool hasRotate(QDomElement & element);

protected slots:
    void rotateCommandSlot(QChar command, bool relative, QList<double> & args, void * userData);

};

#endif // SVGFLATTENER_H
