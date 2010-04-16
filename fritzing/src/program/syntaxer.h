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

$Revision: 4043 $:
$Author: cohen@irascible.com $:
$Date: 2010-03-19 13:06:00 +0100 (Fri, 19 Mar 2010) $

********************************************************************/


#ifndef SYNTAXER_H_
#define SYNTAXER_H_

#include <QDomDocument>
#include <QObject>

class Syntaxer : public QObject
{
Q_OBJECT

public:
	Syntaxer();
	~Syntaxer();

	static QString parseForName(const QString & filename);

	bool loadSyntax(const QString & filename);
};

#endif /* HIGHLIGHTER_H_ */
