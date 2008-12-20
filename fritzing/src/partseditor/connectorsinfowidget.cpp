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


#include <QKeyEvent>
#include <QProgressDialog>
#include <QApplication>

#include "connectorsinfowidget.h"
#include "addremoveconnectorbutton.h"
#include "../debugdialog.h"
#include "../fritzingwindow.h"

#define MISMATCH_CONNS_HEADER tr("Mismatching Connector IDs")
#define MISMATCH_CONNS_FOOTER tr("These problems need to be fixed in the svg-files directly")

ConnectorsInfoWidget::ConnectorsInfoWidget(WaitPushUndoStack *undoStack, QWidget *parent) : QFrame(parent) {
	m_selected = NULL;
	m_undoStack = undoStack;

	createScrollArea();
	createToolsArea();

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(m_title,0,0);
	layout->addWidget(m_scrollArea,1,0);
	layout->addWidget(m_toolsContainter,2,0);
	layout->setContentsMargins(3, 10, 3, 10);

	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	setFocusPolicy(Qt::StrongFocus);

	installEventFilter(this);
}

void ConnectorsInfoWidget::emitPaintNeeded() {
	emit repaintNeeded();
}

void ConnectorsInfoWidget::createToolsArea() {
	m_toolsContainter = new QFrame(this);
	QHBoxLayout *lo = new QHBoxLayout(m_toolsContainter);

	AddRemoveConnectorButton *addBtn = new AddRemoveConnectorButton("Add",this);
	connect(addBtn, SIGNAL(clicked()), this, SLOT(addConnector()));

	AddRemoveConnectorButton *removeBtn = new AddRemoveConnectorButton("Remove",this);
	connect(removeBtn, SIGNAL(clicked()), this, SLOT(removeSelectedConnector()));

	lo->setMargin(2);
	lo->setSpacing(2);
	lo->addWidget(addBtn);
	lo->addWidget(removeBtn);
	lo->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding));

	m_showTerminalPointsCheckBox = new QCheckBox(this);
	m_showTerminalPointsCheckBox->setText(tr("Show Terminal Points"));
	connect(m_showTerminalPointsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(emitShowHideTerminalPoints(int)));

	lo->addWidget(m_showTerminalPointsCheckBox);
}

void ConnectorsInfoWidget::createScrollArea() {
	m_scrollContent = new QFrame();
	m_scrollContent->setObjectName("connInfoContent");
	m_scrollContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	m_mismatchersFrameParent = new QFrame(this);
	m_mismatchersFrameParent->setObjectName("mismatchConns");
	//m_mismatchersFrameParent->setFixedWidth(500);
	m_mismatchersFrameParent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

	QGridLayout *parentLo = new QGridLayout();
	parentLo->setMargin(1);
	m_mismatchersFrameParent->setLayout(parentLo);

	m_mismatchersFrame = new QFrame(m_mismatchersFrameParent);
	//m_mismatchersFrame->resize(this->width(),m_mismatchersFrame->height());
	QGridLayout *mismatchLayout = new QGridLayout();
	mismatchLayout->setMargin(0);
	mismatchLayout->setSpacing(0);
	m_mismatchersFrame->setLayout(mismatchLayout);
	m_mismatchersFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

	QLabel *mismatchConnHeader = new QLabel(MISMATCH_CONNS_HEADER);
	mismatchConnHeader->setObjectName("mismatchConnsHeader");
	parentLo->addWidget(mismatchConnHeader,0,0);

	parentLo->addWidget(m_mismatchersFrame,1,0);

	QLabel *mismatchConnFooter = new QLabel(MISMATCH_CONNS_FOOTER);
	mismatchConnFooter->setObjectName("mismatchConnsFooter");
	parentLo->addWidget(mismatchConnFooter,2,0);

	m_mismatchersFrameParent->hide();


	QGridLayout *scrollLayout = new QGridLayout(m_scrollContent);
	scrollLayout->setMargin(0);
	scrollLayout->setSpacing(0);
	scrollContentLayout()->addWidget(m_mismatchersFrameParent,0,0);

	m_scrollArea = new QScrollArea();
	m_scrollArea->setWidget(m_scrollContent);

	m_title = new QLabel("  "+tr("List of Connectors"));
	m_title->setObjectName("title");
}

void ConnectorsInfoWidget::selectionChanged(AbstractConnectorInfoWidget* selected) {
	if(m_selected) {
		m_selected->setSelected(false);
	}
	m_selected = selected;

	QTimer *timer = new QTimer(this);
	timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), this, SLOT(emitPaintNeeded()));
	timer->start(20);
}

void ConnectorsInfoWidget::setSelected(AbstractConnectorInfoWidget * newSelected) {
	newSelected->setSelected(true,false);
	selectionChanged(newSelected);
}

