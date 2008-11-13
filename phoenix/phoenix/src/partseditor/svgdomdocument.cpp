#include "svgdomdocument.h"
#include <qmath.h>
//
SVGDomDocument::SVGDomDocument(  ) 
	: QDomDocument()
{
	
	QDomElement svgroot = createElement("svg");
	svgroot.setAttribute("xmlns","http://www.w3.org/2000/svg");
	svgroot.setAttribute("version","1.2");
	svgroot.setAttribute("baseProfile","tiny");
	//svgroot.setAttribute("viewBox","0 0 400 400");  //TODO: set dynamically
	appendChild(svgroot);
	
	QDomElement desc = createElement("desc");
	QDomText descText = createTextNode("Fritzing footprint SVG");
	desc.appendChild(descText);
	
	svgroot.appendChild(desc);
}

void SVGDomDocument::setWidth(int width, QString units){
	QDomElement svgroot = documentElement();
	float scaled = 1;
	if(units == "mils"){
		scaled = qreal(width)/1000.0;
		units = "in";
	}
	if(units == "cmil"){
		scaled = qreal(width)/100000.0;
		units = "in";
	}
	QString strwidth = QString::number(scaled) + units;
	
	svgroot.setAttribute("width",strwidth);
}

void SVGDomDocument::setHeight(int height, QString units){
	QDomElement svgroot = documentElement();
	float scaled = 1;
	if(units == "mils"){
		scaled = qreal(height)/1000.0;
		units = "in";
	}
	if(units == "cmil"){
		scaled = qreal(height)/100000.0;
		units = "in";
	}
	QString strheight = QString::number(scaled) + units;
	svgroot.setAttribute("height",strheight);
}

void SVGDomDocument::setViewBox(int minx, int miny, int maxx, int maxy){
	QDomElement svgroot = documentElement();
	
	QString viewBox = QString::number(minx) + " " + QString::number(miny) + " " + QString::number(maxx) + " " + QString::number(maxy);
	svgroot.setAttribute("viewBox", viewBox);
}

void SVGDomDocument::save(QString fileName){
	QFile file(fileName);
	
	if (!file.open(QFile::WriteOnly | QFile::Text)) {
    	QMessageBox::warning(NULL, QObject::tr("Parts Editor"),
                     QObject::tr("Cannot write to file %1:\n%2.")
                     .arg(fileName)
                     .arg(file.errorString()));
    }
    
    QTextStream out(&file);
    // This is kinda naughty but QDom seem to have no other way set the header!
    QString xmlDeclaration = "<?xml version='1.0' encoding='UTF-8'?>\n";
    out << xmlDeclaration << toString();
    
    file.close();
}

QDomElement SVGDomDocument::createGroup(QString id){ 
	QDomElement group = createElement("g");
	group.setAttribute("id", id);
	documentElement().appendChild(group);
	return group;
}
