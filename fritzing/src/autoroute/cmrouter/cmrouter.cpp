/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2011 Fachhochschule Potsdam - http://fh-potsdam.de

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

This router is  based on the one described in Contour: A Tile-based Gridless Router
http://www.hpl.hp.com/techreports/Compaq-DEC/WRL-95-3.pdf

Plus additional ideas from An Efficient Tile-Based ECO Router with Routing Graph Reduction
http://www.cis.nctu.edu.tw/~ylli/paper/f69-li.pdf

The corner stitching code is a modified version of code from the Magic VLSI Layout Tool
http://opencircuitdesign.com/magic/

********************************************************************

$Revision$:
$Author$:
$Date$

********************************************************************/

// TODO:
//
//	make DRC available from trace menu
//	add silkscreen overlap to DRC check (check for overlaps, but don't insert tiles)
//  run separate beginning overlap check with half keepout width
//
//	schematic view: blocks parts, not traces
//
//	fix up cancel/stop: 
//		stop either stops you where you are, 
//		or goes back to the best outcome , if you're in ripup-and-reroute phase
//
//	placing jumpers 
//		if double-sided, tile both sides
//		run the all-at-once search and if it fails
//			run through each queue looking for space tiles that will fit a jumper end
//			choose the space tile closest to an originating connectorItem or wire
//			if double-sided, check that an intersecting space is available on the other layer
//
//	placing vias
//		if double-sided, tile both sides
//		during the normal course of routing, whenever a space large enough for a via is found
//		somehow include the cost of additional real estate
//			see if there's an intersecting space on the other side
//				if so, add that space to the path
//
//	option to turn off propagation feedback
//	remove debugging output and extra calls to processEvents
//
//	after first solution is found, allow user to stop seeking for another
//
//	bugs: 
//		loop: longer route
//		arduino isp: arduino too close to the edge? fails an early easy route in the backtrace
//		arduino no ftdi (barebones arduino): chips too close to the edge? 
//				fails easy routes in the backtrace; 
//				weird short traces; 
//				one off-center trace
//		blink: arduino overlaps?
//		tinyspace next to border is weird
//		border seems asymmetric
//		thin lines need to be better joined
//
//  longer route than expected: 
//		This is because outward propagation stops when the goal is first reached.  
//		It is possible that the shortest tile route is actually longer than the shortest crow-fly route.  
//		For example, in the f�llowing case, route ABC will reach goal before ABDEF:
//                                   -------
//                                   |  A  |  
//      ------------------------------------
//      |               B                  |
//		------------------------------------
//      |       |                 |    D   |
//      |       |               --------------
//      |   C   |               |      E     |
//      |       |              -------------------
//      |       |              |       F         |
//      ------------------------------------------
//      |              GOAL                      |
//      ------------------------------------------
//		There is a way to deal with this in the following paper: http://eie507.eie.polyu.edu.hk/projects/sp-tiles-00980255.pdf
//
//	redo tiling non-manhattan wires
//
//  wire stickiness
//
//	still seeing a few thin tiles going across the board: 
//		this is because the thick tiles above and below are wider than the thin tile
//
//	rip up and reroute:
//		keep a hash table from traces to edges
//		when edges are generated give each an integer ID
//		when first pass at routing is over and there are unrouted edges
//		save the traces (how) along with some score (number of open edges)
//		save the list of IDs in order
//		foreach ratsnest wire remaining (i.e. edge remaining) 
//			find all intersections with traces and map to edges
//			move this edge above that edge in the list of edges
//			just do one edge or all open edges?  
//		keep the traces from edges above those that got moved, clear the rest
//		save the new ordering (list of IDs)
//		now route again with the new edge order from the point of change
//		if no open edges we are done
//			compare edge score with previous and keep the best set of traces
//		how to stop--keep going until user stops or some repeat condition occurs.
//			check the reordering of edges and if they match a previous set quit 
//


#include "cmrouter.h"
#include "../../sketch/pcbsketchwidget.h"
#include "../../debugdialog.h"
#include "../../items/virtualwire.h"
#include "../../items/tracewire.h"
#include "../../items/jumperitem.h"
#include "../../items/resizableboard.h"
#include "../../utils/graphicsutils.h"
#include "../../utils/textutils.h"
#include "../../connectors/connectoritem.h"
#include "../../items/moduleidnames.h"
#include "../../processeventblocker.h"
#include "../../svg/groundplanegenerator.h"
#include "../../fsvgrenderer.h"

#include "tile.h"

#include <qmath.h>
#include <QApplication>
#include <QMessageBox> 

static inline void infoTile(const QString & message, Tile * tile)
{
	DebugDialog::debug(QString("%1 tile:%2 l:%3 t:%4 w:%5 h:%6 type:%7 body:%8")
		.arg(message)
		.arg((long) tile, 0, 16)
		.arg(LEFT(tile))
		.arg(YMIN(tile))
		.arg(WIDTH(tile))
		.arg(HEIGHT(tile))
		.arg(TiGetType(tile))
		.arg((long) TiGetBody(tile), 0, 16)
	);
}

static inline void infoTileRect(const QString & message, const TileRect & tileRect)
{
	DebugDialog::debug(QString("%1 l:%2 t:%3 w:%4 h:%5")
		.arg(message)
		.arg(tileRect.xmini)
		.arg(tileRect.ymini)
		.arg(tileRect.xmaxi - tileRect.xmini)
		.arg(tileRect.ymaxi - tileRect.ymini)
	);
}


static inline int manhattan(TileRect & tr1, TileRect & tr2) {
	int dx =  qAbs(tr1.xmaxi - tr2.xmaxi);
	dx = qMin(qAbs(tr1.xmaxi - tr2.xmini), dx);
	dx = qMin(qAbs(tr1.xmini - tr2.xmaxi), dx);
	dx = qMin(qAbs(tr1.xmini - tr2.xmini), dx);
	int dy =  qAbs(tr1.ymaxi - tr2.ymaxi);
	dy = qMin(qAbs(tr1.ymaxi - tr2.ymini), dy);
	dy = qMin(qAbs(tr1.ymini - tr2.ymaxi), dy);
	dy = qMin(qAbs(tr1.ymini - tr2.ymini), dy);
	return dx + dy;
}

static inline qreal dot(const QPointF & p1, const QPointF & p2)
{
	return (p1.x() * p2.x()) + (p1.y() * p2.y());
}

static inline GridEntry * TiGetGridEntry(Tile * tile) { return dynamic_cast<GridEntry *>(TiGetClient(tile)); }

static const int MaximumProgress = 1000;
static qreal KeepoutSpace = 0;
static const qreal TINYSPACEMAX = 10;
static const int TILEFACTOR = 1000;
static int TileStandardWireWidth = 0;
static int TileHalfStandardWireWidth = 0;
static qreal HalfStandardWireWidth = 0;
static int TileKeepoutSpace = 0;
static const qreal CloseEnough = 0.5;


const int Segment::NotSet = std::numeric_limits<int>::min();


bool edgeLessThan(JEdge * e1, JEdge * e2)
{
	if (e1->viewLayerSpec != e2->viewLayerSpec) {
		// do bottom edges first
		return e1->viewLayerSpec == ViewLayer::Bottom;
	}

	return e1->distance < e2->distance;
}

inline int realToTile(qreal x) {
	return qRound(x * TILEFACTOR);
}

void realsToTile(TileRect & tileRect, qreal l, qreal t, qreal r, qreal b) {
	tileRect.xmini = realToTile(l);
	tileRect.ymini = realToTile(t);
	tileRect.xmaxi = realToTile(r);
	tileRect.ymaxi = realToTile(b);
}

inline qreal tileToReal(int x) {
	return x / ((qreal) TILEFACTOR);
}

void tileToQRect(Tile * tile, QRectF & rect) {
	TileRect tileRect;
	TiToRect(tile, &tileRect);
	rect.setCoords(tileToReal(tileRect.xmini), tileToReal(tileRect.ymini), tileToReal(tileRect.xmaxi), tileToReal(tileRect.ymaxi));
}

bool tileRectIntersects(TileRect * tile1, TileRect * tile2)
{
    int l1 = tile1->xmini;
    int r1 = tile1->xmaxi;
    if (l1 == r1) // null rect
        return false;

    int l2 = tile2->xmini;
    int r2 = tile2->xmaxi;
    if (l2 == r2) // null rect
        return false;

    if (l1 >= r2 || l2 >= r1)
        return false;

    int t1 = tile1->ymini;
    int b1 = tile1->ymaxi;
    if (t1 == b1) // null rect
        return false;

    int t2 = tile2->ymini;
    int b2 = tile2->ymaxi;
    if (t2 == b2) // null rect
        return false;

    if (t1 >= b2 || t2 >= b1)
        return false;

    return true;
}

QSet<Tile *> ChangedTiles;

static void CollectChangedTiles(Tile * tile) {
	ChangedTiles.insert(tile);
}

static void RemoveChangedTiles(Tile * tile) {
	ChangedTiles.remove(tile);
}

int simpleList(Tile * tile, UserData data) {
	QList<Tile *> * tiled = (QList<Tile *> *) data;
	tiled->append(tile);
	return 0;
}

////////////////////////////////////////////////////////////////////

GridEntry::GridEntry(QRectF & r, QGraphicsItem * parent) : QGraphicsRectItem(r, parent)
{
	setAcceptedMouseButtons(Qt::NoButton);
	setAcceptsHoverEvents(false);
}

////////////////////////////////////////////////////////////////////

CMRouter::CMRouter(PCBSketchWidget * sketchWidget) : Autorouter(sketchWidget)
{
	KeepoutSpace = KeepoutSpace = 0.015 * FSvgRenderer::printerScale();;			// 15 mils space
	TileKeepoutSpace = realToTile(KeepoutSpace);
	TileStandardWireWidth = realToTile(Wire::STANDARD_TRACE_WIDTH);
	HalfStandardWireWidth = Wire::STANDARD_TRACE_WIDTH / 2;
	TileHalfStandardWireWidth = realToTile(HalfStandardWireWidth);
}

CMRouter::~CMRouter()
{
}

void CMRouter::start()
{	
	m_maximumProgressPart = 2;
	m_currentProgressPart = 0;

	emit setMaximumProgress(MaximumProgress);

	RoutingStatus routingStatus;
	routingStatus.zero();

	m_sketchWidget->ensureTraceLayersVisible();

	clearGridEntries();

	QUndoCommand * parentCommand = new QUndoCommand("Autoroute");
	new CleanUpWiresCommand(m_sketchWidget, CleanUpWiresCommand::UndoOnly, parentCommand);

	m_bothSidesNow = m_sketchWidget->routeBothSides();
	if (m_bothSidesNow) {
		m_maximumProgressPart = 3;
		emit wantBottomVisible();
		ProcessEventBlocker::processEvents();
	}

	clearTraces(m_sketchWidget, false, parentCommand);
	updateRoutingStatus();

	QHash<ConnectorItem *, int> indexer;
	m_sketchWidget->collectAllNets(indexer, m_allPartConnectorItems, false, m_bothSidesNow);

	if (m_allPartConnectorItems.count() == 0) {
		return;
	}

	// will list connectors on both sides separately
	routingStatus.m_netCount = m_allPartConnectorItems.count();

	QVector<int> netCounters(m_allPartConnectorItems.count());
	for (int i = 0; i < m_allPartConnectorItems.count(); i++) {
		netCounters[i] = (m_allPartConnectorItems[i]->count() - 1) * 2;			// since we use two connectors at a time on a net
	}

	if (m_cancelled || m_stopTrace) {
		restoreOriginalState(parentCommand);
		cleanUp();
		return;
	}

	ProcessEventBlocker::processEvents(); // to keep the app  from freezing

	ItemBase * board = NULL;
	if (m_sketchWidget->autorouteNeedsBounds()) {
		board = m_sketchWidget->findBoard();
	}

	QList<JEdge *> edges;
	QList<Plane *> planes;
	bool allDone = false;
	QList< QList<int> > orderings;
	QHash<Wire *, JEdge *> tracesToEdges;
	for (int run = 0; run < 10; run++) {
		allDone = runEdges(edges, planes, board, netCounters, routingStatus, run == 0, tracesToEdges);
		if (m_cancelled) break;
		if (allDone) break;

		QList<int> ordering;
		foreach (JEdge * edge, edges) {
			ordering.append(edge->id);
		}
		orderings.append(ordering);	

		reorderEdges(edges, tracesToEdges);
		bool gotOne = false;
		foreach (QList<int> ordering, orderings) {
			bool allGood = true;
			for (int i = 0; i < edges.count(); i++) {
				if (ordering.at(i) != edges.at(i)->id) {
					allGood = false;
					break;
				}
			}
			if (allGood) {
				gotOne = true;
				break;
			}
		}
		if (gotOne) {
			// we cycled back to a previous order
			break;
		}

		// TODO: only delete the ones that have been reordered
		foreach (Wire * trace, tracesToEdges.keys()) {
			delete trace;
		}
		tracesToEdges.clear();
		foreach (Plane * plane, planes) clearTiles(plane);
		planes.clear();
	}

	if (m_cancelled) {
		clearEdges(edges);
		foreach (Plane * plane, planes) clearTiles(plane);
		doCancel(parentCommand);
		return;
	}

	m_currentProgressPart++;
	//fixupJumperItems(edges, board);

	cleanUp();

	addToUndo(parentCommand, edges);

	clearEdges(edges);
	foreach (Plane * plane, planes) clearTiles(plane);
	new CleanUpWiresCommand(m_sketchWidget, CleanUpWiresCommand::RedoOnly, parentCommand);

	m_sketchWidget->pushCommand(parentCommand);
	m_sketchWidget->repaint();
	DebugDialog::debug("\n\n\nautorouting complete\n\n\n");
}

struct SourceAndDestinationStruct {
	QList<Tile *> tiles;
	JEdge * edge;
};

