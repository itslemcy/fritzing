#include "svg2gerber.h"
#include "../debugdialog.h"

//TODO: this assumes one layer right now (copper0)

SVG2gerber::SVG2gerber(QString svgStr)
{
    m_SVGDom = QDomDocument("svg");
    m_SVGDom.setContent(svgStr);

    normalizeSVG();
    allPaths2gerber();
}

QString SVG2gerber::getGerber(){
    return m_gerber;
}

void SVG2gerber::normalizeSVG(){
    QDomElement root = m_SVGDom.documentElement();

    //  convert to paths
    convertShapes2paths(root);

    //  get rid of transforms
    flattenSVG(root);

}

void SVG2gerber::convertShapes2paths(QDomNode node){
    // I'm a leaf node.  make me a path
    if(!node.hasChildNodes()) {
        QString tag = node.nodeName().toLower();
        QDomElement element = node.toElement();
        QDomElement path;

        DebugDialog::debug("converting child to path: " + tag);

        if(tag=="polygon"){
            path = poly2path(element);
        }
        else if(tag=="rect"){
            path = rect2path(element);
        }
        else if(tag=="circle"){
            path = circle2path(element);
        }
        else if(tag=="line"){
            path = line2path(element);
        }
        else {
            DebugDialog::debug("svg2gerber ignoring unrecognized SVG element: " + tag);
        }

        return;
    }

    // recurse the children
    QDomNodeList tagList = node.childNodes();

    for(uint i = 0; i < tagList.length(); i++){
        convertShapes2paths(tagList.item(i));
    }
}

void SVG2gerber::flattenSVG(QDomNode node){
    // I'm a leaf node. flatten me
    if(!node.hasChildNodes()) {

    }

    // recurse the children
    QDomNodeList tagList = node.childNodes();

    for(uint i = 0; i < tagList.length(); i++){
        flattenSVG(tagList.item(i));
    }

    // if I'm a <g>, delete me
}

void SVG2gerber::allPaths2gerber() {

}

QDomElement SVG2gerber::rect2path(QDomElement rectElement){
    // 4 x line2path()
    return rectElement;
}

QDomElement SVG2gerber::circle2path(QDomElement circleElement){
    // 4 arcs
    return circleElement;
}

QDomElement SVG2gerber::line2path(QDomElement lineElement){
    return lineElement;
}

QDomElement SVG2gerber::poly2path(QDomElement polyElement){
    // this is just a bunch of calls to line2path()
    return polyElement;
}

QString SVG2gerber::path2gerber(QDomElement pathElement){
    return QString("NOT IMPLEMENTED YET");
}
