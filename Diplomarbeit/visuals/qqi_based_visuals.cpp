#include <QQmlProperty>
#include <QSGTexture>
#include <QtSvg/QSvgRenderer>
#include <QPainter>
#include <QtCore/qmath.h>
#include <QSGSimpleTextureNode>
#include <iostream>

#include "qqi_based_visuals.h"


QQI_Based_Visuals::QQI_Based_Visuals(QQuickItem* parent) : Visuals(parent)
{
    startIndex = - 1;
    startMs = 0;
    setFlags(ItemHasContents);
}


QQI_Based_Visuals::~QQI_Based_Visuals()
{
    disconnect(window(), SIGNAL(beforeRendering()), this, SLOT(update()));
    for(int i = 0; i < digitTextures.count(); i++) {
        delete digitTextures[i];
    }
    digitTextures.clear();
    if (leftTriangleTexture)
        delete leftTriangleTexture;
    if (rightTriangleTexture)
        delete rightTriangleTexture;
}


QSGSimpleRectNode* QQI_Based_Visuals::getPlayerNoteRectanlgeNode(QPointer<Note> note, qreal left, qreal top, qreal width, qreal height)
{
    QRectF noteRectangle(left, top, width, height);
    QSGSimpleRectNode* newRectNode = new QSGSimpleRectNode(noteRectangle, Qt::black);

    if (!note->getTarget().isNull())
    {
        if (note->getPitch() == note->getTarget()->getPitch())
        {
            newRectNode->setColor(Qt::green);
            // < 0 => too early      > 0 => too late
            qreal startDeviation = note->getTime() - note->getTarget()->getTime();
            qreal stopDeviation = note->getTime() + note->getDuration() - (note->getTarget()->getTime() + note->getTarget()->getTieDuration());

            // note played too early
            if (startDeviation < -game->timingOnThreshold)
            {
                qreal redWidth;
                if (note->getDuration() == 0 && game->getGameTime() < note->getTarget()->getTime())
                {
                    redWidth = width;
                }
                else
                {
                    redWidth = startDeviation * (-1) * getTimeFactor();
                }
                QRectF leftErrorRect = QRectF(noteRectangle.left(), noteRectangle.top(), redWidth, noteRectangle.height());
                QSGSimpleRectNode* leftErrorNode = new QSGSimpleRectNode(leftErrorRect, Qt::red);
                newRectNode->appendChildNode(leftErrorNode);

                QSGSimpleTextureNode* triangleNode = new QSGSimpleTextureNode();
                QRectF triangleBoundingRect = QRectF(left - height / 2, top, height / 2, height);
                triangleNode->setTexture(leftTriangleTexture);
                triangleNode->setRect(triangleBoundingRect);
                newRectNode->appendChildNode(triangleNode);
            }

            // note played too late
            if (startDeviation > game->timingOnThreshold)
            {
                qreal redWidth = startDeviation * getTimeFactor();
                QRectF rightErrorRect = QRectF(noteRectangle.left() - redWidth, noteRectangle.top(), redWidth, noteRectangle.height());
                QSGSimpleRectNode* rightErrorNode = new QSGSimpleRectNode(rightErrorRect, Qt::red);
                newRectNode->appendChildNode(rightErrorNode);

                QSGSimpleTextureNode* triangleNode = new QSGSimpleTextureNode();
                QRectF triangleBoundingRect = QRectF(left, top, height / 2, height);
                triangleNode->setTexture(rightTriangleTexture);
                triangleNode->setRect(triangleBoundingRect);
                newRectNode->appendChildNode(triangleNode);
            }

            if (note->getDuration() != 0) {

                qreal triangleWidth = min((qreal) 15, width / 5);


                if (stopDeviation > game->timingOffThreshold)
                {
                    qreal redWidth = stopDeviation * getTimeFactor();
                    QRectF rightErrorRect = QRectF(noteRectangle.right() - redWidth, noteRectangle.top(),redWidth, noteRectangle.height());
                    QSGSimpleRectNode* rightErrorNode = new QSGSimpleRectNode(rightErrorRect, Qt::red);
                    newRectNode->appendChildNode(rightErrorNode);

                    QSGSimpleTextureNode* triangleNode = new QSGSimpleTextureNode();
                    QRectF triangleBoundingRect = QRectF(noteRectangle.right(), top, height / 2, height);
                    triangleNode->setTexture(rightTriangleTexture);
                    triangleNode->setRect(triangleBoundingRect);
                    newRectNode->appendChildNode(triangleNode);
                }

                // stopped too early
                if (false)//stopDeviation < -game->timingOffThreshold)
                {
                    qreal redWidth = -stopDeviation * getTimeFactor();
                    QRectF rightErrorRect = QRectF(noteRectangle.right(), noteRectangle.top(), redWidth, noteRectangle.height());
                    QSGSimpleRectNode* rightErrorNode = new QSGSimpleRectNode(rightErrorRect, Qt::red);
                    newRectNode->appendChildNode(rightErrorNode);

                    QSGSimpleTextureNode* triangleNode = new QSGSimpleTextureNode();
                    QRectF triangleBoundingRect = QRectF(noteRectangle.right() - height / 2, top, height / 2, height);
                    triangleNode->setTexture(leftTriangleTexture);
                    triangleNode->setRect(triangleBoundingRect);
                    newRectNode->appendChildNode(triangleNode);
                }
            }
        }
        else
        {
            newRectNode->setColor(Qt::red);
        }
    }
    else
    {
        newRectNode->setColor(Qt::red);
    }

    return newRectNode;
}