int findSourceAndDestination(Tile * tile, UserData data) {
	SourceAndDestinationStruct * sourceAndDestinationStruct = (SourceAndDestinationStruct *) data;

	ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(TiGetBody(tile));
	if (connectorItem) {
		if (sourceAndDestinationStruct->edge->fromConnectorItems.contains(connectorItem)) {
			TiSetType(tile, Tile::SOURCE);
			sourceAndDestinationStruct->tiles.append(tile);
		}
		else if (sourceAndDestinationStruct->edge->toConnectorItems.contains(connectorItem)) {
			TiSetType(tile, Tile::DESTINATION);
			sourceAndDestinationStruct->tiles.append(tile);
		}

		return 0;
	}

	Wire * wire = dynamic_cast<Wire *>(TiGetBody(tile));
	if (wire == NULL) return 0;

	if (sourceAndDestinationStruct->edge->fromTraces.contains(wire)) {
		TiSetType(tile, Tile::SOURCE);
		sourceAndDestinationStruct->tiles.append(tile);
	}
	else if (sourceAndDestinationStruct->edge->toTraces.contains(wire)) {
		TiSetType(tile, Tile::DESTINATION);
		sourceAndDestinationStruct->tiles.append(tile);
	}
	
	return 0;
}

int clearSourceAndDestination(Tile * tile, UserData) {
	if (TiGetType(tile) == Tile::SOURCE || TiGetType(tile) == Tile::DESTINATION) {
		TiSetType(tile, Tile::OBSTACLE);
	}
	return 0;
}

bool CMRouter::runEdges(QList<JEdge *> & edges, QList<Plane *> & planes, ItemBase * board,
					   QVector<int> & netCounters, RoutingStatus & routingStatus, bool firstTime,
					   QHash<Wire *, JEdge *> & tracesToEdges)
{	
	bool allRouted = true;

	ViewGeometry vg;
	vg.setTrace(true);
	ViewLayer::ViewLayerID copper0 = m_sketchWidget->getWireViewLayerID(vg, ViewLayer::Bottom);
	ViewLayer::ViewLayerID copper1 = m_sketchWidget->getWireViewLayerID(vg, ViewLayer::Top);

	QList<Tile *> alreadyTiled;
	Plane * plane0 = tilePlane(board, copper0, alreadyTiled);
	if (plane0) planes.append(plane0);
	Plane * plane1 = NULL;
	if (alreadyTiled.count() > 0) {
		m_cancelled = true;
		displayBadTiles(alreadyTiled);
		QMessageBox::warning(NULL, QObject::tr("Fritzing"), QObject::tr("Cannot autoroute: parts or traces are overlapping"));
		return false;
	}

	if (m_bothSidesNow) {
		plane1 = tilePlane(board, copper1, alreadyTiled);
		if (plane1) planes.append(plane1);
		if (alreadyTiled.count() > 0) {
			m_cancelled = true;
			displayBadTiles(alreadyTiled);
			QMessageBox::warning(NULL, QObject::tr("Fritzing"), QObject::tr("Cannot autoroute: parts or traces are overlapping"));
			return false;
		}
	}

	if (firstTime) {
		collectEdges(edges, plane0, plane1, copper0, copper1);
		qSort(edges.begin(), edges.end(), edgeLessThan);	// sort the edges by distance and layer
	}
	else {
		foreach (JEdge * edge, edges) {
			edge->plane = (edge->viewLayerID == copper0) ? plane0 : plane1;
			edge->routed = false;
			edge->fromConnectorItems.clear();
			edge->toConnectorItems.clear();
			edge->fromTraces.clear();
			edge->toTraces.clear();
		}
	}

	int edgesDone = 0;
	foreach (JEdge * edge, edges) {	
		if (edge->linkedEdge && edge->linkedEdge->routed) {
			// got it one one side; no need to route it on the other
			continue;
		}

		expand(edge->from, edge->fromConnectorItems, edge->fromTraces);
		expand(edge->to, edge->toConnectorItems, edge->toTraces);

		PriorityQueue<PathUnit *> queue1;
		PriorityQueue<PathUnit *> queue2;

		QMultiHash<Tile *, PathUnit *> tilePathUnits;

		SourceAndDestinationStruct sourceAndDestinationStruct;
		sourceAndDestinationStruct.edge = edge;
		TiSrArea(NULL, edge->plane, &m_tileMaxRect, findSourceAndDestination, &sourceAndDestinationStruct);

		foreach (Tile * tile, sourceAndDestinationStruct.tiles) {
			initPathUnit(edge, tile, (TiGetType(tile) == Tile::SOURCE ? queue1 : queue2), tilePathUnits);
		}

		edge->routed = propagate(queue1, queue2, edge, tracesToEdges, tilePathUnits, board);

		if (!edge->routed) {
			allRouted = false;
		}

		foreach (PathUnit * pathUnit, m_pathUnits) {
			delete pathUnit;
		}
		m_pathUnits.clear();

		TiSrArea(NULL, edge->plane, &m_tileMaxRect, clearSourceAndDestination, NULL);


		updateProgress(++edgesDone, edges.count());

		for (int i = 0; i < m_allPartConnectorItems.count(); i++) {
			if (m_allPartConnectorItems[i]->contains(edge->from)) {
				netCounters[i] -= 2;
				break;
			}
		}

		routingStatus.m_netRoutedCount = 0;
		routingStatus.m_connectorsLeftToRoute = edges.count() + 1 - edgesDone;
		foreach (int c, netCounters) {
			if (c <= 0) {
				routingStatus.m_netRoutedCount++;
			}
		}
		m_sketchWidget->forwardRoutingStatus(routingStatus);

		ProcessEventBlocker::processEvents();

		if (m_cancelled) {
			break;
		}

		if (m_stopTrace) {
			break;
		}
	}

	return allRouted;
}

int alignTile(Tile * tile, UserData data) {
	if (TiGetType(tile) != Tile::SPACE) return 0;

	QMultiHash<Tile *, TileRect *> * tileRects  = (QMultiHash<Tile *, TileRect *> *) data;

	// handle four cases from ECO router http://www.cis.nctu.edu.tw/~ylli/paper/f69-li.pdf

	// case 1
	Tile * rt = RT(tile);
	if (TiGetType(rt) == Tile::SPACE && RIGHT(rt) > RIGHT(tile)) {
		bool shrink = true;
		// go along top edge
		for (Tile * tp = RT(rt); LEFT(tp) > RIGHT(tile); tp = BL(tp)) {
			if (TiGetType(tp) == Tile::SPACE && LEFT(tp) >= RIGHT(tile) && LEFT(tp) < RIGHT(rt)) {
				shrink = false;
				break;
			}
		}
		if (shrink) {
			// go along bottom edge
			for (Tile * tp = TR(tile); LEFT(tp) < RIGHT(rt); tp = TR(tp)) {
				if (TiGetType(tp) == Tile::SPACE && LEFT(tp) >= RIGHT(tile) && LEFT(tp) < RIGHT(rt)) {
					shrink = false;
					break;
				}
			}
		}

		if (shrink) {
			TileRect * tileRect = new TileRect;
			tileRect->xmini = RIGHT(tile);
			tileRect->xmaxi = RIGHT(rt);
			tileRect->ymaxi = YMAX(rt);
			tileRect->ymini = YMIN(rt);
			tileRects->insert(rt, tileRect);
		}
	}

	// case 2
	if (TiGetType(rt) != Tile::SPACE && RIGHT(rt) > RIGHT(tile)) {
		bool shrink = true;
		for (Tile * tp = LB(tile); LEFT(tp) < RIGHT(tile); tp = TR(tp)) {
			if (TiGetType(tp) == Tile::SPACE && LEFT(tp) >= LEFT(rt) && LEFT(tp) < RIGHT(tile)) {
				shrink = false;
				break;
			}
		}
		if (shrink) {
			TileRect * tileRect = new TileRect;
			tileRect->xmini = LEFT(tile);
			tileRect->xmaxi = LEFT(rt);
			tileRect->ymaxi = YMAX(tile);
			tileRect->ymini = YMIN(tile);
			tileRects->insert(rt, tileRect);
		}
	}

	// case 3
	Tile * lb = LB(tile);
	if (TiGetType(lb) == Tile::SPACE && LEFT(lb) < LEFT(tile)) {
		bool shrink = true;
		// go along top edge
		for (Tile * tp = TR(tile); RIGHT(tp) > LEFT(lb); tp = BL(tp)) {
			if (TiGetType(tp) == Tile::SPACE && RIGHT(tp) > LEFT(lb) && RIGHT(tp) < LEFT(tile)) {
				shrink = false;
				break;
			}
		}
		if (shrink) {
			// go along bottom edge
			for (Tile * tp = LB(lb); RIGHT(tp) < LEFT(tile); tp = TR(tp)) {
				if (TiGetType(tp) == Tile::SPACE && RIGHT(tp) > LEFT(lb) && RIGHT(tp) < LEFT(tile)) {
					shrink = false;
					break;
				}
			}
		}

		if (shrink) {
			TileRect * tileRect = new TileRect;
			tileRect->xmini = LEFT(tile);
			tileRect->xmaxi = RIGHT(lb);
			tileRect->ymaxi = YMAX(lb);
			tileRect->ymini = YMIN(lb);
			tileRects->insert(lb, tileRect);
		}
	}

	// case 4
	if (TiGetType(lb) != Tile::SPACE && LEFT(lb) < LEFT(tile)) {
		bool shrink = true;
		for (Tile * tp = RT(tile); RIGHT(tp) > LEFT(tile); tp = BL(tp)) {
			if (TiGetType(tp) == Tile::SPACE && RIGHT(tp) >= LEFT(tile) && RIGHT(tp) < RIGHT(lb)) {
				shrink = false;
				break;
			}
		}
		if (shrink) {
			TileRect * tileRect = new TileRect;
			tileRect->xmini = RIGHT(lb);
			tileRect->xmaxi = RIGHT(tile);
			tileRect->ymaxi = YMAX(tile);
			tileRect->ymini = YMIN(tile);
			tileRects->insert(lb, tileRect);
		}
	}


	return 0;
}

Plane * CMRouter::tilePlane(ItemBase * board, ViewLayer::ViewLayerID viewLayerID, QList<Tile *> & alreadyTiled) {
	TiSetChangedFunc(NULL);
	TiSetFreeFunc(NULL);

	Tile * bufferTile = TiAlloc();
	TiSetType(bufferTile, Tile::BUFFER);
	TiSetBody(bufferTile, NULL);

	if (board) {
		m_maxRect = board->boundingRect();
		m_maxRect.translate(board->pos());
	}
	else {
		m_maxRect = m_sketchWidget->scene()->itemsBoundingRect();
		m_maxRect.adjust(-m_maxRect.width() / 2, -m_maxRect.height() / 2, m_maxRect.width() / 2, m_maxRect.height() / 2);
	}

	realsToTile(m_tileMaxRect, m_maxRect.left(), m_maxRect.top(), m_maxRect.right(), m_maxRect.bottom()); 

	QRectF bufferRect(m_maxRect);
	bufferRect.adjust(-m_maxRect.width(), -m_maxRect.height(), m_maxRect.width(), m_maxRect.height());

	SETLEFT(bufferTile, realToTile(bufferRect.left()));
	SETYMIN(bufferTile, realToTile(bufferRect.top()));		// TILE is Math Y-axis not computer-graphic Y-axis

	Plane * thePlane = TiNewPlane(bufferTile);

	SETRIGHT(bufferTile, realToTile(bufferRect.right()));
	SETYMAX(bufferTile, realToTile(bufferRect.bottom()));		// TILE is Math Y-axis not computer-graphic Y-axis

	QList<Tile *> already;
	insertTile(thePlane, m_tileMaxRect, already, NULL, Tile::SPACE, CMRouter::IgnoreAllOverlaps);
	// if board is not rectangular, add tiles for the outside edges;

	if (!initBoard(board, thePlane, alreadyTiled)) return thePlane;

	if (m_sketchWidget->autorouteCheckConnectors()) {
		// deal with "rectangular" elements first
		foreach (QGraphicsItem * item, m_sketchWidget->scene()->items()) {
			// TODO: need to leave expansion area around coords?
			ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
			if (connectorItem != NULL) {
				if (!connectorItem->attachedTo()->isVisible()) continue;
				if (connectorItem->attachedTo()->hidden()) continue;
				if (connectorItem->attachedToItemType() == ModelPart::Wire) continue;
				if (!m_sketchWidget->sameElectricalLayer2(connectorItem->attachedToViewLayerID(), viewLayerID)) continue;

				//DebugDialog::debug(QString("coords connectoritem %1 %2 %3 %4 %5")
				//						.arg(connectorItem->connectorSharedID())
				//						.arg(connectorItem->connectorSharedName())
				//						.arg(connectorItem->attachedToTitle())
				//						.arg(connectorItem->attachedToID())
				//						.arg(connectorItem->attachedToInstanceTitle())
				//				);

				addTile(connectorItem, Tile::OBSTACLE, thePlane, alreadyTiled, CMRouter::ClipAllOverlaps);
				if (alreadyTiled.count() > 0) {
					return thePlane;
				}

				continue;
			}
		}
	}

	if (m_sketchWidget->autorouteCheckWires()) {
		// now insert the wires
		QList<Wire *> beenThere;
		foreach (QGraphicsItem * item, m_sketchWidget->scene()->items()) {
			Wire * wire = dynamic_cast<Wire *>(item);
			if (wire == NULL) continue;
			if (!wire->isVisible()) continue;
			if (wire->hidden()) continue;
			if (!wire->getTrace()) continue;
			if (!m_sketchWidget->sameElectricalLayer2(wire->viewLayerID(), viewLayerID)) continue;
			if (beenThere.contains(wire)) continue;

			tileWire(wire, thePlane, beenThere, alreadyTiled, Tile::OBSTACLE, CMRouter::ClipAllOverlaps);
			if (alreadyTiled.count() > 0) {
				return thePlane;
			}	
		}
	}

	if (m_sketchWidget->autorouteCheckConnectors()) {
		// deal with "rectangular" elements first
		foreach (QGraphicsItem * item, m_sketchWidget->scene()->items()) {
			// TODO: need to leave expansion area around coords?
			ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
			if (connectorItem != NULL) {
				continue;
			}

			NonConnectorItem * nonConnectorItem = dynamic_cast<NonConnectorItem *>(item);
			if (nonConnectorItem != NULL) {
				if (!nonConnectorItem->attachedTo()->isVisible()) continue;
				if (nonConnectorItem->attachedTo()->hidden()) continue;
				if (!m_sketchWidget->sameElectricalLayer2(connectorItem->attachedToViewLayerID(), viewLayerID)) continue;

				DebugDialog::debug(QString("coords nonconnectoritem %1 %2")
										.arg(nonConnectorItem->attachedToTitle())
										.arg(nonConnectorItem->attachedToID())
										);

				addTile(nonConnectorItem, Tile::OBSTACLE, thePlane, alreadyTiled, CMRouter::ClipAllOverlaps);
				if (alreadyTiled.count() > 0) {
					return thePlane;
				}

				continue;
			}
		}
	}

	if (m_sketchWidget->autorouteCheckParts()) {
		QList<Wire *> beenThere;
		foreach (QGraphicsItem * item, m_sketchWidget->scene()->items()) {
			ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
			if (itemBase == NULL) continue;

			if (itemBase->itemType() == ModelPart::Wire) {
				Wire * wire = dynamic_cast<Wire *>(item);
				if (wire == NULL) continue;
				if (!wire->isVisible()) continue;
				if (wire->hidden()) continue;
				if (!wire->getTrace()) continue;
				if (!m_sketchWidget->sameElectricalLayer2(wire->viewLayerID(), viewLayerID)) continue;
				if (beenThere.contains(wire)) continue;

				//tileWire(wire, thePlane, beenThere, alreadyTiledWithWireCrossing);
				if (alreadyTiled.count() > 0) {
					return thePlane;
				}

				continue;
			}

			//tilePart(itemBase, thePlane);
		}
	}

	//QMultiHash<Tile *, TileRect *> tileRects;
	//TiSrArea(NULL, thePlane, &m_tileMaxRect, alignTile, &tileRects);
	//makeAlignTiles(tileRects, thePlane);

	TiSetChangedFunc(CollectChangedTiles);
	TiSetFreeFunc(RemoveChangedTiles);

	return thePlane;
}