bool ConnectorsInfoWidget::eventFilter(QObject *obj, QEvent *event) {
	if(obj == this) {
		if(event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride) {
			QKeyEvent *keyEvent = (QKeyEvent*)event;
			if(keyEvent->key() == Qt::Key_Up) {
				selectPrev();
				return true;
			} else if(keyEvent->key() == Qt::Key_Down) {
				selectNext();
				return true;
			}
		}
	}
	return false;
}

/*void ConnectorsInfoWidget::ensureSelection(int i) {
	// TODO Mariano: Strange bug (if this is set, each up or dow key pressed event triggers twice)
	if(i==1) { // 1 == the fixed idx of the tab
		bool found = false;
		if(!m_selected) {
			if(m_connsInfo.size() > 0) {
				setSelected(m_connsInfo[0]);
				found = true;
			} else if(m_mismatchConnsInfo.size() > 0) {
				setSelected(m_mismatchConnsInfo[0]);
				found = true;
			}
			if(found) {
				this->setFocus();
			}
		}
	}
}*/

void ConnectorsInfoWidget::selectNext() {
	if(m_selected) {
		int selIdx = m_connsInfo.indexOf((SingleConnectorInfoWidget*)m_selected);
		if(selIdx > -1 && selIdx < m_connsInfo.size()-1) { // It's a single connector
			setSelected(m_connsInfo[selIdx+1]);
		} else {
			selIdx = m_mismatchConnsInfo.indexOf((MismatchingConnectorWidget*)m_selected);
			if(selIdx > -1) {
				if(selIdx < m_mismatchConnsInfo.size()-1) {
					setSelected(m_mismatchConnsInfo[selIdx+1]);
				} else if(m_connsInfo.size() > 0) {
					setSelected(m_connsInfo[0]);
				}
			}
		}
	}
}

void ConnectorsInfoWidget::selectPrev() {
	if(m_selected) {
		int selIdx = m_connsInfo.indexOf((SingleConnectorInfoWidget*)m_selected);
		if(selIdx > -1 ) { // It's a single connector and not the first
			if(selIdx > 0) {
				setSelected(m_connsInfo[selIdx-1]);
			} else if(m_mismatchConnsInfo.size()>0) {
				setSelected(m_mismatchConnsInfo[m_mismatchConnsInfo.size()-1]);
			}
		} else {
			selIdx = m_mismatchConnsInfo.indexOf((MismatchingConnectorWidget*)m_selected);
			if(selIdx > 0) { // it's a mismatch
				setSelected(m_mismatchConnsInfo[selIdx-1]);
			}
		}
	}
}

void ConnectorsInfoWidget::updateLayout() {
	m_mismatchersFrame->updateGeometry();
	m_mismatchersFrame->adjustSize();
	m_mismatchersFrameParent->updateGeometry();
	m_mismatchersFrameParent->adjustSize();
	m_scrollContent->adjustSize();
}

void ConnectorsInfoWidget::addConnectorInfo(MismatchingConnectorWidget* mcw) {
	if(mcw->prevConn()) {
		addConnectorInfo(mcw->prevConn());
	} else {
		addConnectorInfo(mcw->connId());
	}
}

Connector* ConnectorsInfoWidget::addConnectorInfo(QString id) {
	ConnectorStuff *connStuff = new ConnectorStuff();
	connStuff->setId(id);
	Connector *conn = new Connector(connStuff,0); // modelPart =? null
	addConnectorInfo(conn);
	return conn;
}

void ConnectorsInfoWidget::addConnectorInfo(Connector *conn) {
	m_connIds << conn->connectorStuffID();

	int connCount = m_connsInfo.size();
	SingleConnectorInfoWidget *sci = new SingleConnectorInfoWidget(m_undoStack,conn,m_scrollContent);
	scrollContentLayout()->addWidget(sci,connCount+1,0);
	m_connsInfo << sci;
	connect(sci,SIGNAL(editionStarted()),this,SLOT(updateLayout()));
	connect(sci,SIGNAL(editionFinished()),this,SLOT(updateLayout()));
	connect(sci,SIGNAL(tellSistersImNewSelected(AbstractConnectorInfoWidget*)),this,SLOT(selectionChanged(AbstractConnectorInfoWidget*)));
	connect(sci,SIGNAL(tellViewsMyConnectorIsNewSelected(const QString&)),this,SLOT(informConnectorSelection(const QString &)));
	connect(this,SIGNAL(editionCompleted()),sci,SLOT(editionCompleted()));
	m_scrollContent->updateGeometry();
}