void QQI_Based_Visuals::setGame(const QObject *game) {
    window()->setClearBeforeRendering(false);
    //window()->setPersistentSceneGraph(true);
    //connect(window(), SIGNAL(frameSwapped()), this, SLOT(update()));
    connect(window(), SIGNAL(beforeRendering()), this, SLOT(update()));
    Visuals::setGame(game);
}


QSGNode* QQI_Based_Visuals::getNowLine() {
    return getLineNode(boundingRect().x() + boundingRect().width() / 2,
                       boundingRect().bottom(),
                       boundingRect().x() + boundingRect().width() / 2,
                       boundingRect().top(),
                       Qt::darkRed,
                       4);
}


QSGNode* QQI_Based_Visuals::getBarsNode() {

    QSGNode* node = new QSGNode();

    for (int i=0; i<this->game->getScore()->bars.size(); i++) {
        node->appendChildNode(
            new QSGSimpleRectNode(
                        QRectF(calculatePosition(this->game->getScore()->bars[i], true) - xOffset,
                               boundingRect().top(), 2, boundingRect().height()), QColor(0,0,0,63)));
    }

    return node;
}

QSGNode *QQI_Based_Visuals::updatePaintNode(QSGNode *n, QQuickItem::UpdatePaintNodeData *data)
{
    int closestNotes = game->getTargetTrack()->getClosestNotes();
    maxMsPerScreen = 8000;

    if (!texturesAvailable)
    {
        loadTextures();
    } else if (reloadSpecificTexturesOnResize) {
        if(oldHeight != boundingRect().height()) {
            loadDisplayOptionSpecificTextures();
            oldHeight = boundingRect().height();
        }
    }

    QSGGeometryNode *node = static_cast<QSGGeometryNode *>(n);
    if (node)
        delete node;
    node = new QSGSimpleRectNode(boundingRect(), Qt::white);

    if (this->game->getScore())
    {
        updateGameTimeToPosition();
        gameTime = game->getGameTime();

        node->appendChildNode(getNowLine());
        node->appendChildNode(getBarsNode());
        node->appendChildNode(getTargetNotes());
        node->appendChildNode(getPlayedNotes());

        node->appendChildNode(getDisplayOptionSpecificNodes());

        updateScrollBarPosition();
        node->markDirty(QSGNode::DirtyForceUpdate);
    }
    return node;
}


QSGNode *QQI_Based_Visuals::getPlayedNotes()
{
    QSGNode* node = new QSGNode();
    node->setFlag(QSGNode::OwnedByParent);
    if (!game->getPlayerTrack().isNull())
    {
        for (unsigned int i = 0; i < game->getPlayerTrack()->notes.size(); i++)
        {
            QPointer<Note> note = game->getPlayerTrack()->notes.at(i);
            if (!note.isNull()) {
                QSGSimpleRectNode *drawNode = getNodeForNote(note);
                if(drawNode != NULL) {
                    node->setFlag(QSGNode::OwnedByParent);
                    node->appendChildNode(drawNode);
                }
            }
        }
    }

    return node;

}

QSGNode* QQI_Based_Visuals::getLineNode(qreal x0, qreal y0, qreal x1, qreal y1) {
    return getLineNode(x0, y0, x1, y1, Qt::gray);
}

QSGNode* QQI_Based_Visuals::getLineNode(qreal x0, qreal y0, qreal x1, qreal y1, QColor color) {
    return getLineNode(x0, y0, x1, y1, color, 2);
}