bool CMRouter::initBoard(ItemBase * board, Plane * thePlane, QList<Tile *> & alreadyTiled)
{
	if (board == NULL) return true;

	QHash<QString, SvgFileSplitter *> svgHash;
	QSizeF boardSize = board->size();
	ResizableBoard * resizableBoard = qobject_cast<ResizableBoard *>(board->layerKinChief());
	if (resizableBoard != NULL) {
		boardSize = resizableBoard->getSizeMM() * FSvgRenderer::printerScale() / 25.4;
	}
	QString svg = TextUtils::makeSVGHeader(FSvgRenderer::printerScale(), 
										   FSvgRenderer::printerScale(), 
										   boardSize.width(), 
										   boardSize.height());
	svg += board->retrieveSvg(ViewLayer::Board, svgHash, true, FSvgRenderer::printerScale());
	svg += board->retrieveSvg(ViewLayer::Silkscreen1, svgHash, true, FSvgRenderer::printerScale());
	svg += board->retrieveSvg(ViewLayer::Silkscreen0, svgHash, true, FSvgRenderer::printerScale());
	svg += "</svg>";
	GroundPlaneGenerator gpg;
	QList<QRect> rects;
	gpg.getBoardRects(svg, board, FSvgRenderer::printerScale(), KeepoutSpace, rects);
	QPointF boardPos = board->pos();
	foreach (QRect r, rects) {
		TileRect tileRect;
		realsToTile(tileRect, r.left() + boardPos.x(), r.top() + boardPos.y(), r.right() + boardPos.x(), r.bottom() + 1 + boardPos.y());  // note off-by-one weirdness
		Tile * tile = insertTile(thePlane, tileRect, alreadyTiled, NULL, Tile::OBSTACLE, CMRouter::IgnoreAllOverlaps);
		drawGridItem(tile);
	}

	return true;
}

bool clipRect(TileRect * r, TileRect * clip, QList<TileRect> & rects) {
	if (!tileRectIntersects(r, clip)) return false;

	if (r->ymini < clip->ymini)
	{
		TileRect s;
		s.xmini = r->xmini;
		s.ymini = r->ymini; 
		s.xmaxi = r->xmaxi;
		s.ymaxi = clip->ymini;
		rects.append(s);
		r->ymini = clip->ymini;
	}
	if (r->ymaxi > clip->ymaxi)
	{
		TileRect s;
		s.xmini = r->xmini;
		s.ymini = clip->ymaxi;
		s.xmaxi = r->xmaxi;
		s.ymaxi = r->ymaxi;
		rects.append(s);
		r->ymaxi = clip->ymaxi;
	}
	if (r->xmini < clip->xmini)
	{
		TileRect s;
		s.xmini = r->xmini;
		s.ymini = r->ymini;
		s.xmaxi = clip->xmini;
		s.ymaxi = r->ymaxi;
		rects.append(s);
		r->xmini = clip->xmini;
	}
	if (r->xmaxi > clip->xmaxi)
	{
		TileRect s;
		s.xmini = clip->xmaxi;
		s.ymini = r->ymini;
		s.xmaxi = r->xmaxi;
		s.ymaxi = r->ymaxi;
		rects.append(s);
		r->xmaxi = clip->xmaxi;
	}

	return true;
}


void CMRouter::tileWire(Wire * wire, Plane * thePlane, QList<Wire *> & beenThere, QList<Tile *> & alreadyTiled, Tile::TileType tileType, CMRouter::OverlapType overlapType) 
{
	DebugDialog::debug(QString("coords wire %1, x1:%2 y1:%3, x2:%4 y2:%5")
		.arg(wire->id())
		.arg(wire->pos().x())
		.arg(wire->pos().y())
		.arg(wire->line().p2().x())
		.arg(wire->line().p2().y()) );			


	QList<ConnectorItem *> ends;
	QList<Wire *> wires;
	wire->collectChained(wires, ends);
	beenThere.append(wires);
	if (ends.count() < 1) {
		// something is very wrong
		return;
	}

	tileWires(wires, thePlane, alreadyTiled, tileType, overlapType);
}

void CMRouter::tileWires(QList<Wire *> & wires, Plane * thePlane, QList<Tile *> & alreadyTiled, Tile::TileType tileType, CMRouter::OverlapType overlapType) 
{
	QMultiHash<Wire *, QRectF> wireRects;
	foreach (Wire * wire, wires) {
		QPointF p1 = wire->connector0()->sceneAdjustedTerminalPoint(NULL);
		QPointF p2 = wire->connector1()->sceneAdjustedTerminalPoint(NULL);
		qreal dx = qAbs(p1.x() - p2.x());
		qreal dy = qAbs(p1.y() - p2.y());
		if (dx < CloseEnough) {
			// vertical line
			qreal x = qMin(p1.x(), p2.x()) - (wire->width() / 2);
			qreal y = qMin(p1.y(), p2.y());			
			wireRects.insert(wire, QRectF(x, y, wire->width() + dx, dy));
		}
		else if (dy < CloseEnough) {
			// horizontal line
			qreal y =  qMin(p1.y(), p2.y()) - (wire->width() / 2);
			qreal x = qMin(p1.x(), p2.x());
			wireRects.insert(wire, QRectF(x, y, qMax(p1.x(), p2.x()) - x, wire->width() + dy));
		}
		else {
			qreal angle = atan2(p2.y() - p1.y(), p2.x() - p1.x());
			if (dy >= dx) {
				//sliceWireHorizontally(w, angle, p1, p2, rects);
			}
			else {
				//sliceWireVertically(w, angle, p1, p2, rects);
			}
		}
	}

	foreach (Wire * w, wireRects.uniqueKeys()) {
		foreach (QRectF r, wireRects.values(w)) {
			TileRect tileRect;
			realsToTile(tileRect, r.left() - KeepoutSpace, r.top() - KeepoutSpace, r.right() + KeepoutSpace, r.bottom() + KeepoutSpace);
			insertTile(thePlane, tileRect, alreadyTiled, w, tileType, overlapType);
			if (alreadyTiled.count() > 0) {
				return;
			}
		}
	}

}

void CMRouter::sliceWireVertically(Wire * w, qreal angle, QPointF p1, QPointF p2, QList<QRectF> & rects) {
	// tiler gets confused when horizontally contiguous tiles are the same type, so join rects horizontally
	qreal x, y, xend, yend;
	qreal slantWidth = qAbs((w->width() + KeepoutSpace + KeepoutSpace) / cos(angle));
	if (p1.x() < p2.x()) {
		x = p1.x();
		y = p1.y();
		xend = p2.x();
		yend = p2.y();
	}
	else {
		x = p2.x();
		y = p2.y();
		xend = p1.x();
		yend = p1.y();
	}

	int xunits = qCeil((xend - x) / Wire::STANDARD_TRACE_WIDTH);
	int miny = qFloor((qMin(y, yend) - (slantWidth / 2)) / Wire::STANDARD_TRACE_WIDTH);
	int maxy = qCeil((qMax(y, yend) + (slantWidth / 2)) / Wire::STANDARD_TRACE_WIDTH);
	int yunits = maxy - miny;
	QVector< QVector<char> > tiler(yunits, QVector<char>(xunits + 1, 0));
	
	qreal cx = x;
	for (int ix = 0; cx < xend; ix++) {
		qreal dx = cx - x;
		qreal dy = dx * (yend - y) / (xend - x);
		qreal y1 = y + dy - (slantWidth / 2);
		qreal y2 = y1 + slantWidth;
		int yi1 = qFloor(y1 / Wire::STANDARD_TRACE_WIDTH);
		int yi2 = qCeil(y2 / Wire::STANDARD_TRACE_WIDTH);
		for (int iy = yi1; iy < yi2; iy++) {
			tiler[iy - miny][ix] = 1;
		}
		cx += Wire::STANDARD_TRACE_WIDTH;
	}

	for (int iy = 0; iy < yunits; iy++) {
		bool inRect = false;
		int started;
		for (int ix = 0; ix < xunits + 1; ix++) {
			if (tiler[iy][ix] == 1) {
				if (!inRect) {
					inRect = true;
					started = ix;
				}
			}
			else {
				if (inRect) {
					inRect = false;
					QRectF r(x + (Wire::STANDARD_TRACE_WIDTH * started), 
							 (miny + iy) * Wire::STANDARD_TRACE_WIDTH,
							 (ix - started) * Wire::STANDARD_TRACE_WIDTH,
							 Wire::STANDARD_TRACE_WIDTH);
					rects.append(r);
				}
			}
		}
	}
}

void CMRouter::sliceWireHorizontally(Wire * w, qreal angle, QPointF p1, QPointF p2, QList<QRectF> & rects) {
	qreal x, y, xend, yend;
	qreal slantWidth = qAbs((w->width() + KeepoutSpace + KeepoutSpace) / sin(angle));
	if (p1.y() < p2.y()) {
		x = p1.x();
		y = p1.y();
		xend = p2.x();
		yend = p2.y();
	}
	else {
		x = p2.x();
		y = p2.y();
		xend = p1.x();
		yend = p1.y();
	}

	// begin somewhere on the pseudo-grid
	int ystart = qFloor(y / Wire::STANDARD_TRACE_WIDTH);
	for (qreal cy = ystart * Wire::STANDARD_TRACE_WIDTH; cy < yend; ystart++) {
		qreal dy = cy - y;
		qreal dx = dy * (xend - x) / (yend - y);
		QRectF r(x + dx - (slantWidth / 2), cy, slantWidth, Wire::STANDARD_TRACE_WIDTH);
		rects.append(r);
		cy += Wire::STANDARD_TRACE_WIDTH;
	}	
}

/*
void CMRouter::fixupJumperItems(QList<JEdge *> & edges, ItemBase * board) {
	if (edges.count() <= 0) return;

	int todo = 0;
	foreach (JEdge * edge, edges) {
		if (!edge->routed && (edge->linkedEdge && !edge->linkedEdge->routed)) {
			todo++;
		}
	}

	int jumpersDone = 0;
	foreach (JEdge * edge, edges) {
		if (edge->routed) continue;
		if (edge->linkedEdge) {
			if (edge->linkedEdge->routed) continue;
			if (edge->linkedEdge->jumperItem) continue;
		}

		drawJumperItem(edge, board);
		jumpersDone += ((edge->linkedEdge) ? 2 : 1);
		updateProgress(jumpersDone, todo);
	}
}
*/


ConnectorItem * CMRouter::splitTrace(Wire * wire, QPointF point, ItemBase * board) 
{
	// split the trace at point
	QLineF originalLine = wire->line();
	QLineF newLine(QPointF(0,0), point - wire->pos());
	wire->setLine(newLine);
	TraceWire * splitWire = drawOneTrace(point, originalLine.p2() + wire->pos(), wire->width(), wire->viewLayerSpec());
	ConnectorItem * connector1 = wire->connector1();
	ConnectorItem * newConnector1 = splitWire->connector1();
	foreach (ConnectorItem * toConnectorItem, connector1->connectedToItems()) {
		connector1->tempRemove(toConnectorItem, false);
		toConnectorItem->tempRemove(connector1, false);
		newConnector1->tempConnectTo(toConnectorItem, false);
		toConnectorItem->tempConnectTo(newConnector1, false);
	}

	connector1->tempConnectTo(splitWire->connector0(), false);
	splitWire->connector0()->tempConnectTo(connector1, false);

	if (board) {
		splitWire->addSticky(board, true);
		board->addSticky(splitWire, true);
	}

	if (!wire->getAutoroutable()) {
		// TODO: deal with undo
	}

	return splitWire->connector0();
}