void ConnectorsInfoWidget::addMismatchingConnectorInfo(ItemBase::ViewIdentifier viewId, QString connId) {
	m_connIds << connId;
	addMismatchingConnectorInfo(new MismatchingConnectorWidget(viewId,connId,m_mismatchersFrame));
}

void ConnectorsInfoWidget::addMismatchingConnectorInfo(MismatchingConnectorWidget *mcw) {
	int connCount = m_mismatchConnsInfo.size();

	((QGridLayout*)m_mismatchersFrame->layout())->addWidget(mcw,connCount,0);
	m_mismatchConnsInfo << mcw;
	connect(mcw,SIGNAL(tellSistersImNewSelected(AbstractConnectorInfoWidget*)),this,SLOT(selectionChanged(AbstractConnectorInfoWidget*)));
	connect(mcw,SIGNAL(tellViewsMyConnectorIsNewSelected(const QString &)),this,SLOT(informConnectorSelection(const QString &)));
	if(m_mismatchConnsInfo.size()==1) {
		m_mismatchersFrameParent->show();
	}

	/*if(!m_selected && connCount == 0) {
		setSelected(mcw);
	}*/

	foreach(ItemBase::ViewIdentifier viewId, mcw->views()) {
		emit setMismatching(viewId, mcw->connId(), true);
	}
}

QGridLayout *ConnectorsInfoWidget::scrollContentLayout() {
	return (QGridLayout*)m_scrollContent->layout();
}

void ConnectorsInfoWidget::connectorsFound(QList<Connector *> conns) {
	qSort(conns);

	QProgressDialog progress(tr("Loading connectors..."), 0, 0, conns.size(), this);
	progress.show();
	for(int i=0; i < conns.size(); i++) {
		progress.setValue(i);
		qApp->processEvents(); // to keep the app away from freezing
		addConnectorInfo(conns[i]);
	}
	progress.setValue(conns.size());

	updateLayout();
}

void ConnectorsInfoWidget::informConnectorSelection(const QString &connId) {
	emit connectorSelected(connId);
}

void ConnectorsInfoWidget::informEditionCompleted() {
	emit editionCompleted();
}

const QList<ConnectorStuff *> ConnectorsInfoWidget::connectorsStuffs() {
	QList<ConnectorStuff *> connectorsStuff;
	for(int i=0; i<m_connsInfo.size(); i++) {
		SingleConnectorInfoWidget *sci = m_connsInfo[i];
		QString id = sci->id();
		Connector *conn = sci->connector();
		ConnectorStuff* cs = conn->connectorStuff();
		cs->setId(id);
		cs->setName(sci->name());
		cs->setDescription(sci->description());
		cs->setConnectorType(sci->type());

		foreach(ItemBase::ViewIdentifier viewId, m_connectorsPins[id].keys()) {
			cs->removePins(viewId);
			foreach(SvgIdLayer *pin, m_connectorsPins[id].values(viewId)) { // Multihash
				// TODO Mariano: change this layer if the connectors aren't in the default layer
				ViewLayer::ViewLayerID viewLayerId = pin->m_viewLayerID == ViewLayer::UnknownLayer
					? ItemBase::defaultConnectorLayer(viewId)
					: pin->m_viewLayerID;
				cs->addPin(viewId, pin->m_svgId, viewLayerId, pin->m_terminalId);
			}
		}

		connectorsStuff << cs;
	}
	return connectorsStuff;
}

// If we're reloading an image, clear mismatching connectors related exclusively to that view
void ConnectorsInfoWidget::clearMismatchingForView(ItemBase::ViewIdentifier viewId) {
	foreach(MismatchingConnectorWidget* mcw, m_mismatchConnsInfo) {
		if(mcw->views().size()==1 &&  mcw->views()[0] == viewId) {
			removeMismatchingConnectorInfo(mcw);
		}
	}
}

// Updates previous connector to mismatching if they are not in the list
void ConnectorsInfoWidget::singleToMismatchingNotInView(ItemBase::ViewIdentifier viewId, const QStringList &connIds) {
	foreach(SingleConnectorInfoWidget* sci, m_connsInfo) {
		if(connIds.indexOf(sci->connector()->connectorStuffID()) == -1) {
			addMismatchingConnectorInfo(sci->toMismatching(viewId));
			removeConnectorInfo(sci);
		}
	}
	m_mismatchersFrame->adjustSize();
	scrollContentLayout()->update();
	m_mismatchersFrameParent->adjustSize();
	m_mismatchersFrameParent->layout()->update();
	updateLayout();
}

