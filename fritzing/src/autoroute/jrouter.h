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

$Revision$:
$Author$:
$Date$

********************************************************************/

#ifndef JROUTER_H
#define JROUTER_H

#include <QAction>
#include <QHash>
#include <QVector>
#include <QList>
#include <QSet>
#include <QPointF>
#include <QGraphicsItem>
#include <QLine>
#include <QProgressDialog>
#include <QUndoCommand>

#include "../viewgeometry.h"
#include "../viewlayer.h"

struct JEdge {
	class ConnectorItem * from;
	class ConnectorItem * to;
	double distance;
	bool ground;
	QList<class ConnectorItem *> fromConnectorItems;
	QSet<class Wire *> fromTraces;
	QList<class ConnectorItem *> toConnectorItems;
	QSet<class Wire *> toTraces;
};

struct JSubedge {
	struct JEdge * edge;
	ConnectorItem * from;
	ConnectorItem * to;
	QPointF fromPoint;
	QPointF toPoint;
	class Wire * fromWire;
	class Wire * toWire;
	QPointF point;
	double distance;
	bool forward;
};

class GridEntry : public QGraphicsRectItem {
public:
	enum {
		EMPTY = 1,
		IGNORE = 2,
		SAFE = 4,
		ALIGN = 8,
		SELF = 16,
		GOAL = 32,   // goal followed by blockers must be highest values
		OWNSIDE = 64,
		BLOCK = 128,
		NOTBOARD = 256
	};

public:
	GridEntry(qreal x, qreal y, qreal width, qreal height, int flags, QGraphicsItem * parent); 

public:
	int m_flags;
};

class JRouter : public QObject
{
	Q_OBJECT

public:
	JRouter(class PCBSketchWidget *);
	~JRouter(void);