void CMRouter::hookUpWires(JEdge * edge, QList<PathUnit *> & fullPath, QList<Wire *> & wires) {
	if (wires.count() <= 0) return;
		
	PathUnit * fromPathUnit = fullPath.first();
	PathUnit * toPathUnit = fullPath.last();
	if (fromPathUnit->connectorItem) {
		fromPathUnit->connectorItem->tempConnectTo(wires[0]->connector0(), true);
		wires[0]->connector0()->tempConnectTo(fromPathUnit->connectorItem, true);
	}
	else {
	}
	int last = wires.count() - 1;
	if (toPathUnit->connectorItem) {
		toPathUnit->connectorItem->tempConnectTo(wires[last]->connector1(), true);
		wires[last]->connector1()->tempConnectTo(toPathUnit->connectorItem, true);
	}
	else {
	}
	for (int i = 0; i < last; i++) {
		ConnectorItem * c1 = wires[i]->connector1();
		ConnectorItem * c0 = wires[i + 1]->connector0();
		c1->tempConnectTo(c0, true);
		c0->tempConnectTo(c1, true);
	}

	QList<Tile *> alreadyTiled;
	tileWires(wires, edge->plane, alreadyTiled, Tile::OBSTACLE, CMRouter::ClipAllOverlaps);

	//QMultiHash<Tile *, TileRect *> tileRects;
	//foreach (Tile * tile, ChangedTiles) {
		//alignTile(tile, &tileRects);
	//}
	//makeAlignTiles(tileRects, edge->plane);
}

struct CheckAlreadyStruct
{
	QList<Tile *> * alreadyTiled;
	QGraphicsItem * item;
	Tile::TileType type;
};

int checkAlready(Tile * tile, UserData data) {
	if (TiGetType(tile) == Tile::SPACE) return 0;
	if (TiGetType(tile) == Tile::BUFFER) return 0;

	CheckAlreadyStruct * checkAlreadyStruct = (CheckAlreadyStruct *) data;
	checkAlreadyStruct->alreadyTiled->append(tile);
	return 0;
}

struct Range 
{
	int rmin;
	int rmax;

	Range(int mi, int ma) {
		rmin = mi;
		rmax = ma;
	}
	Range() {}
};
/*
bool CMRouter::findShortcut(TileRect & tileRect, bool useX, bool targetGreater, JSubedge * subedge, QList<QPointF> & allPoints, int ix)
{
	QList<Range> spaces;
	Range space;
	if (useX) {
		space.rmin = tileRect.xmini;
		space.rmax = tileRect.xmaxi;
	}
	else {
		space.rmin = tileRect.ymini;
		space.rmax = tileRect.ymaxi;
	}
	if (tileToReal(space.rmax - space.rmin) < m_wireWidthNeeded) {
		return false;
	}

	spaces.append(space);
	Range cacheSpace(space.rmin, space.rmax);

	QList<Tile *> alreadyTiled;
	TiSrArea(NULL, subedge->edge->plane, &tileRect, simpleList, &alreadyTiled);
	foreach (Tile * tile, alreadyTiled) {
		short result = checkCandidate(subedge, tile, false);
		if (result < GridEntry::BLOCK) continue;

		Range range;
		if (useX) {
			range.rmin = LEFT(tile);
			range.rmax = RIGHT(tile);
		}
		else {
			range.rmin = YMIN(tile);
			range.rmax = YMAX(tile);
		}
		for (int sp = 0; sp < spaces.count(); sp++) {
			Range nextSpace = spaces.at(sp);
			if (nextSpace.rmax <= range.rmin) continue;
			if (range.rmax <= nextSpace.rmin) continue;

			if (range.rmin <= nextSpace.rmin && range.rmax >= nextSpace.rmax) {
				spaces.replace(sp, Range(0,0));
			}
			else if (range.rmin >= nextSpace.rmin && range.rmax <= nextSpace.rmax) {
				spaces.replace(sp, Range(nextSpace.rmin, range.rmin));
				spaces.append(Range(range.rmax, nextSpace.rmax));
			}
			else if (range.rmax > nextSpace.rmin) {
				spaces.replace(sp, Range(range.rmax, nextSpace.rmax));
			}
			else {
				spaces.replace(sp, Range(nextSpace.rmin, range.rmin));
			}
		}
	}

	bool success = false;
	qreal result;
	if (targetGreater) {
		result = std::numeric_limits<int>::min();
	}
	else {
		result = std::numeric_limits<int>::max();
	}
	foreach (Range range, spaces) {
		if (tileToReal(range.rmax - range.rmin) >= m_wireWidthNeeded) {
			if (targetGreater) {
				if (tileToReal(range.rmax) - m_halfWireWidthNeeded > result) {
					result = tileToReal(range.rmax) - m_halfWireWidthNeeded;
					success = true;
				}
			}
			else {
				if (tileToReal(range.rmin) + m_halfWireWidthNeeded < result) {
					result = tileToReal(range.rmin) + m_halfWireWidthNeeded;
					success = true;
				}
			}
		}
	}

	if (!success) return false;

	QPointF newP1 = allPoints.at(ix + 1);
	QPointF newP2 = allPoints.at(ix + 2);
	if (useX) {
		newP1.setX(result);
		newP2.setX(result);
	}
	else {
		newP1.setY(result);
		newP2.setY(result);
	}
	allPoints.replace(ix + 1, newP1);
	allPoints.replace(ix + 2, newP2);
	QPointF p0 = allPoints.at(ix + 0);
	QPointF p3 = allPoints.at(ix + 3);
	if (p0 == newP1) {
		allPoints.removeAt(ix + 1);
	}
	if (p3 == newP2) {
		allPoints.removeAt(ix + 2);
	}

	return true;
}
*/

/*
void CMRouter::shortenUs(QList<QPointF> & allPoints, JSubedge * subedge)
{
	// TODO: this could be implemented recursively as a child tile space 
	//		with the goals being the sides of the U-shape and the obstacles copied in from the parent tile space
	// for now just look for a straight line
	int ix = 0;
	while (ix < allPoints.count() - 3) {
		QPointF p0 = allPoints.at(ix);
		QPointF p1 = allPoints.at(ix + 1);
		QPointF p2 = allPoints.at(ix + 2);
		QPointF p3 = allPoints.at(ix + 3);
		ix += 1;

		TileRect tileRect;
		if (p1.x() == p2.x()) {
			if ((p0.x() > p1.x() && p3.x() > p2.x()) || (p0.x() < p1.x() && p3.x() < p2.x())) {
				// opening to left or right
				bool targetGreater;
				if (p0.x() < p1.x()) {
					// opening left
					targetGreater = false;
					realsToTile(tileRect, qMax(p0.x(), p3.x()), qMin(p0.y(), p3.y()), p2.x(), qMax(p0.y(), p3.y()));
				}
				else {
					// opening right
					targetGreater = true;
					realsToTile(tileRect, p2.x(), qMin(p0.y(), p3.y()), qMin(p0.x(), p3.x()), qMax(p0.y(), p3.y()));
				}

				if (findShortcut(tileRect, true, targetGreater, subedge, allPoints, ix - 1)) {
					ix--;
				}
			}
			else {
				// not a U-shape
				continue;
			}
		}
		else if (p1.y() == p2.y()) {
			if ((p0.y() > p1.y() && p3.y() > p2.y()) || (p0.y() < p1.y() && p3.y() < p2.y())) {
				// opening to top or bottom
				bool targetGreater;
				if (p0.y() < p1.y()) {
					// opening top
					targetGreater = false;
					realsToTile(tileRect, qMin(p0.x(), p3.x()), qMax(p0.y(), p3.y()), qMax(p0.x(), p3.x()), p2.y());
				}
				else {
					// opening bottom
					targetGreater = true;
					realsToTile(tileRect, qMin(p0.x(), p3.x()), p2.y(), qMax(p0.x(), p3.x()), qMin(p0.y(), p3.y()));
				}

				if (findShortcut(tileRect, false, targetGreater, subedge, allPoints, ix - 1)) {
					ix--;
				}
			}
			else {
				// not a U-shape
				continue;
			}
		}

	}
}
*/
/*
void CMRouter::removeCorners(QList<QPointF> & allPoints, JEdge * edge)
{
	int ix = 0;
	while (ix < allPoints.count() - 3) {
		QPointF p0 = allPoints.at(ix);
		QPointF p1 = allPoints.at(ix + 1);
		QPointF p2 = allPoints.at(ix + 2);
		QPointF p3 = allPoints.at(ix + 3);
		ix += 1;

		bool removeCorner = false;
		QPointF proposed;
		if (p0.y() == p1.y()) {
			if ((p0.x() < p1.x() && p1.x() < p3.x()) || (p0.x() > p1.x() && p1.x() > p3.x())) {
				// x must be monotonically increasing or decreasing
				// dogleg horizontal, vertical, horizontal
			}
			else {
				continue;
			}

			proposed.setX(p3.x());
			proposed.setY(p1.y());
			removeCorner = checkProposed(proposed, p1, p3, edge, ix == 1 || ix + 3 == allPoints.count());
			if (!removeCorner) {
				proposed.setX(p0.x());
				proposed.setY(p2.y());
				removeCorner = checkProposed(proposed, p0, p2, edge, ix == 1 || ix + 3 == allPoints.count());
			}
		}
		else if (p0.x() == p1.x()) {
			if ((p0.y() < p1.y() && p1.y() < p3.y()) || (p0.y() > p1.y() && p1.y() > p3.y())) {
				// y must be monotonically increasing or decreasing
				// dogleg vertical, horizontal, vertical
			}
			else {
				continue;
			}

			proposed.setY(p3.y());
			proposed.setX(p1.x());
			removeCorner = checkProposed(proposed, p1, p3, edge, ix == 1 || ix + 3 == allPoints.count());
			if (!removeCorner) {
				proposed.setY(p0.y());
				proposed.setX(p2.x());
				removeCorner = checkProposed(proposed, p0, p2, edge, ix == 1 || ix + 3 == allPoints.count());
			}
		}
		if (!removeCorner) continue;

		ix--;
		allPoints.replace(ix + 1, proposed);
		allPoints.removeAt(ix + 2);
		if (allPoints.count() > ix + 3) {
			allPoints.removeAt(ix + 2);
		}
	}
}

bool CMRouter::checkProposed(const QPointF & proposed, const QPointF & p1, const QPointF & p3, JEdge * edge, bool atStartOrEnd) 
{
	if (atStartOrEnd) {
		Tile * tile = TiSrPoint(NULL, edge->plane, realToTile(proposed.x()), realToTile(proposed.y()));
		Tile::TileType type = checkCandidate(edge, tile);
		if (type > GridEntry::IGNORE) {
			// don't want to draw traces within the target connector itself
			return false;
		}
	}

	QList<Tile *> alreadyTiled;
	TileRect tileRect;
	qreal x = proposed.x() - HalfStandardWireWidth;
	realsToTile(tileRect, x, qMin(p1.y(), p3.y()), x + Wire::STANDARD_TRACE_WIDTH, qMax(p1.y(), p3.y()));
	TiSrArea(NULL, edge->plane, &tileRect, simpleList, &alreadyTiled);
	qreal y = proposed.y() - HalfStandardWireWidth;
	realsToTile(tileRect, qMin(p1.x(), p3.x()), y, qMax(p1.x(), p3.x()), y + Wire::STANDARD_TRACE_WIDTH);
	TiSrArea(NULL, edge->plane, &tileRect, simpleList, &alreadyTiled);
	foreach (Tile * tile, alreadyTiled) {
		Tile::TileType type = checkCandidate(edge, tile);
		if (type >= GridEntry::BLOCK) return false;
	}

	return true;
}

Tile::TileType CMRouter::checkCandidate(JEdge * edge, Tile * tile) 
{	
	switch (TiGetType(tile)) {
		case Tile::SPACE:
			return GridEntry::EMPTY;

		case Tile::TINYSPACE:
			return GridEntry::BLOCK;

		case Tile::CONNECTOR:
			if (!m_sketchWidget->autorouteCheckConnectors()) {
				return GridEntry::IGNORE;
			}
	
			{
				ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(TiGetBody(tile));
				if (edge->fromConnectorItems.contains(connectorItem)) {
					return GridEntry::SAFE;			
				}

				if (edge->toConnectorItems.contains(connectorItem)) {
					return GridEntry::SAFE;			
				}
			}

			return GridEntry::BLOCK;

		case Tile::TRACE:
			if (!m_sketchWidget->autorouteCheckWires()) {
				return GridEntry::IGNORE;
			}

			{
				Wire * wire = dynamic_cast<Wire *>(TiGetBody(tile));
				if (edge->fromTraces.contains(wire)) {
					return GridEntry::SAFE;
				}

				if (edge->toTraces.contains(wire)) {
					return GridEntry::SAFE;
				}
			}

			return GridEntry::BLOCK;

		case Tile::PART:
			if (!m_sketchWidget->autorouteCheckParts()) {
				return GridEntry::IGNORE;
			}

			return GridEntry::BLOCK;

		case Tile::NONCONNECTOR:
		case Tile::NOTBOARD:
		case Tile::BUFFER:
		case Tile::CONTOUR:
			return GridEntry::BLOCK;

		default:
			// shouldn't happen:
			return GridEntry::IGNORE;
	}
}

*/
void CMRouter::appendIf(PathUnit * pathUnit, Tile * next, QList<Tile *> & tiles, QMultiHash<Tile *, PathUnit *> & tilePathUnits, PathUnit::Direction direction, int tWidthNeeded) 
{
	if (pathUnit->tile == next) {
		// just came from here
		return;
	}

	foreach (PathUnit * pu, tilePathUnits.values(next)) {
		if (pu->parent == pathUnit) {
			// we've already explored this path
			return;
		}
	}

	if (TiGetType(next) == Tile::BUFFER) {
		return;
	}

	//infoTile("    append if", next);

	bool horizontal = (direction == PathUnit::Left || direction == PathUnit::Right);

	bool bail = true;
	switch (TiGetType(next)) {
		case Tile::OBSTACLE:
			break;
		case Tile::SOURCE:
			bail = (TiGetType(pathUnit->tile) != Tile::DESTINATION); 
			break;
		case Tile::DESTINATION:
			bail = (TiGetType(pathUnit->tile) != Tile::SOURCE); 
			break;
		case Tile::SPACE:
			bail = false;
			break;
		default:	
			DebugDialog::debug("default tile type: shouldn't happen");
			return;
	}

	drawGridItem(next);
	if (bail) return;

	if (pathUnit->parent != NULL || pathUnit->connectorItem == NULL) {
		if (horizontal) {
			if (qMin(YMAX(pathUnit->tile), YMAX(next)) - qMax(YMIN(pathUnit->tile), YMIN(next)) < tWidthNeeded) return;
		}
		else {
			if (qMin(RIGHT(pathUnit->tile), RIGHT(next)) - qMax(LEFT(pathUnit->tile), LEFT(next)) < tWidthNeeded) return;
		}
	}
	else {
		if (horizontal) {
			if (qMin(pathUnit->minCostRect.ymaxi, YMAX(next)) - qMax(pathUnit->minCostRect.ymini, YMIN(next)) < tWidthNeeded) return;
		}
		else {
			if (qMin(pathUnit->minCostRect.xmaxi, RIGHT(next)) - qMax(pathUnit->minCostRect.xmini, LEFT(next)) < tWidthNeeded) return;
		}
	}

	tiles.append(next);
}

