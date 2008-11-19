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
								
#include "version.h"
											
#include <QString>
#include <QStringList>
		
QString Version::m_majorVersion("0");
QString Version::m_minorVersion("1");
QString Version::m_modifier("b");
QString Version::m_svnRevision("$Revision$:");
QString Version::m_revision;
QString Version::m_versionString;
Version * Version::m_singleton = new Version();
	
Version::Version() {
	m_revision = "";
	QStringList strings = m_svnRevision.split(" ", QString::SkipEmptyParts);
	if (strings.size() >= 2) {
		m_revision = strings[1];
	}

	m_versionString = QString("%1.%2.%3.%4").arg(m_majorVersion).arg(m_minorVersion).arg(m_modifier).arg(m_revision);
}

const QString & Version::majorVersion() {
	return m_majorVersion;
}

const QString & Version::minorVersion() {
	return m_minorVersion;
}

const QString & Version::revision() {
	return m_revision;
}

const QString & Version::modifier() {
	return m_modifier;
}

const QString & Version::versionString() {
	return m_versionString;
}