	void start();
	
protected:
	bool drawTrace(struct JSubedge *, struct Plane *, ViewLayer::ViewLayerID viewLayerID, QList<Wire *> &);
	void cleanUp();
	void updateRoutingStatus();
	class JumperItem * drawJumperItem(struct JumperItemStruct *);
	void restoreOriginalState(QUndoCommand * parentCommand);
	void addToUndo(Wire * wire, QUndoCommand * parentCommand);
	void addToUndo(QUndoCommand * parentCommand, QList<struct JumperItemStruct *> &);
	void reduceWires(QList<Wire *> & wires, ConnectorItem * from, ConnectorItem * to, const QPolygonF & boundingPoly);
	Wire * reduceWiresAux(QList<Wire *> & wires, ConnectorItem * from, ConnectorItem * to, QPointF fromPos, QPointF toPos, const QPolygonF & boundingPoly);
	void findNearestIntersection(QLineF & l1, QPointF & fromPos, const QPolygonF & boundingPoly, bool & inBounds, QPointF & nearestBoundsIntersection, qreal & nearestBoundsIntersectionDistance); 
	class TraceWire * drawOneTrace(QPointF fromPos, QPointF toPos, int width, ViewLayer::ViewLayerSpec);
	void reduceColinearWires(QList<Wire *> &);
	void expand(ConnectorItem * connectorItem, QList<ConnectorItem *> & connectorItems, QSet<Wire *> & visited);
	bool findSpaceFor(ConnectorItem * & from, class JumperItem *, struct JumperItemStruct *, QPointF & candidate); 
	void collectEdges(QVector<int> & netCounters, QList<struct JEdge *> & edges, ViewLayer::ViewLayerID);
	bool traceSubedge(struct JSubedge* subedge, struct Plane *, class ItemBase * partForBounds, ViewLayer::ViewLayerID);
	void fixupJumperItems(QList<struct JumperItemStruct *> &);
	void runEdges(QList<JEdge *> & edges, QList<struct JumperItemStruct *> & jumperItemStructs,
				  QVector<int> & netCounters, struct RoutingStatus &);
	void clearEdges(QList<JEdge *> & edges);
	void doCancel(QUndoCommand * parentCommand);
	bool alreadyJumper(QList<struct JumperItemStruct *> & jumperItemStructs, ConnectorItem * from, ConnectorItem * to);
	bool hasCollisions(JumperItem *, ViewLayer::ViewLayerID, QGraphicsItem *, ConnectorItem * from); 
	void updateProgress(int num, int denom);
	GridEntry * drawGridItem(qreal x1, qreal y1, qreal x2, qreal y2, int distance, short flag);
	bool propagate(JSubedge * subedge, QList<struct Tile *> & path, struct Plane *, ViewLayer::ViewLayerID);
	bool backPropagate(JSubedge * subedge, QList<struct Tile *> & path, struct Plane *, ViewLayer::ViewLayerID viewLayerID, QList<Wire *> & wires);
	short checkCandidate(JSubedge * subedge, struct Tile *, ViewLayer::ViewLayerID);
	JSubedge * makeSubedge(JEdge * edge, QPointF p1, ConnectorItem * from, QPointF p2, ConnectorItem * to, bool forward);
	struct Tile * addTile(class NonConnectorItem * nci, int type, struct Plane *, QList<struct Tile *> & alreadyTiled);
	void seedNext(Tile * seed, QList<Tile *> & seeds);
	struct Plane * tilePlane(ItemBase * board, ViewLayer::ViewLayerID, QList<struct Tile *> & alreadyTiled);
	void tileWire(Wire *, struct Plane *, QList<Wire *> & beenThere, QList<struct Tile *> & alreadyTiled);
	short checkConnector(JSubedge * subedge, Tile * tile, ViewLayer::ViewLayerID viewLayerID, ConnectorItem *);
	short checkTrace(JSubedge * subedge, Tile * tile, ViewLayer::ViewLayerID viewLayerID, Wire *);
	void clearTiles(struct Plane * thePlane);
	void displayBadTiles(QList<struct Tile *> & alreadyTiled);
	struct Tile * insertTile(struct Plane* thePlane, struct TileRect &tileRect, QList<struct Tile *> &alreadyTiled, QGraphicsItem *, int type, bool adjustToGrid);
	void clearGridEntries();
	void appendIf(Tile * seed, Tile * next, QList<Tile *> & seeds, bool (*enoughOverlap)(Tile*, Tile*));
	void sliceWireHorizontally(Wire * w, qreal angle, QPointF p1, QPointF p2, QList<QRectF> & rects);
	void sliceWireVertically(Wire * w, qreal angle, QPointF p1, QPointF p2, QList<QRectF> & rects);
	struct SeedTree * followPath(SeedTree * & root, QList<Tile *> & path);
	void drawDirectionHorizontal(QPointF & startPoint, QPointF & endPoint, QRectF & fromTileRect, QRectF & toTileRect, QList<Wire *> & wires);
	void drawDirectionVertical(QPointF & startPoint, QPointF & endPoint, QRectF & fromTileRect, QRectF & toTileRect, QList<Wire *> & wires);

protected:
	static void clearTraces(PCBSketchWidget * sketchWidget, bool deleteAll, QUndoCommand * parentCommand);
	static void addUndoConnections(PCBSketchWidget * sketchWidget, bool connect, QList<Wire *> & wires, QUndoCommand * parentCommand);

public slots:
	void cancel();
	void cancelTrace();
	void stopTrace();

signals:
	void setMaximumProgress(int);
	void setProgressValue(int);
	void wantTopVisible();
	void wantBottomVisible();

protected:
	class PCBSketchWidget * m_sketchWidget;
	QList< QList<class ConnectorItem *>* > m_allPartConnectorItems;
	bool m_cancelled;
	bool m_cancelTrace;
	bool m_stopTrace;
	bool m_bothSidesNow;
	QGraphicsItem * m_nearestObstacle;
	QList<Wire *> m_cleanWires;
	ViewLayer::ViewLayerSpec m_viewLayerSpec;
	int m_maximumProgressPart;
	int m_currentProgressPart;
	QRectF m_maxRect;
};

#endif