void CMRouter::seedNext(PathUnit * pathUnit, QList<Tile *> & tiles, QMultiHash<Tile *, PathUnit *> & tilePathUnits) {
	infoTile("seed next", pathUnit->tile);
	int tWidthNeeded = TileStandardWireWidth;
	if ((RIGHT(pathUnit->tile) < m_tileMaxRect.xmaxi) && (HEIGHT(pathUnit->tile) >= tWidthNeeded)) {
		Tile * next = TR(pathUnit->tile);
		appendIf(pathUnit, next, tiles, tilePathUnits, PathUnit::Right, tWidthNeeded);
		while (true) {
			next = LB(next);
			if (YMAX(next) <= YMIN(pathUnit->tile)) {
				break;
			}

			appendIf(pathUnit, next, tiles, tilePathUnits, PathUnit::Right, tWidthNeeded);
		}
	}

	if ((LEFT(pathUnit->tile) > m_tileMaxRect.xmini) && (HEIGHT(pathUnit->tile) >= tWidthNeeded)) {
		Tile * next = BL(pathUnit->tile);
		appendIf(pathUnit, next, tiles, tilePathUnits, PathUnit::Left, tWidthNeeded);
		while (true) {
			next = RT(next);
			if (YMIN(next) >= YMAX(pathUnit->tile)) {
				break;
			}

			appendIf(pathUnit, next, tiles, tilePathUnits, PathUnit::Left, tWidthNeeded);
		}
	}

	if ((YMAX(pathUnit->tile) < m_tileMaxRect.ymaxi) && (WIDTH(pathUnit->tile) >= tWidthNeeded)) {	
		Tile * next = RT(pathUnit->tile);
		appendIf(pathUnit, next, tiles, tilePathUnits, PathUnit::Down, tWidthNeeded);
		while (true) {
			next = BL(next);
			if (RIGHT(next) <= LEFT(pathUnit->tile)) {
				break;
			}

			appendIf(pathUnit, next, tiles, tilePathUnits, PathUnit::Down, tWidthNeeded);
		}
	}

	if ((YMIN(pathUnit->tile) > m_tileMaxRect.ymini) && (WIDTH(pathUnit->tile) >= tWidthNeeded)) {		
		Tile * next = LB(pathUnit->tile);
		appendIf(pathUnit, next, tiles, tilePathUnits, PathUnit::Up, tWidthNeeded);
		while (true) {
			next = TR(next);
			if (LEFT(next) >= RIGHT(pathUnit->tile)) {
				break;
			}

			appendIf(pathUnit, next, tiles, tilePathUnits, PathUnit::Up, tWidthNeeded);
		}
	}
}

struct EmptyThing {
	QList<int> x_s;
	QList<int> x2_s;
	QList<int> y_s;
	int maxY;
};

int collectXandY(Tile * tile, UserData data) {
	EmptyThing * emptyThing = (EmptyThing *) data;
	Tile::TileType type = TiGetType(tile);
	if (type == Tile::SPACE) {
		if (!emptyThing->x_s.contains(LEFT(tile))) {
			emptyThing->x_s.append(LEFT(tile));
		}
		if (!emptyThing->x2_s.contains(RIGHT(tile))) {
			emptyThing->x2_s.append(RIGHT(tile));
		}
		if (YMIN(tile) <= emptyThing->maxY && !emptyThing->y_s.contains(YMIN(tile))) {
			emptyThing->y_s.append(YMIN(tile));
		}
	}

	return 0;
}

int allEmpty(Tile * tile, UserData) {
	Tile::TileType type = TiGetType(tile);
	if (type == Tile::SPACE) {
		return 0;
	}

	return 1;		// not empty; will stop the search
}

/*
short CMRouter::checkSpace(JSubedge * subedge, Tile * tile, bool forEmpty) 
{
	short result = TiGetType(tile) == SPACE ? GridEntry::EMPTY : GridEntry::TINY;
	return result;

	TileRect tileRect;
	TiToRect(tile, &tileRect);

	QSizeF sizeNeeded = m_sketchWidget->jumperItemSize();
	int tWidthNeeded = realToTile(sizeNeeded.width() + KeepoutSpace + KeepoutSpace);
	int tHeightNeeded = realToTile(sizeNeeded.height() + KeepoutSpace + KeepoutSpace);
	if (tileRect.xmaxi - tileRect.xmini  < tWidthNeeded) {
		return result;
	}

	// need to figure out how to minimize the distance between this space and the previous wire;
	// need to save the set of open spaces found....

	if (tileRect.ymaxi - tileRect.ymini  >= tHeightNeeded) {
		DebugDialog::debug("empty GOAL");
		return GridEntry::GOAL;
	}

	TileRect searchRect = tileRect;
	searchRect.ymini = qMax(realToTile(m_maxRect.top()), tileRect.ymini - tHeightNeeded + (tileRect.ymaxi - tileRect.ymini));
	searchRect.ymaxi = qMin(realToTile(m_maxRect.bottom()), tileRect.ymini + tHeightNeeded);

	EmptyThing emptyThing;
	emptyThing.maxY = tileRect.ymini;
	TiSrArea(tile, subedge->edge->plane, &searchRect, collectXandY, &emptyThing);

	foreach (int y, emptyThing.y_s) {
		foreach (int x, emptyThing.x_s) {
			searchRect.xmini = x;
			searchRect.xmaxi = x + tWidthNeeded;
			searchRect.ymini = y;
			searchRect.ymaxi = y + tHeightNeeded;
			if (TiSrArea(tile, subedge->edge->plane, &searchRect, allEmpty, NULL) == 0) {
				return GridEntry::GOAL;
			}
		}
	}



	return result;


}
*/


GridEntry * CMRouter::drawGridItem(Tile * tile)
{
	if (tile == NULL) return NULL;

	QRectF r;
	tileToQRect(tile, r);

	int alpha = 128;
	GridEntry * gridEntry = TiGetGridEntry(tile);
	if (gridEntry == NULL) {
		gridEntry = new GridEntry(r, NULL);
		gridEntry->setZValue(m_sketchWidget->getTopZ());
		TiSetClient(tile, gridEntry);
	}
	else {
		gridEntry->setRect(r);
	}

	QColor c;
	switch (TiGetType(tile)) {
		case Tile::SPACE:
			c = QColor(255, 255, 0, alpha);
			break;
		case Tile::SOURCE:
			c = QColor(0, 255, 0, alpha);
			break;
		case Tile::DESTINATION:
			c = QColor(0, 0, 255, alpha);
			break;
		case Tile::OBSTACLE:
			c = QColor(80, 80, 80, alpha);
			break;
		default:
			c = QColor(255, 0, 0, alpha);
			break;
	}

	gridEntry->setPen(c);
	gridEntry->setBrush(QBrush(c));
	if (gridEntry->scene() == NULL) {
		m_sketchWidget->scene()->addItem(gridEntry);
	}
	gridEntry->show();
	ProcessEventBlocker::processEvents();
	return gridEntry;
}


void CMRouter::collectEdges(QList<JEdge *> & edges, Plane * plane0, Plane * plane1, ViewLayer::ViewLayerID copper0, ViewLayer::ViewLayerID copper1)
{
	foreach (QGraphicsItem * item, m_sketchWidget->scene()->items()) {
		VirtualWire * vw = dynamic_cast<VirtualWire *>(item);
		if (vw == NULL) continue;

		ConnectorItem * from = vw->connector0()->firstConnectedToIsh();
		ConnectorItem * otherFrom = from->getCrossLayerConnectorItem();
		if (!m_sketchWidget->sameElectricalLayer2(copper0, from->attachedToViewLayerID())) {
			ConnectorItem * temp = from;
			from = otherFrom;
			otherFrom = temp;
		}
		if (!from) {
			continue;
		}

		ConnectorItem * to = vw->connector1()->firstConnectedToIsh();
		ConnectorItem * otherTo = to->getCrossLayerConnectorItem();
		if (!m_sketchWidget->sameElectricalLayer2(copper0, to->attachedToViewLayerID())) {
			ConnectorItem * temp = to;
			to = otherTo;
			otherTo = temp;
		}
		if (!to) {
			continue;
		}

		JEdge * edge0 = makeEdge(from, to, ViewLayer::Bottom, copper0, plane0, vw);
		edge0->id = edges.count();
		edges.append(edge0);
		if (m_bothSidesNow && otherFrom != NULL && otherTo != NULL) {
			JEdge * edge1 = makeEdge(otherFrom, otherTo, ViewLayer::Top, copper1, plane1, vw);
			edge1->id = edges.count();
			edges.append(edge1);
			edge0->linkedEdge = edge1;
			edge1->linkedEdge = edge0;
		}
	}
}

JEdge * CMRouter::makeEdge(ConnectorItem * from, ConnectorItem * to, 
						  ViewLayer::ViewLayerSpec viewLayerSpec, ViewLayer::ViewLayerID viewLayerID, 
						  Plane * plane, VirtualWire * vw) {
	JEdge * edge = new JEdge;
	edge->linkedEdge = NULL;
	edge->from = from;
	edge->to = to;
	edge->viewLayerSpec = viewLayerSpec;
	edge->viewLayerID = viewLayerID;
	edge->plane = plane;
	edge->routed = false;
	edge->vw = vw;
	edge->jumperItem = NULL;
	QPointF pi = from->sceneAdjustedTerminalPoint(NULL);
	QPointF pj = to->sceneAdjustedTerminalPoint(NULL);
	double px = pi.x() - pj.x();
	double py = pi.y() - pj.y();
	edge->distance = (px * px) + (py * py);
	return edge;
}


void CMRouter::clearTraces(PCBSketchWidget * sketchWidget, bool deleteAll, QUndoCommand * parentCommand) {
	QList<Wire *> oldTraces;
	QList<JumperItem *> oldJumperItems;
	if (sketchWidget->usesJumperItem()) {
		foreach (QGraphicsItem * item, sketchWidget->scene()->items()) {
			JumperItem * jumperItem = dynamic_cast<JumperItem *>(item);
			if (jumperItem == NULL) continue;

			if (deleteAll || jumperItem->autoroutable()) {
				oldJumperItems.append(jumperItem);

				// now deal with the traces connecting the jumperitem to the part
				QList<ConnectorItem *> both;
				foreach (ConnectorItem * ci, jumperItem->connector0()->connectedToItems()) both.append(ci);
				foreach (ConnectorItem * ci, jumperItem->connector1()->connectedToItems()) both.append(ci);
				foreach (ConnectorItem * connectorItem, both) {
					Wire * w = dynamic_cast<Wire *>(connectorItem->attachedTo());
					if (w == NULL) continue;

					if (w->getTrace()) {
						QList<Wire *> wires;
						QList<ConnectorItem *> ends;
						w->collectChained(wires, ends);
						foreach (Wire * wire, wires) {
							wire->setAutoroutable(true);
						}
					}
				}
			}
		}
	}

	foreach (QGraphicsItem * item, sketchWidget->scene()->items()) {
		Wire * wire = dynamic_cast<Wire *>(item);
		if (wire != NULL) {		
			if (wire->getTrace()) {
				if (deleteAll || wire->getAutoroutable()) {
					oldTraces.append(wire);
				}
			}
			/*
			else if (wire->getRatsnest()) {
				if (parentCommand) {
					sketchWidget->makeChangeRoutedCommand(wire, false, sketchWidget->getRatsnestOpacity(false), parentCommand);
				}
				wire->setRouted(false);
				wire->setOpacity(sketchWidget->getRatsnestOpacity(false));	
			}
			*/
			continue;
		}

	}

	if (parentCommand) {
		addUndoConnections(sketchWidget, false, oldTraces, parentCommand);
		foreach (Wire * wire, oldTraces) {
			sketchWidget->makeDeleteItemCommand(wire, BaseCommand::SingleView, parentCommand);
		}
		foreach (JumperItem * jumperItem, oldJumperItems) {
			sketchWidget->makeDeleteItemCommand(jumperItem, BaseCommand::CrossView, parentCommand);
		}
	}

	
	foreach (Wire * wire, oldTraces) {
		sketchWidget->deleteItem(wire, true, false, false);
	}
	foreach (JumperItem * jumperItem, oldJumperItems) {
		sketchWidget->deleteItem(jumperItem, true, true, false);
	}
}

