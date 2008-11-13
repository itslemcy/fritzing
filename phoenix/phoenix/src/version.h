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

$Revision$:
$Author$:
$Date$

********************************************************************/

#ifndef VERSION_H
#define VERSION_H

#include <QString>

class Version {

public:
	static const QString & majorVersion();
	static const QString & minorVersion();
	static const QString & modifier();
	static const QString & revision();
	static const QString & versionString();

protected:
	Version();

protected:

	static QString m_majorVersion;
	static QString m_minorVersion;
	static QString m_svnRevision;
	static QString m_revision;
	static QString m_modifier;
	static QString m_versionString;
	static Version * m_singleton;

};

#endif