QSGNode* QQI_Based_Visuals::getLineNode(qreal x0, qreal y0, qreal x1, qreal y1, QColor color, int lineWidth) {
    QSGGeometryNode* lineNode = new QSGGeometryNode();
    QSGGeometry* lineGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 2);
    lineGeometry->setLineWidth(lineWidth);
    lineGeometry->setDrawingMode(GL_LINE_STRIP);
    lineNode->setGeometry(lineGeometry);
    QSGFlatColorMaterial* material = new QSGFlatColorMaterial();
    material->setColor(color);
    lineNode->setMaterial(material);

    QSGGeometry::Point2D *vertices = lineGeometry->vertexDataAsPoint2D();
    vertices[0].x = x0;
    vertices[0].y = y0;
    vertices[1].x = x1;
    vertices[1].y = y1;

    lineNode->setFlag(QSGNode::OwnedByParent);
    lineNode->setFlag(QSGNode::OwnsGeometry);
    lineNode->setFlag(QSGNode::OwnsMaterial);
    lineNode->setFlag(QSGNode::OwnsOpaqueMaterial);
    return lineNode;
}


QSGNode* QQI_Based_Visuals::getTargetNotes() {

    QSGNode* targetNotes = new QSGNode();

    if (game->getGameMode() == REPLAY) {
        startIndex = 0;
    }

    if (startIndex == -1)
    {
        startIndex = this->game->getTargetTrack()->getStartIndex(game->getSelectedSectionIndex());
    }

    int endIndex = getLastIndex();

    for(unsigned int i = startIndex; i < game->getTargetTrack()->notes.size(); i++)
    {
        QPointer<Note> note = this->game->getTargetTrack()->notes.at(i);

        int noteX = 0;
        noteX = max(calculatePosition(note->getTime() + note->getTieDuration(), true),
                            calculatePosition(note->getTime() + note->getDuration(), true));

        if(noteX >= -2000*2 && noteX <= width()+2000) {

            if (!note.isNull())
            {
                QSGNode *node = getNodeForNote(note, true);
                if (node)
                {
                    node->setFlag(QSGNode::OwnedByParent);
                    targetNotes->appendChildNode(node);
                }
            }

        }
    }
    targetNotes->setFlag(QSGNode::OwnedByParent);
    return targetNotes;
}


QSGSimpleRectNode* QQI_Based_Visuals::getNodeForNote(QPointer<Note> note, bool isFromScore) {
    return nullptr;
}


QSGNode *QQI_Based_Visuals::getDisplayOptionSpecificNodes()
{
    return nullptr;
}


void QQI_Based_Visuals::loadDisplayOptionSpecificTextures()
{

}


void QQI_Based_Visuals::displayOptionSpecificPreparations()
{

}


void QQI_Based_Visuals::loadTextures()
{
    loadDisplayOptionSpecificTextures();
    loadNonSpecificTextures();

    texturesAvailable = true;
    oldHeight = boundingRect().height();
}


int QQI_Based_Visuals::getLastIndex()
{
    for (int i = startIndex; i < game->getTargetTrack()->notes.size(); i++)
    {
        QPointer<Note> note = game->getTargetTrack()->notes.at(i);
        int pos = calculatePosition(note->getTime() - xOffset*3, true);
        if (pos > window()->width()) {
            return i;
        }
    }

    return game->getTargetTrack()->notes.size();
}


qreal QQI_Based_Visuals::calculatePosition(QPointer<Note> note, bool isFromScore)  {

    qreal left = boundingRect().right() - (qreal) boundingRect().width() / 2 + (note->getTime() - game->getGameTime()) * getTimeFactor() * scale;

    if (isFromScore)
    {
        left -= (startMs) * getTimeFactor();
    }

    return left;
}


qreal QQI_Based_Visuals::calculatePosition(int time, bool isFromScore)  {

    qreal left = boundingRect().right() - (qreal) boundingRect().width() / 2 + (time - game->getGameTime()) * getTimeFactor() * scale;

    if (isFromScore)
    {
        left -= (startMs) * getTimeFactor();
    }

    return left;
}