/*
JumperItem * CMRouter::drawJumperItem(JEdge * edge, ItemBase * board) 
{
	// TODO: check across planes...

	QPointF fp = edge->from->sceneAdjustedTerminalPoint(NULL);
	QPointF tp = edge->to->sceneAdjustedTerminalPoint(NULL);

	QList<JSubedge *> fromSubedges, toSubedges;
	foreach (ConnectorItem * from, edge->fromConnectorItems) {
		QPointF p1 = from->sceneAdjustedTerminalPoint(NULL);
		fromSubedges.append(makeSubedge(edge, p1, from, NULL, tp, true));
	}
	foreach (Wire * from, edge->fromTraces) {
		QPointF p1 = from->connector0()->sceneAdjustedTerminalPoint(NULL);
		QPointF p2 = from->connector1()->sceneAdjustedTerminalPoint(NULL);
		fromSubedges.append(makeSubedge(edge, (p1 + p2) / 2, NULL, from, tp, true));
	}
	// reverse direction
	foreach (ConnectorItem * to, edge->toConnectorItems) {
		QPointF p1 = to->sceneAdjustedTerminalPoint(NULL);
		toSubedges.append(makeSubedge(edge, p1, to, NULL, fp, false));
	}
	foreach (Wire * to, edge->toTraces) {
		QPointF p1 = to->connector0()->sceneAdjustedTerminalPoint(NULL);
		QPointF p2 = to->connector1()->sceneAdjustedTerminalPoint(NULL);
		toSubedges.append(makeSubedge(edge, (p1 + p2) / 2, NULL, to, fp, false));
	}

	DebugDialog::debug(QString("\n\nedge from %1 %2 %3 to %4 %5 %6, %7")
		.arg(edge->from->attachedToTitle())
		.arg(edge->from->attachedToID())
		.arg(edge->from->connectorSharedID())
		.arg(edge->to->attachedToTitle())
		.arg(edge->to->attachedToID())
		.arg(edge->to->connectorSharedID())
		.arg(edge->distance) );

	QList<Wire *> fromWires;
	QList<Wire *> toWires;
	JSubedge * fromSubedge = NULL;
	JSubedge * toSubedge = NULL;
	Tile * fromTile = NULL;
	Tile * toTile = NULL;

	foreach (JSubedge * subedge, fromSubedges) {
		fromTile = drawTrace(subedge, fromWires, true); 
		if (fromTile) {
			fromSubedge = subedge;
			break;
		}
	}

	hideTiles();

	// TODO: two jumper items may try to share the same tiles
	// so should insert the tiles from the first jumper now

	if (fromSubedge != NULL) {
		foreach (JSubedge * subedge, toSubedges) {
			toTile = drawTrace(subedge, toWires, true);
			if (toTile) {
				toSubedge = subedge;
				break;
			}
		}
	}

	hideTiles();

	if (fromSubedge != NULL && toSubedge != NULL) {
		long newID = ItemBase::getNextID();
		ViewGeometry viewGeometry;
		ItemBase * itemBase = m_sketchWidget->addItem(m_sketchWidget->paletteModel()->retrieveModelPart(ModuleIDNames::jumperModuleIDName), 
												  edge->from->attachedTo()->viewLayerSpec(), 
												  BaseCommand::SingleView, viewGeometry, newID, -1, NULL, NULL);
		if (itemBase == NULL) {
			// we're in trouble
			return NULL;
		}

		JumperItem * jumperItem = dynamic_cast<JumperItem *>(itemBase);

		QPointF fromDestPoint = fromWires.last()->connector1()->sceneAdjustedTerminalPoint(NULL);
		QPointF toDestPoint = toWires.last()->connector1()->sceneAdjustedTerminalPoint(NULL);

		QSizeF sizeNeeded = m_sketchWidget->jumperItemSize();
		qreal widthNeeded = sizeNeeded.width() + KeepoutSpace + KeepoutSpace;
		qreal heightNeeded = sizeNeeded.height() + KeepoutSpace + KeepoutSpace;

		fromDestPoint = findNearestSpace(fromTile, widthNeeded, heightNeeded, edge->plane, fromDestPoint);
		toDestPoint = findNearestSpace(toTile, widthNeeded, heightNeeded, edge->plane, toDestPoint); 
		jumperItem->resize(fromDestPoint, toDestPoint);

		if (board) {
			jumperItem->addSticky(board, true);
			board->addSticky(jumperItem, true);
		}

		m_sketchWidget->scene()->addItem(jumperItem);
		fromSubedge->toConnectorItem = jumperItem->connector0();
		QList<Tile *> already;
		addTile(jumperItem->connector0(), CONNECTOR, edge->plane, already);
		hookUpWires(fromSubedge, fromWires);

		toSubedge->toConnectorItem = jumperItem->connector1();
		addTile(jumperItem->connector1(), CONNECTOR, edge->plane, already);
		hookUpWires(toSubedge, toWires);
		edge->jumperItem = jumperItem;
	}
	else {
		foreach (Wire * wire, fromWires) {
			delete wire;
		}
		foreach (Wire * wire, toWires) {
			delete wire;
		}
	}

	foreach (JSubedge * subedge, fromSubedges) {
		delete subedge;
	}
	fromSubedges.clear();

	foreach (JSubedge * subedge, toSubedges) {
		delete subedge;
	}
	toSubedges.clear();

	return edge->jumperItem;
}
*/

void CMRouter::restoreOriginalState(QUndoCommand * parentCommand) {
	QUndoStack undoStack;
	QList<JEdge *> edges;
	addToUndo(parentCommand, edges);
	undoStack.push(parentCommand);
	undoStack.undo();
}

void CMRouter::addToUndo(Wire * wire, QUndoCommand * parentCommand) {
	if (!wire->getAutoroutable()) {
		// it was here before the autoroute, so don't add it again
		return;
	}

	AddItemCommand * addItemCommand = new AddItemCommand(m_sketchWidget, BaseCommand::SingleView, ModuleIDNames::wireModuleIDName, wire->viewLayerSpec(), wire->getViewGeometry(), wire->id(), false, -1, parentCommand);
	new CheckStickyCommand(m_sketchWidget, BaseCommand::SingleView, wire->id(), false, CheckStickyCommand::RemoveOnly, parentCommand);
	
	new WireWidthChangeCommand(m_sketchWidget, wire->id(), wire->width(), wire->width(), parentCommand);
	new WireColorChangeCommand(m_sketchWidget, wire->id(), wire->colorString(), wire->colorString(), wire->opacity(), wire->opacity(), parentCommand);
	addItemCommand->turnOffFirstRedo();
}

void CMRouter::addToUndo(QUndoCommand * parentCommand, QList<JEdge *> & edges) 
{
	QList<Wire *> wires;
	foreach (QGraphicsItem * item, m_sketchWidget->items()) {
		TraceWire * wire = dynamic_cast<TraceWire *>(item);
		if (wire != NULL) {
			m_sketchWidget->setClipEnds(wire, true);
			wire->update();
			if (wire->getAutoroutable()) {
				wire->setWireWidth(Wire::STANDARD_TRACE_WIDTH, m_sketchWidget);
			}
			addToUndo(wire, parentCommand);
			wires.append(wire);
			continue;
		}
	}

	foreach (JEdge * edge, edges) {	
		JumperItem * jumperItem = edge->jumperItem;
		if (jumperItem == NULL) continue;

		jumperItem->saveParams();
		QPointF pos, c0, c1;
		jumperItem->getParams(pos, c0, c1);

		new AddItemCommand(m_sketchWidget, BaseCommand::CrossView, ModuleIDNames::jumperModuleIDName, jumperItem->viewLayerSpec(), jumperItem->getViewGeometry(), jumperItem->id(), false, -1, parentCommand);
		new ResizeJumperItemCommand(m_sketchWidget, jumperItem->id(), pos, c0, c1, pos, c0, c1, parentCommand);
		new CheckStickyCommand(m_sketchWidget, BaseCommand::SingleView, jumperItem->id(), false, CheckStickyCommand::RemoveOnly, parentCommand);

		m_sketchWidget->createWire(jumperItem->connector0(), edge->from, ViewGeometry::NoFlag, false, BaseCommand::CrossView, parentCommand);
		m_sketchWidget->createWire(jumperItem->connector1(), edge->to, ViewGeometry::NoFlag, false, BaseCommand::CrossView, parentCommand);

	}

	addUndoConnections(m_sketchWidget, true, wires, parentCommand);
}

void CMRouter::addUndoConnections(PCBSketchWidget * sketchWidget, bool connect, QList<Wire *> & wires, QUndoCommand * parentCommand) 
{
	foreach (Wire * wire, wires) {
		if (!wire->getAutoroutable()) {
			// since the autorouter didn't change this wire, don't add undo connections
			continue;
		}

		ConnectorItem * connector1 = wire->connector1();
		foreach (ConnectorItem * toConnectorItem, connector1->connectedToItems()) {
			ChangeConnectionCommand * ccc = new ChangeConnectionCommand(sketchWidget, BaseCommand::SingleView, toConnectorItem->attachedToID(), toConnectorItem->connectorSharedID(),
												wire->id(), connector1->connectorSharedID(),
												ViewLayer::specFromID(wire->viewLayerID()),
												connect, parentCommand);
			ccc->setUpdateConnections(false);
		}
		ConnectorItem * connector0 = wire->connector0();
		foreach (ConnectorItem * toConnectorItem, connector0->connectedToItems()) {
			ChangeConnectionCommand * ccc = new ChangeConnectionCommand(sketchWidget, BaseCommand::SingleView, toConnectorItem->attachedToID(), toConnectorItem->connectorSharedID(),
												wire->id(), connector0->connectorSharedID(),
												ViewLayer::specFromID(wire->viewLayerID()),
												connect, parentCommand);
			ccc->setUpdateConnections(false);
		}
	}
}


void CMRouter::clearEdges(QList<JEdge *> & edges) {
	foreach (JEdge * edge, edges) {
		if (edge->jumperItem) {
			m_sketchWidget->deleteItem(edge->jumperItem->id(), true, false, false);
		}
		delete edge;
	}
	edges.clear();
}

void CMRouter::doCancel(QUndoCommand * parentCommand) {
	clearTraces(m_sketchWidget, false, NULL);
	restoreOriginalState(parentCommand);
	cleanUp();
}

void CMRouter::updateProgress(int num, int denom) 
{
	emit setProgressValue((int) MaximumProgress * (m_currentProgressPart + (num / (qreal) denom)) / (qreal) m_maximumProgressPart);
}

Tile * CMRouter::addTile(NonConnectorItem * nci, Tile::TileType type, Plane * thePlane, QList<Tile *> & alreadyTiled, CMRouter::OverlapType overlapType) 
{
	QRectF r = nci->attachedTo()->mapRectToScene(nci->rect());
	TileRect tileRect;
	realsToTile(tileRect, r.left(), r.top(), r.right(), r.bottom());
	TileRect cl;
	cl.xmini = tileRect.xmini - TileKeepoutSpace;
	cl.xmaxi = tileRect.xmaxi + TileKeepoutSpace;
	cl.ymini = tileRect.ymini - TileKeepoutSpace;
	cl.ymaxi = tileRect.ymaxi + TileKeepoutSpace;
	Tile * tile = insertTile(thePlane, cl, alreadyTiled, nci, type, overlapType);
	//drawGridItem(tile);
	return tile;
}

int prepDeleteTile(Tile * tile, UserData data) {
	switch(TiGetType(tile)) {
		case Tile::DUMMYLEFT:
		case Tile::DUMMYRIGHT:
		case Tile::DUMMYTOP:
		case Tile::DUMMYBOTTOM:
			return 0;
	}

	//infoTile("prep delete", tile);
	QSet<Tile *> * tiles = (QSet<Tile *> *) data;
	tiles->insert(tile);

	return 0;
}


void CMRouter::hideTiles() 
{
	foreach (QGraphicsItem * item, m_sketchWidget->items()) {
		GridEntry * gridEntry = dynamic_cast<GridEntry *>(item);
		if (gridEntry) gridEntry->setVisible(false);
	}
}

void CMRouter::clearTiles(Plane * thePlane) 
{
	if (thePlane == NULL) return;

	foreach (QGraphicsItem * item, m_sketchWidget->items()) {
		GridEntry * gridEntry = dynamic_cast<GridEntry *>(item);
		if (gridEntry) delete gridEntry;
	}

	QSet<Tile *> tiles;
	TiSrArea(NULL, thePlane, &m_tileMaxRect, prepDeleteTile, &tiles);
	foreach (Tile * tile, tiles) {
		TiFree(tile);
	}

	TiFreePlane(thePlane);
}

void CMRouter::displayBadTiles(QList<Tile *> & alreadyTiled) {
	foreach (Tile * tile, alreadyTiled) {
		drawGridItem(tile); 
	}
}

Tile * CMRouter::insertTiny(Plane * thePlane, TileRect & tinyRect)
{
	Tile * tile = TiInsertTile(thePlane, &tinyRect, NULL, Tile::OBSTACLE);
	//drawGridItem(tile);
	return tile;
}

Tile * CMRouter::insertTile(Plane * thePlane, TileRect & tileRect, QList<Tile *> & alreadyTiled, QGraphicsItem * item, Tile::TileType tileType, CMRouter::OverlapType overlapType) 
{
	infoTileRect("insert tile", tileRect);

	bool gotOverlap = false;
	bool doClip = false;
	if (overlapType != CMRouter::IgnoreAllOverlaps) {
		CheckAlreadyStruct checkAlreadyStruct;
		checkAlreadyStruct.alreadyTiled = &alreadyTiled;
		checkAlreadyStruct.type = tileType;
		checkAlreadyStruct.item = item;
		TiSrArea(NULL, thePlane, &tileRect, checkAlready, &checkAlreadyStruct);
		if (alreadyTiled.count() > 0) {	
			switch(overlapType) {
				case CMRouter::ReportAllOverlaps:
					gotOverlap = true;
					break;
				case CMRouter::ClipAllOverlaps:
					doClip = overlapsOnly(item, alreadyTiled);
					break;
			}
		}
	}

	if (gotOverlap) {
		DebugDialog::debug("!!!!!!!!!!!!!!!!!!!!!!! overlaps not allowed !!!!!!!!!!!!!!!!!!!!!!");
		return NULL;
	}
	Tile * newTile = NULL;
	if (doClip) {
		clipInsertTile(thePlane, tileRect, alreadyTiled, item, tileType);
	}
	else {
		newTile = TiInsertTile(thePlane, &tileRect, item, tileType);
	}

	//drawGridItem(newTile);
	return newTile;
}