void ConnectorsInfoWidget::syncNewConnectors(ItemBase::ViewIdentifier viewId, const QList<Connector*> &conns) {
	clearMismatchingForView(viewId);

	// clean the old pins for this view
	foreach(QString oldId, m_connectorsPins.keys()) {
		m_connectorsPins[oldId].remove(viewId);
	}

	QStringList connIds;
	foreach(Connector *conn, conns) {
		QString connId = conn->connectorStuffID();
		connIds << connId;

		foreach(SvgIdLayer* pin, conn->connectorStuff()->pins().values(viewId)) {
			m_connectorsPins[connId].insert(viewId,pin);
		}

		if(existingConnId(connId)) {
			emit existingConnector(viewId, connId, findConnector(connId));
		} else {
			MismatchingConnectorWidget *mcw;
			if(( mcw = existingMismatchingConnector(connId) )) {
				if(mcw->onlyMissingThisView(viewId)) {
					removeMismatchingConnectorInfo(mcw);
					addConnectorInfo(mcw);
					emit existingConnector(viewId, connId, findConnector(connId));
				} else {
					mcw->addViewPresence(viewId);
					emit setMismatching(viewId, mcw->connId(), true);
				}
			} else {
				addMismatchingConnectorInfo(viewId, connId);
			}
		}
	}

	//clearMismatchingForView(viewId);
	singleToMismatchingNotInView(viewId,connIds);
}

bool ConnectorsInfoWidget::existingConnId(const QString &id) {
	return findConnector(id) != NULL;
}

MismatchingConnectorWidget* ConnectorsInfoWidget::existingMismatchingConnector(const QString &id) {
	foreach(MismatchingConnectorWidget *mci, m_mismatchConnsInfo) {
		if(mci->connId() == id) {
			return mci;
		}
	}
	return NULL;
}

void ConnectorsInfoWidget::removeMismatchingConnectorInfo(MismatchingConnectorWidget* mcw, bool alsoDeleteFromView) {
	m_mismatchersFrame->layout()->removeWidget(mcw);
	m_mismatchConnsInfo.removeOne(mcw);
	if(m_mismatchConnsInfo.size()==0) {
		m_mismatchersFrameParent->hide();
		updateLayout();
	}

	if(alsoDeleteFromView) {
		emit removeConnectorFrom(mcw->connId(),ItemBase::AllViews);
	} else {
		foreach(ItemBase::ViewIdentifier viewId, mcw->views()) {
			emit setMismatching(viewId, mcw->connId(), false);
		}
	}

	if(m_selected == mcw) {
		m_selected = NULL;
	}

	delete mcw;
}

void ConnectorsInfoWidget::removeConnectorInfo(SingleConnectorInfoWidget *sci, bool alsoDeleteFromView) {
	scrollContentLayout()->removeWidget(sci);
	m_connsInfo.removeOne(sci);

	if(m_selected == sci) {
		m_selected = NULL;
	}

	if(alsoDeleteFromView) {
		emit removeConnectorFrom(sci->connector()->connectorStuffID(), ItemBase::AllViews);
	}

	delete sci;
}

Connector* ConnectorsInfoWidget::findConnector(const QString &id) {
	foreach(SingleConnectorInfoWidget *sci, m_connsInfo) {
		if(sci->id() == id) {
			return sci->connector();
		}
	}
	return NULL;
}


void ConnectorsInfoWidget::emitShowHideTerminalPoints(int checkState) {
	if(checkState == Qt::Checked) {
		emit showTerminalPoints(true);
	} else if(checkState == Qt::Unchecked) {
		emit showTerminalPoints(false);
	}
}

void ConnectorsInfoWidget::addConnector() {
	QString connId = QString("connector%1").arg(nextConnId());
	emit drawConnector(addConnectorInfo(connId),m_showTerminalPointsCheckBox->isChecked());
}

void ConnectorsInfoWidget::removeSelectedConnector() {
	if(!m_selected) return;
	MismatchingConnectorWidget* mismatch = dynamic_cast<MismatchingConnectorWidget*>(m_selected);
	if(mismatch) {
		removeMismatchingConnectorInfo(mismatch, true);
	} else {
		SingleConnectorInfoWidget *single = dynamic_cast<SingleConnectorInfoWidget*>(m_selected);
		if(single) {
			removeConnectorInfo(single, true);
		}
	}
}

int ConnectorsInfoWidget::nextConnId() {
	int max = 0;
	foreach(QString connId, m_connIds) {
		QString currId = connId;

		if(currId.startsWith("connector")) {
			QString helpStr = currId.remove("connector");
			if(!helpStr.isEmpty()) {
				bool isInt;
				int helpInt = helpStr.toInt(&isInt);
				if(isInt && max <= helpInt) {
					max = ++helpInt;
				}
			}
		}
	}
	return max;
}

QCheckBox *ConnectorsInfoWidget::showTerminalPointsCheckBox() {
	return m_showTerminalPointsCheckBox;
}