QSGSimpleRectNode* QQI_Based_Visuals::getBorderedRectangleNode(qreal left, qreal top, qreal width, qreal height)
{
    qreal lineWidth = max(round(boundingRect().height() / 400.0), 1.0);

    QRectF noteRectangle = QRectF(left, top, width, height);
    QSGSimpleRectNode* newRectNode = new QSGSimpleRectNode(noteRectangle, Qt::black);

    QRectF subNodeRectangle = noteRectangle;
    subNodeRectangle.setTop(top + lineWidth);
	    subNodeRectangle.setLeft(left + lineWidth);
    subNodeRectangle.setWidth(width - lineWidth * 2);
    subNodeRectangle.setHeight(height - lineWidth * 2);
    newRectNode->setFlag(QSGNode::OwnedByParent);
    newRectNode->appendChildNode(new QSGSimpleRectNode(subNodeRectangle, Qt::white));
    return newRectNode;
}


qreal QQI_Based_Visuals::getNoteRectangleWidth(QPointer<Note> note) {

    qreal width;

    if (note->getDuration() != 0)
    {
        if (note->getLinking() == LinkingType::tie) {
            width = note->getTieDuration() * getTimeFactor();
        }
        else
        {
            width = note->getDuration() * getTimeFactor();
        }
    }
    else
    {
        width = (gameTime - note->getTime()) * getTimeFactor();
    }

    return width;
}


void QQI_Based_Visuals::loadNonSpecificTextures()
{
    QImage myImage;

    if (leftTriangleTexture == NULL)
    {
        #ifdef Q_OS_ANDROID
            cout << myImage.load("assets:/graphics/triangle_left.png");
        #else
            cout << myImage.load("graphics/triangle_left.png");
        #endif
        leftTriangleTexture = window()->createTextureFromImage(myImage.copy());
    }

    if (rightTriangleTexture == NULL)
    {
        #ifdef Q_OS_ANDROID
            cout << myImage.load("assets:/graphics/triangle_right.png");
        #else
            cout << myImage.load("graphics/triangle_right.png");
        #endif
        rightTriangleTexture = window()->createTextureFromImage(myImage.copy());
    }

    loadDigitTextures();
}


void QQI_Based_Visuals::loadDigitTextures()
{
    if (digitTextures.size() == 0)
    {
        QString path;
        #ifdef Q_OS_ANDROID
            path = "assets:/graphics/numbers/";
        #else
            path = "graphics/numbers/";
        #endif

        QList<QImage> subImages;

        for (int i = 0; i < 10; i++)
        {
            QImage image;
            cout << image.load(path + QString::number(i) + ".png");
            subImages.append(image);
        }

        for (int i = 0; i < subImages.size(); i++)
        {
            QSGTexture* digitTexture = window()->createTextureFromImage(subImages.at(i));
            digitTextures.append(digitTexture);
        }
    }
}


QSGSimpleRectNode *QQI_Based_Visuals::getNumberNode(int number, int size, int left, int top)
{
    QList<int> digits = numberToDigits(number);
    QSGSimpleRectNode *node = new QSGSimpleRectNode(QRectF(left, top, digits.length() * size, size), QColor(0, 0, 0, 0));
    node->setFlag(QSGNode::OwnedByParent);
    for (int i=0; i<digits.length(); i++)
    {
        QSGSimpleTextureNode *digitNode = new QSGSimpleTextureNode();
        digitNode->setFlag(QSGNode::OwnedByParent);
        digitNode->setTexture(digitTextures[digits[i]]);
        digitNode->setRect(left + i * size * 0.6, top, size, size);
        node->appendChildNode(digitNode);
    }

    return node;
}


QList<int> QQI_Based_Visuals::numberToDigits(int number)
{
    if (number < 0) {
        return QList<int>();
    }

    if (number == 0) {
        QList<int> list;
        list.append(0);
        return list;
    }

    std::vector<int> digits;

    while (number)
    {
        digits.push_back(number % 10);
        number /= 10;
    }

    std::reverse(digits.begin(), digits.end());
    return QList<int>::fromVector(QVector<int>::fromStdVector(digits));
}


QImage QQI_Based_Visuals::createSubImage(QImage* image, const QRect & rect) // broken
{
    size_t offset = rect.x() * image->depth() / 8
                    + rect.y() * image->bytesPerLine();
    return QImage(image->bits() + offset, rect.width(), rect.height(),
                  image->bytesPerLine(), image->format());
}



QSGTexture* QQI_Based_Visuals::loadTextureFromSVG(QString texturePath, qreal width, qreal height)
{
#if defined(Q_OS_ANDROID)
    texturePath = QString("assets:/") + texturePath;
#endif

    QSvgRenderer svgRenderer(texturePath);
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(0xFFFFFF);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    svgRenderer.render(&painter);

    return window()->createTextureFromImage(image);
}