bool CMRouter::overlapsOnly(QGraphicsItem *, QList<Tile *> & alreadyTiled)
{
	bool doClip = false;
	for (int i = alreadyTiled.count() - 1;  i >= 0; i--) {
		Tile * intersectingTile = alreadyTiled.at(i);
		if (dynamic_cast<Wire *>(TiGetBody(intersectingTile)) != NULL || dynamic_cast<ConnectorItem *>(TiGetBody(intersectingTile)) != NULL) {
			doClip = true;
			continue;
		}

		alreadyTiled.removeAt(i);
	}

	return doClip;

	/*
	QList<ConnectorItem *> equipotential;
	Wire * w = dynamic_cast<Wire *>(item);
	if (w) {
		equipotential.append(w->connector0());
	}
	else {
		ConnectorItem * ci = dynamic_cast<ConnectorItem *>(item);
		equipotential.append(ci);
	}
	ConnectorItem::collectEqualPotential(equipotential, false, ViewGeometry::NoFlag);
		
	foreach (Tile * intersectingTile, alreadyTiled) {
		QGraphicsItem * bodyItem = TiGetBody(intersectingTile);
		ConnectorItem * ci = dynamic_cast<ConnectorItem *>(bodyItem);
		if (ci != NULL) {
			if (!equipotential.contains(ci)) {
				// overlap not allowed
				infoTile("intersecting", intersectingTile);
				return false;
			}
		}
		else {
			Wire * w = dynamic_cast<Wire *>(bodyItem);
			if (w == NULL) return false;

			if (!equipotential.contains(w->connector0())) {
				// overlap not allowed
				infoTile("intersecting", intersectingTile);
				return false;
			}
		}
	}

	return true;

	*/
}


void CMRouter::clipInsertTile(Plane * thePlane, TileRect & tileRect, QList<Tile *> & alreadyTiled, QGraphicsItem * item, Tile::TileType type) 
{
	//infoTileRect("clip insert", tileRect);

	QList<TileRect> tileRects;
	tileRects.append(tileRect);
	int ix = 0;
	while (ix < tileRects.count()) {
		bool clipped = false;
		TileRect * r = &tileRects[ix++];
		foreach (Tile * intersectingTile, alreadyTiled) {
			TileRect intersectingRect;
			TiToRect(intersectingTile, &intersectingRect);

			if (clipRect(r, &intersectingRect, tileRects)) {
				clipped = true;
				break;
			}
		}
		if (clipped) continue;

		//Tile * newTile = 
			TiInsertTile(thePlane, r, item, type);
		//drawGridItem(newTile);

	}

	alreadyTiled.clear();
}

void CMRouter::clearGridEntries() {
	foreach (QGraphicsItem * item, m_sketchWidget->scene()->items()) {
		GridEntry * gridEntry = dynamic_cast<GridEntry *>(item);
		if (gridEntry == NULL) continue;

		delete gridEntry;
	}
}

void CMRouter::reorderEdges(QList<JEdge *> & edges, QHash<Wire *, JEdge *> & tracesToEdges) {
	int ix = 0;
	while (ix < edges.count()) {
		JEdge * edge = edges.at(ix++);
		if (edge->routed) continue;
		if (edge->linkedEdge && edge->linkedEdge->routed) continue;

		int minIndex = edges.count();
		foreach (QGraphicsItem * item, m_sketchWidget->scene()->collidingItems(edge->vw)) {
			TraceWire * traceWire = dynamic_cast<TraceWire *>(item);
			if (traceWire == NULL) continue;

			JEdge * tedge = tracesToEdges.value(traceWire, NULL);
			if (tedge != NULL) {
				int tix = edges.indexOf(tedge);
				minIndex = qMin(tix, minIndex);
			}
		}

		if (minIndex < ix) {
			edges.removeOne(edge);
			edges.insert(minIndex, edge);
		}
	}
}

void CMRouter::initPathUnit(JEdge * edge, Tile * tile, PriorityQueue<PathUnit *> & pq, QMultiHash<Tile *, PathUnit *> & tilePathUnits)
{	
	PathUnit * pathUnit = new PathUnit(&pq);

	ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(TiGetBody(tile));
	if (connectorItem) {
		pathUnit->connectorItem = connectorItem;
		QPointF p = connectorItem->sceneAdjustedTerminalPoint(NULL);
		realsToTile(pathUnit->minCostRect, p.x() - HalfStandardWireWidth, p.y() - HalfStandardWireWidth, p.x() + HalfStandardWireWidth, p.y() + HalfStandardWireWidth);
	}
	else {
		Wire * wire = dynamic_cast<Wire *>(TiGetBody(tile));
		if (wire == NULL) {
			// shouldn't be here
			delete pathUnit;
			return;
		}

		pathUnit->wire = wire;
		QLineF line = wire->line();
		QPointF p1 = wire->pos() + line.p1();
		QPointF p2 = wire->pos() + line.p2();
		TileRect tileRect;
		TiToRect(tile, &tileRect);
		if (qAbs(p1.x() - p2.x()) < CloseEnough) {
			// vertical
			pathUnit->minCostRect.ymini = tileRect.ymini;
			pathUnit->minCostRect.ymaxi = tileRect.ymaxi;
			pathUnit->minCostRect.xmini = realToTile(qMin(p1.x(), p2.x()) - HalfStandardWireWidth);
			pathUnit->minCostRect.xmaxi = realToTile(qMax(p1.x(), p2.x()) + HalfStandardWireWidth);

		}
		else if (qAbs(p1.y() - p2.y()) < CloseEnough) {
			// horizontal
			pathUnit->minCostRect.xmini = tileRect.xmini;
			pathUnit->minCostRect.xmaxi = tileRect.xmaxi;
			pathUnit->minCostRect.ymini = realToTile(qMin(p1.y(), p2.y()) - HalfStandardWireWidth);
			pathUnit->minCostRect.ymaxi = realToTile(qMax(p1.y(), p2.y()) + HalfStandardWireWidth);
		}
		else {
			DebugDialog::debug("not yet dealing with slanted wires");
			delete pathUnit;
			return;
		}
	}

	m_pathUnits.append(pathUnit);
	pathUnit->edge = edge;
	pathUnit->tile = tile;
	drawGridItem(tile);
	pq.append(pathUnit);
	tilePathUnits.insert(tile, pathUnit);
}

bool CMRouter::propagate(PriorityQueue<PathUnit *> & p1, PriorityQueue<PathUnit *> & p2, JEdge * edge, QHash<Wire *, JEdge *> & tracesToEdges, QMultiHash<Tile *, PathUnit *> & tilePathUnits, ItemBase * board)
{
	QList<Wire *> wires;

	QList<PathUnit *> p1Terminals;
	QList<PathUnit *> p2Terminals;

	bool firstTime = true;
	for (int i = p1.count() - 1; i >= 0; i--) {
		PathUnit * p1PathUnit = p1.at(i);
		p1Terminals.append(p1PathUnit);
		p1PathUnit->destCost = std::numeric_limits<int>::max();
		int keepj = -1;
		for (int j = p2.count() - 1; j >= 0; j--) {
			PathUnit * p2PathUnit = p2.at(j);
			if (firstTime) p2Terminals.append(p2PathUnit);
			p2PathUnit->destCost = std::numeric_limits<int>::max();
			int d = manhattan(p1PathUnit->minCostRect, p2PathUnit->minCostRect);
			if (d < p1PathUnit->destCost) {
				p1PathUnit->destCost = d;
				keepj = j;
			}
		}
		firstTime = false;
		p2.at(keepj)->destCost = p1PathUnit->destCost;
		p2.resetPriority(keepj, p1PathUnit->destCost);
		p1.resetPriority(i, p1PathUnit->destCost);
	}
	p1.sort();
	p2.sort();

	CompletePath completePath;
	completePath.source = completePath.dest = NULL;
	bool success = false;
	while (p1.count() > 0 && p2.count() > 0) {
		PathUnit * pathUnit1 = p1.dequeue();
		if (propagateUnit(pathUnit1, p1, p2, p2Terminals, tilePathUnits, completePath)) {
			success = true;
			if (goodEnough(completePath)) break;
		}

		PathUnit * pathUnit2 = p2.dequeue();
		if (propagateUnit(pathUnit2, p2, p1, p1Terminals, tilePathUnits, completePath)) {
			success = true;
			if (goodEnough(completePath)) break;
		}

		ProcessEventBlocker::processEvents();
		if (m_cancelTrace || m_stopTrace || m_cancelled) {
			return false;
		}
	}

	if (success) {
		tracePath(edge, completePath, tracesToEdges, board);
	}
	else {
		// find a jumper
	}

	hideTiles();

	return success;
}

bool CMRouter::propagateUnit(PathUnit * pathUnit, PriorityQueue<PathUnit *> & sourceQueue, 
							PriorityQueue<PathUnit *> & destQueue, QList<PathUnit *> & destTerminals, 
							QMultiHash<Tile *, PathUnit *> & tilePathUnits, CompletePath & completePath)
{
	bool result = false;
	QList<Tile *> tiles;
	seedNext(pathUnit, tiles, tilePathUnits);
	foreach (Tile * tile, tiles) {
		int destCost = std::numeric_limits<int>::max();
		TileRect minCostRect = calcMinCostRect(pathUnit, tile);

		int sourceCost = pathUnit->sourceCost + manhattan(pathUnit->minCostRect, minCostRect);	
		bool redundantTile = false;
		QList<PathUnit *> redundantPathUnits;
		foreach (PathUnit * tpu, tilePathUnits.values(tile)) {
			if (tpu->priorityQueue == &sourceQueue) {
				if (!isRedundantPath(tpu, minCostRect, sourceCost)) continue;
				
				if (sourceCost >= tpu->sourceCost) {
					redundantTile = true;
					break;
				}
				else {
					redundantPathUnits.append(tpu);
				}
			}
		}

		foreach(PathUnit * rpu, redundantPathUnits) {
			tilePathUnits.remove(tile, rpu);
			sourceQueue.removeOne(rpu);
		}

		if (redundantTile) {
			continue;
		}
		
		QList<PathUnit *> goals;
		foreach (PathUnit * tpu, tilePathUnits.values(tile)) {
			if (tpu->priorityQueue == &destQueue) {
				goals.append(tpu);
			}
		}
		foreach (PathUnit * destTerminal, destTerminals) {
			destCost = qMin(manhattan(destTerminal->minCostRect, minCostRect), destCost);
		}

		PathUnit * nextPathUnit = new PathUnit(&sourceQueue);
		m_pathUnits.append(nextPathUnit);
		nextPathUnit->sourceCost = sourceCost;
		nextPathUnit->destCost = destCost;
		nextPathUnit->minCostRect = minCostRect;
		nextPathUnit->parent = pathUnit;
		nextPathUnit->edge = pathUnit->edge;
		nextPathUnit->tile = tile;
		tilePathUnits.insert(tile, nextPathUnit);
		infoTile("tilepathunits insert", tile);
		sourceQueue.enqueue(sourceCost + destCost, nextPathUnit);
		if (goals.count() > 0) {
			PathUnit * bestGoal = NULL;
			int bestCost;
			foreach (PathUnit * goalUnit, goals) {
				if (bestGoal == NULL) {
					bestGoal = goalUnit;
					bestCost = manhattan(goalUnit->minCostRect, nextPathUnit->minCostRect) + goalUnit->sourceCost + nextPathUnit->sourceCost;
				}
				else {
					int cost = manhattan(goalUnit->minCostRect, nextPathUnit->minCostRect)  + goalUnit->sourceCost + nextPathUnit->sourceCost;
					if (cost < bestCost) {
						bestCost = cost;
						bestGoal = goalUnit;
					}
				}
			}
			if (completePath.source == NULL || completePath.sourceCost > bestCost) {
				completePath.source = nextPathUnit;
				completePath.dest = bestGoal;
				completePath.sourceCost = bestCost;
				result = true;
			}
		}
	}

	return result;
}


TileRect CMRouter::calcMinCostRect(PathUnit * pathUnit, Tile * next)
{
	//infoTile("pathunit", pathUnit->tile);
	//infoTileRect("    pmcr", pathUnit->minCostRect);
	//infoTile("    next", next);

	TileRect nextMinCostRect;
	if (pathUnit->minCostRect.xmaxi <= LEFT(next)) {
		nextMinCostRect.xmini = LEFT(next);
		nextMinCostRect.xmaxi = qMin(LEFT(next) + TileStandardWireWidth, RIGHT(next));
	}
	else if (pathUnit->minCostRect.xmini >= RIGHT(next)) {
		nextMinCostRect.xmaxi = RIGHT(next);
		nextMinCostRect.xmini = qMax(RIGHT(next) - TileStandardWireWidth, LEFT(next));
	}
	else {
		nextMinCostRect.xmini = qMax(LEFT(next), pathUnit->minCostRect.xmini);
		nextMinCostRect.xmaxi = qMin(RIGHT(next), pathUnit->minCostRect.xmaxi);
	}

	if (pathUnit->minCostRect.ymaxi <= YMIN(next)) {
		nextMinCostRect.ymini = YMIN(next);
		nextMinCostRect.ymaxi = qMin(YMIN(next) + TileStandardWireWidth, YMAX(next));
	}
	else if (pathUnit->minCostRect.ymini >= YMAX(next)) {
		nextMinCostRect.ymaxi = YMAX(next);
		nextMinCostRect.ymini = qMax(YMAX(next) - TileStandardWireWidth, YMIN(next));
	}
	else {
		nextMinCostRect.ymini = qMax(YMIN(next), pathUnit->minCostRect.ymini);
		nextMinCostRect.ymaxi = qMin(YMAX(next), pathUnit->minCostRect.ymaxi);
	}

	//infoTileRect("     mcr", nextMinCostRect);

	return nextMinCostRect;
}

