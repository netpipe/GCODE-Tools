#include "svgreader.h"
#include <QPen>
#include <QFile>
#include <QMessageBox>
#include <QDomDocument>
#include <QStringList>
#include <QRect>
SvgReader::SvgReader()
{

}

QVector<QPointF> SvgReader::getElements(const QString filename)
{
    QVector<QPointF> ret;
    QList<QGraphicsItem *> itemList;    // Declare the stack list boxes

    QDomDocument doc;       // document object
    QFile file(filename);   // Open your SVG-file
    // If it is not opened, or have failed to transmit the contents in QDocDocument
    if (!file.open(QIODevice::ReadOnly) || !doc.setContent(&file))
        return ret;    // the refund list, but empty

    // We are looking for all objects in a document with tag g
    QDomNodeList gList = doc.elementsByTagName("g");
    // Getting them to touch
    for (int i = 0; i < gList.size(); i++) {
        QDomNode gNode = gList.item(i);     // Select from the node list

        QDomNodeList objList = gNode.childNodes();
        for(int j = 0; j<objList.size(); j++)
        {
            QDomElement ele = objList.at(j).toElement();
            if(ele.tagName() == "path"){
                QString d = ele.attribute("d");

                d = d.replace("M ", "");
                d = d.replace(",", " ");

                QStringList list = d.split(" ");
                for(int k = 0; k<list.size()/2; k++)
                {
                    QPointF p (list[2*k].toFloat(), list[2*k+1].toFloat());

                    if( ret.size() ==0 || ret.last() != p)
                    {
                        ret.push_back(p);
                    }
                }
            }
        }

    }
    //if first == last => remove last
    if(ret.size() >= 2)
    {
        if(ret.first() == ret.last())
            ret.pop_back();
    }
    file.close();
    return ret;
}

QRectF SvgReader::getSizes(const QString filename)
{
    QDomDocument doc;       // initialize the stack object QDomDocument
    QFile file(filename);   // Open your SVG-file
    // If it is not opened, or have failed to transmit the contents in QDocDocument
    if (!file.open(QIODevice::ReadOnly) || !doc.setContent(&file))
        return QRectF(0,0,200,200); // the return values for the default scene

    /* Then pick up a list of items with the tag svg.
      * If a list of elements is not empty,
      * take the size of the graphic scenes.
     * */
    QDomNodeList list = doc.elementsByTagName("svg");
    if(list.length() > 0) {
        QDomElement svgElement = list.item(0).toElement();
        QStringList parameters = svgElement.attribute("viewBox").split(" ");
        return QRectF(parameters.at(0).toInt(),
                      parameters.at(1).toInt(),
                      parameters.at(2).toInt(),
                      parameters.at(3).toInt());
    }
    return QRectF(0,0,200,200);
}