bool CMRouter::isRedundantPath(PathUnit * pathUnit, TileRect & minCostRect, int sourceCost)
{
	// eliminate redundant (not min cost) paths using "cost cones"

	// TODO: slopes are now assuming equal horizontal and vertical cost

	// first find the point of each cone
	QLineF minx1(minCostRect.xmini, sourceCost, minCostRect.xmini - 1000, sourceCost + 1000);
	QLineF maxx1(minCostRect.xmaxi, sourceCost, minCostRect.xmaxi + 1000, sourceCost + 1000);
	QPointF x1;
	minx1.intersect(maxx1, &x1);

	QLineF minx2(pathUnit->minCostRect.xmini, pathUnit->sourceCost, pathUnit->minCostRect.xmini - 1000, pathUnit->sourceCost + 1000);
	QLineF maxx2(pathUnit->minCostRect.xmaxi, pathUnit->sourceCost, pathUnit->minCostRect.xmaxi + 1000, pathUnit->sourceCost + 1000);
	QPointF x2;
	minx2.intersect(maxx2, &x2);

	if (x1.y() < x2.y()) {
		if (!insideV(x2, x1)) return false;
	}
	else if (x1.y() > x2.y()) {
		if (!insideV(x1, x2)) return false;
	}
	// else equal vertices is redundant

	QLineF miny1(minCostRect.ymini, sourceCost, minCostRect.ymini - 1000, sourceCost + 1000);
	QLineF maxy1(minCostRect.ymaxi, sourceCost, minCostRect.ymaxi + 1000, sourceCost + 1000);
	QPointF y1;
	miny1.intersect(maxy1, &y1);

	QLineF miny2(pathUnit->minCostRect.ymini, pathUnit->sourceCost, pathUnit->minCostRect.ymini - 1000, pathUnit->sourceCost + 1000);
	QLineF maxy2(pathUnit->minCostRect.ymaxi, pathUnit->sourceCost, pathUnit->minCostRect.ymaxi + 1000, pathUnit->sourceCost + 1000);
	QPointF y2;
	miny2.intersect(maxy2, &y2);

	if (y1.y() < y2.y()) {
		if (!insideV(y2, y1)) return false;
	}
	else if (y1.y() > y2.y()) {
		if (!insideV(y1, y2)) return false;
	}
	// else equal vertices is redundant

	return true;
}

bool CMRouter::insideV(const QPointF & check, const QPointF & vertex)
{
	// form the V from p2

	QPointF lv(vertex.x() - 10 + vertex.y() - check.y(), check.y() + 10);
	QPointF rv(vertex.x() + 10 - vertex.y() + check.y(), check.y() + 10);

	// the rest of this from: http://www.blackpawn.com/texts/pointinpoly/default.html

	QPointF v0 = rv - vertex;
	QPointF v1 = lv - vertex;
	QPointF v2 = check - vertex;

	// Compute dot products
	qreal dot00 = dot(v0, v0);
	qreal dot01 = dot(v0, v1);
	qreal dot02 = dot(v0, v2);
	qreal dot11 = dot(v1, v1);
	qreal dot12 = dot(v1, v2);

	// Compute barycentric coordinates
	qreal invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
	qreal u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	qreal v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	// Check if point is in or on triangle
	return (u >= 0) && (v >= 0) && (u + v <= 1);
}

bool CMRouter::goodEnough(CompletePath & completePath) {
	PathUnit * sourcePathUnit = completePath.source;
	PathUnit * destPathUnit = completePath.dest;
	for ( ; sourcePathUnit->parent; sourcePathUnit = sourcePathUnit->parent);
	for ( ; destPathUnit->parent; destPathUnit = destPathUnit->parent);
	int bestCost = manhattan(sourcePathUnit->minCostRect, destPathUnit->minCostRect);
	return completePath.sourceCost < ((qreal) bestCost) * 1.05;
}

void CMRouter::tracePath(JEdge * edge, CompletePath & completePath, QHash<Wire *, JEdge *> & tracesToEdges, ItemBase * board)
{
	QList<PathUnit *> fullPath;
	for (PathUnit * spu = completePath.source; spu; spu = spu->parent) {
		fullPath.push_front(spu);
	}
	fullPath.takeLast();			// this tile is redundant so pop it from the list
	for (PathUnit * dpu = completePath.dest; dpu; dpu = dpu->parent) {
		fullPath.append(dpu);
	}

	foreach (PathUnit * pathUnit, fullPath) {
		infoTile("tracepath", pathUnit->tile);
	}

	QList<Segment *> hSegments;
	QList<Segment *> vSegments;
	foreach (PathUnit * pathUnit, fullPath) {
		Segment * h = new Segment;
		h->sEntry = h->sExit = Segment::NotSet;
		h->sMin = LEFT(pathUnit->tile);
		h->sMax = RIGHT(pathUnit->tile);
		Segment * v = new Segment;
		v->sEntry = v->sExit = Segment::NotSet;
		v->sMin = YMIN(pathUnit->tile);
		v->sMax = YMAX(pathUnit->tile);
		hSegments.append(h);
		vSegments.append(v);
	}

	initConnectorSegments(0, fullPath, hSegments, vSegments);
	initConnectorSegments(fullPath.count() - 1, fullPath, hSegments, vSegments);

	traceSegments(hSegments);
	traceSegments(vSegments);

	QList<QPointF> allPoints;
	for (int i = 0; i < hSegments.count(); i++) {
		Segment * h = hSegments.at(i);
		Segment * v = vSegments.at(i);
		allPoints.append(QPointF(tileToReal(h->sEntry), tileToReal(v->sEntry)));
		allPoints.append(QPointF(tileToReal(h->sExit), tileToReal(v->sExit)));
	}
	allPoints.takeFirst();
	allPoints.takeLast();

	foreach (Segment * segment, hSegments) {
		delete segment;
	}
	foreach (Segment * segment, vSegments) {
		delete segment;
	}

	cleanPoints(allPoints, edge);
	
	QList<Wire *> wires;
	for (int i = 0; i < allPoints.count() - 1; i++) {
		QPointF p1 = allPoints.at(i);
		QPointF p2 = allPoints.at(i + 1);
		Wire * trace = drawOneTrace(p1, p2, Wire::STANDARD_TRACE_WIDTH, edge->viewLayerSpec);
		wires.append(trace);
	}

	// TODO: handle wire stickyness
	// TODO: make sure that splitTrace succeeds if trace was not autoroutable


	PathUnit * first = fullPath.first();
	if (first->connectorItem == NULL) {
		first->connectorItem = splitTrace(first->wire, allPoints.first(), board);
		Wire * split = qobject_cast<Wire *>(first->connectorItem->attachedTo());
		tracesToEdges.insert(split, edge);
	}
	PathUnit * last = fullPath.last();
	if (last->connectorItem == NULL) {
		last->connectorItem = splitTrace(last->wire, allPoints.last(), board);
		Wire * split = qobject_cast<Wire *>(last->connectorItem->attachedTo());
		tracesToEdges.insert(split, edge);
	}

	// hook everyone up
	hookUpWires(edge, fullPath, wires);
	foreach (Wire * wire, wires) {
		tracesToEdges.insert(wire, edge);
	}
}

void CMRouter::cleanPoints(QList<QPointF> & allPoints, JEdge * edge) 
{
	foreach (QPointF p, allPoints) {
		DebugDialog::debug("allpoint before:", p);
	}

	// remove redundant pairs
	int ix = allPoints.count() - 1;
	while (ix > 0) {
		if (allPoints[ix] == allPoints[ix - 1]) {
			allPoints.removeAt(ix);
		}
		ix--;
	}

	// make sure all pairs have 90 degree turns
	ix = 0;
	while (ix < allPoints.count() - 1) {
		QPointF p1 = allPoints[ix];
		QPointF p2 = allPoints[ix + 1];
		if (qAbs(p1.x() - p2.x()) > 1 && qAbs(p1.y() - p2.y()) >= CloseEnough) {
			// insert another point
			QPointF p3(p2.x(), p1.y());   // the other corner may be better
			allPoints.insert(ix + 1, p3);
		}
		ix++;
	}

	// eliminate redundant colinear points
	ix = 0;
	while (ix < allPoints.count() - 2) {
		QPointF p1 = allPoints[ix];
		QPointF p2 = allPoints[ix + 1];
		QPointF p3 = allPoints[ix + 2];
		if (p1.x() == p2.x() && p2.x() == p3.x()) {
			allPoints.removeAt(ix + 1);
			ix--;
		}
		else if (p1.y() == p2.y() && p2.y() == p3.y()) {
			allPoints.removeAt(ix + 1);
			ix--;
		}
		ix++;
	}

	//removeCorners(allPoints, edge);
	//shortenUs(allPoints, subedge);

	foreach (QPointF p, allPoints) {
		DebugDialog::debug("allpoint after:", p);
	}
}

void CMRouter::initConnectorSegments(int ix0, QList<PathUnit *> & fullPath, QList<Segment *> & hSegments, QList<Segment *> & vSegments) 
{
	PathUnit * pathUnit = fullPath.at(ix0);
	if (pathUnit->connectorItem == NULL) return;

	Segment * h = hSegments.at(ix0);
	Segment * v = vSegments.at(ix0);
	h->sMin = pathUnit->minCostRect.xmini;
	h->sEntry = h->sMin + TileHalfStandardWireWidth;
	h->sMax = pathUnit->minCostRect.xmaxi;
	h->sExit = h->sMax - TileHalfStandardWireWidth;

	v->sMin = pathUnit->minCostRect.ymini;
	v->sEntry = v->sMin + TileHalfStandardWireWidth;
	v->sMax = pathUnit->minCostRect.ymaxi;
	v->sExit = v->sMax - TileHalfStandardWireWidth;
}

void CMRouter::traceSegments(QList<Segment *> & segments) {
	foreach(Segment * segment, segments) {
		DebugDialog::debug(QString("segment %1 %2 %3 %4").arg(segment->sMin).arg(segment->sMax).arg(segment->sEntry).arg(segment->sExit));
	}

	// use non-overlaps to set entry and exit
	for (int ix = 0; ix < segments.count(); ix++) {
		Segment * from = segments.at(ix);
		if (from->sEntry != Segment::NotSet) {
			if (ix < segments.count() - 1) {
				Segment * to = segments.at(ix + 1);
				if (to->sMin <= from->sEntry - TileHalfStandardWireWidth && to->sMax >= from->sEntry + TileHalfStandardWireWidth) {
					// can enter and exit at the same place in the current segment and put the problem off to the next segment
					to->setEntry(from->sEntry);
					from->setExit(from->sEntry);
					continue;
				}
			}
		}
		else if (ix > 0) {
			// from->sEntry not set
			Segment * prev = segments.at(ix - 1);
			if (prev->sExit > from->sMax - TileHalfStandardWireWidth) {
				from->setEntry(from->sMax - TileHalfStandardWireWidth);
			}
			else if (prev->sExit < from->sMin + TileHalfStandardWireWidth) {
				from->setEntry(from->sMin + TileHalfStandardWireWidth);
			}
			else {
				from->setEntry(prev->sExit);
			}
		}

		// if no non-overlaps, use limitMin and limitMax
		bool gotNonOverlap = false;
		int limitMin = from->sMin;			
		int limitMax = from->sMax;
		for (int jx = ix + 1; jx < segments.count(); jx++) {
			Segment * to = segments.at(jx);
			int entryMin = qMax(from->sMin, to->sMin);
			int entryMax = qMin(from->sMax, to->sMax);
			if (entryMax - entryMin >= TileStandardWireWidth) {
				limitMin = qMin(qMax(limitMin, to->sMin), limitMax - TileStandardWireWidth);
				limitMax = qMax(qMin(limitMax, to->sMax), limitMin + TileStandardWireWidth);
				continue;
			}

			gotNonOverlap = true;
			//int clipMin;
			//int clipMax;
			if (from->sMax < to->sMax) {
				from->setExit(from->sMax - TileHalfStandardWireWidth);
				to->setEntry(to->sMin + TileHalfStandardWireWidth);
				//clipMin = from->sMax - TileStandardWireWidth;
				//clipMax = to->sMin + TileStandardWireWidth;
			}
			else {
				from->setExit(from->sMin + TileHalfStandardWireWidth);
				to->setEntry(to->sMax - TileHalfStandardWireWidth);
				//clipMax = from->sMin + TileStandardWireWidth;
				//clipMin = to->sMax - TileStandardWireWidth;
			}
			/*
			for (int kx = ix + 1; kx < jx; kx++) {
				Segment * btween = segments.at(kx);
				if (clipMax < btween->sMax) {
					btween->sMax = clipMax;
				}
				if (clipMin > btween->sMin) {
					btween->sMin = clipMin;
				}
				if (btween->sMin >= btween->sMax) {
					DebugDialog::debug("clipping failure");
				}
			}
			*/

			break;
		}

		if (!gotNonOverlap) {
			if (from->sEntry == Segment::NotSet) {
				from->setEntry(limitMin + TileHalfStandardWireWidth);
			}
			if (qAbs(from->sEntry - limitMin) <= qAbs(from->sEntry - limitMax)) {
				from->setExit(limitMin + TileHalfStandardWireWidth);
			}
			else {
				from->setExit(limitMax - TileHalfStandardWireWidth);
			}
		}
	}

	for (int ix = 0; ix < segments.count(); ix++) {
		Segment * segment = segments.at(ix);
		DebugDialog::debug(QString("final segment %1 %2 %3 %4").arg(segment->sMin).arg(segment->sMax).arg(segment->sEntry).arg(segment->sExit));
		if (ix > 0 && segment->sEntry == Segment::NotSet) {
			DebugDialog::debug("segment failure");
		}
		if (ix < segments.count() - 1 && segment->sExit == Segment::NotSet) {
			DebugDialog::debug("segment failure");
		}
	}
}

void CMRouter::makeAlignTiles(QMultiHash<Tile *, TileRect *> & tileRects, Plane * thePlane)
{
	foreach (Tile * tile, tileRects.uniqueKeys()) {
		foreach (TileRect * tileRect, tileRects.values(tile)) {
			insertTiny(thePlane, *tileRect);
			delete tileRect;
		}
	}
	ChangedTiles.clear();
}
