#include <QQmlProperty>
#include <QSGTexture>
#include <QtSvg/QSvgRenderer>
#include <QPainter>
#include <QtCore/qmath.h>
#include <QSGSimpleTextureNode>
#include <iostream>

#include "scoreview.h"
#include "../music/track.h"
#include "visuals.h"

using namespace std;

ScoreView::ScoreView(QQuickItem *parent) : QQI_Based_Visuals(parent)
{
    setFlag(ItemHasContents);
    setParent(parent);
    scale = 2.0;
    reloadSpecificTexturesOnResize = true;
    noteTextures = new QSGTexture*[6]();
    redNoteTextures = new QSGTexture*[6]();
    greenNoteTextures = new QSGTexture*[6]();
}

ScoreView::~ScoreView()
{
    textureMutex.lock();
    for(int i = 0; i < 6; i++) {
        if(noteTextures[i] != NULL) {
            delete noteTextures[i];
            delete redNoteTextures[i];
            delete greenNoteTextures[i];
        }
    }
    delete [] noteTextures;
    delete [] greenNoteTextures;
    delete [] redNoteTextures;
    delete dotTexture;
    delete sharpTexture;
    delete keyTexture;
    delete graceTexture;
    delete slurTexture;
    textureMutex.unlock();
}

QSGNode* ScoreView::getScoreViewSpecificNode(qreal x, qreal y, qreal width, qreal height, QSGTexture *texture)
{
    textureMutex.lock();
    QSGSimpleTextureNode* noteNode = new QSGSimpleTextureNode();
    QRectF noteRect = QRectF(x, y, width, height);

    noteNode->setTexture(texture);
    noteNode->setRect(noteRect);
    noteNode->setFlag(QSGNode::OwnedByParent);

    textureMutex.unlock();
    return noteNode;
}

QSGNode* ScoreView::getDisplayOptionSpecificNodes()
{
    QRectF noteRectangle = QRectF(0, 0, width(), height());
    QSGSimpleRectNode* node = new QSGSimpleRectNode(noteRectangle, Qt::transparent);
    node->setFlag(QSGNode::OwnedByParent);

    for(int i = 0; i < 5; i++) {
        int yPos = lowerBound - (toneStep*i);
        node->appendChildNode(getLineNode(0, yPos, width(), yPos, Qt::black));
    }
    node->appendChildNode(getScoreViewSpecificNode(toneStep/2, lowerBound - (toneStep*4) - toneStep * 2,
                                     toneStep * 4, toneStep * 8, keyTexture));

    return node;
}

QSGSimpleRectNode* ScoreView::getNodeForNote(QPointer<Note> note, bool isFromScore)
{
    qreal noteY = getDrawPitchForPitch(note->getPitch());
    QSGSimpleRectNode* node;

    if(noteY == -1)
        return NULL;

    if(isFromScore) {
        qreal noteX = calculatePosition(note, true) + toneStep / 2;

        QRectF noteRectangle = QRectF(noteY - noteYOffset, noteX - noteXOffset, noteWidth, noteHeight);
        node = new QSGSimpleRectNode(noteRectangle, Qt::transparent);
        node->setFlag(QSGNode::OwnedByParent);

        if(noteX >= -toneStep*2 && noteX <= width()+toneStep*2) {
            QSGTexture **source = noteTextures;

            bool targetFound = false;
            for(int j = 0; j < game->getPlayerTrack()->notes.count() && !targetFound; j++) {
                QPointer<Note> tmpPlayedNote = game->getPlayerTrack()->notes[j];
                if(!tmpPlayedNote->getTarget().isNull() && tmpPlayedNote->getDuration() != 0 &&
                        tmpPlayedNote->getTarget() == note) {
                    if(tmpPlayedNote->getTarget()->getPitch() == note->getPitch() && tmpPlayedNote->getScore() > 0) {
                        source = greenNoteTextures;
                    } else {
                        source = redNoteTextures;
                    }
                    targetFound = true;
                }
            }

            if (isSharp(note->getPitch())) {
                node->appendChildNode(getScoreViewSpecificNode(noteX - noteXOffset, noteY - noteYOffset,
                                                               noteWidth, noteHeight, sharpTexture));
            }

            if (note->getDot()) {
                node->appendChildNode(getScoreViewSpecificNode(noteX - noteXOffset, noteY - noteYOffset,
                                                               noteWidth, noteHeight, dotTexture));
            }

            if(note->getGrace()) {
                node->appendChildNode(getScoreViewSpecificNode(noteX - noteXOffset, noteY - noteYOffset,
                                                               noteWidth, noteHeight, graceTexture));
            } else if(note->getRelativeDuration() + 2 >= 0 && note->getRelativeDuration() + 2 < 6){
                node->appendChildNode(getScoreViewSpecificNode(noteX - noteXOffset, noteY - noteYOffset,
                                                               noteWidth, noteHeight, source[note->getRelativeDuration() + 2]));
            }

            if(67 < note->getPitch()) {
                QStack<int> missingLines;
                for(int j = 67; j <= note->getPitch(); j++) {
                    int tmpPitch = (int)getDrawPitchForPitch(j);
                    if(!missingLines.contains(tmpPitch)) {
                        missingLines.push(tmpPitch);
                    }
                }

                int tmpCount = 1;
                foreach (const int &value, missingLines) {
                    if(tmpCount %  2 == 0) {
                        node->appendChildNode(getLineNode(noteX-(toneStep*1.3), value,
                                          noteX+(toneStep*1.3), value, Qt::black));
                    }
                    tmpCount++;
                }
            } else if(note->getPitch() < 50) {
                QStack<int> missingLines;
                for(int j = 50; j >= note->getPitch(); j--) {
                    int tmpPitch = (int)getDrawPitchForPitch(j);
                    if(!missingLines.contains(tmpPitch)) {
                        missingLines.push(tmpPitch);
                    }
                }

                int tmpCount = 1;
                foreach (const int &value, missingLines) {
                    if(tmpCount %  2 == 0) {
                        node->appendChildNode(getLineNode(noteX-(toneStep*1.3), value,
                                          noteX+(toneStep*1.3), value, Qt::black));
                    }
                    tmpCount++;
                }
            }
        }

        if(note->getLinking() == LinkingType::tie) {
            int nextNoteX = calculatePosition(note->getTime() + note->getDuration(), true)  + toneStep / 2;
            node->appendChildNode(getScoreViewSpecificNode(noteX, noteY + toneStep / 2,
                                                           nextNoteX - noteX, toneStep / 2, slurTexture));
        }
    } else {
        qreal noteX = calculatePosition(note, true) + toneStep / 2;

        QRectF noteRectangle = QRectF(noteY - noteYOffset, noteX - noteXOffset, noteWidth, noteHeight);
        node = new QSGSimpleRectNode(noteRectangle, Qt::transparent);
        node->setFlag(QSGNode::OwnedByParent);

        int lineX1;
        if(note->getDuration() == 0) {
            lineX1 = width()/2;
        } else {
            lineX1 = calculatePosition(note->getTime() + note->getDuration(), false) + toneStep / 2;
        }
        int lineX2 = calculatePosition(note->getTime(), false) + toneStep / 2;

        if(!note->getTarget().isNull() && note->getScore() > 0 && lineX1 >= -2000 && lineX1 <= width()+2000) {
            int timeX = calculatePosition(note->getTime(), false) + toneStep / 2;
            int targetTimeX = calculatePosition(note->getTarget()->getTime(), false) + toneStep / 2;

            int durationX;
            if(note->getDuration() != 0) {
                durationX =
                    calculatePosition(note->getTime() + note->getDuration(), false) + toneStep / 2;
            } else {
                durationX = lineX1;
            }

            /*int targetDurationX =
                    calculatePosition(note->getTarget()->getTime() + note->getTarget()->getTieDuration(), false)
                    + toneStep / 2;*/

            //if(note->getDuration() != 0) {
                node->appendChildNode(getLineNode(lineX1, noteY, lineX2, noteY, Qt::green, 10));
            /*} else {
                int correctX = min(targetDurationX, lineX1);
                node->appendChildNode(getLineNode(correctX, noteY, lineX2, noteY, Qt::green, 9));
            }*/

            node->appendChildNode(getLineNode(targetTimeX, noteY, timeX, noteY, Qt::red, 10));

            /*if(note->getDuration() != 0) {
                node->appendChildNode(getLineNode(targetDurationX, noteY, durationX, noteY, Qt::red, 9));
            } else if(targetDurationX < lineX1){
                node->appendChildNode(getLineNode(targetDurationX, noteY, lineX1, noteY, Qt::red, 9));
            }*/
        }

        QSGSimpleRectNode *number = getNumberNode(note->getScore(),
                                                  toneStep,
                                                  noteX,
                                                  noteY - (toneStep / 2));
        node->appendChildNode(number);
    }

    return node;
}

void ScoreView::loadDisplayOptionSpecificTextures() // we really need a texture manager...
{
    if (window() != NULL && oldHeight != window()->height() && calculateLines())
    {
        textureMutex.lock();

        for(int i = 0; i < 6; i++) {
            loadNoteTexture(i, qRgba(0, 0, 0, 255), noteTextures);
            loadNoteTexture(i, qRgba(255, 0, 0, 255), redNoteTextures);
            loadNoteTexture(i, qRgba(0, 255, 0, 255), greenNoteTextures);
        }

        if(dotTexture)
            delete dotTexture;
        dotTexture = loadTextureFromSVG(QString("graphics/notes/dot.svg"), noteWidth, noteHeight);

        if(sharpTexture)
            delete sharpTexture;
        sharpTexture = loadTextureFromSVG(QString("graphics/notes/sharp.svg"), noteWidth, noteHeight);

        if(graceTexture)
            delete graceTexture;
        graceTexture = loadTextureFromSVG(QString("graphics/notes/grace.svg"), noteWidth, noteHeight);

        if(slurTexture)
            delete slurTexture;
        slurTexture = loadTextureFromSVG(QString("graphics/notes/tie.svg"), width(), height());

        if(keyTexture)
            delete keyTexture;
        keyTexture = loadTextureFromSVG(QString("graphics/notes/key.svg"), toneStep*4, toneStep*8);

        textureMutex.unlock();
    }
}

QImage ScoreView::colorizeQImage(QImage image, QRgb color)
{
    QImage newImage(image.width(), image.height(), QImage::Format_ARGB32);
    QRgb pixelVal;
    QRgb black = qRgba(0, 0, 0, 255);

    for(int x = 0; x < image.width(); x++)
    {
        for(int y = 0; y < image.height(); y++)
        {
            pixelVal = image.pixel(x,y);
            if (pixelVal == black)
                newImage.setPixel(x, y, color);
            else
                newImage.setPixel(x, y, pixelVal);
        }
    }
    return newImage;
}

void ScoreView::loadNoteTexture(int i, QRgb color, QSGTexture **destination)
{
    QString texturePath("");
    QRgb black = qRgba(255, 255, 255, 255);

#if !defined(Q_OS_ANDROID)
    texturePath = QString("graphics/notes/") + QString::number(i+1) + QString(".svg");
#else
    texturePath = QString("assets:/graphics/notes/") + QString::number(i+1) + QString(".svg");
#endif

    QSvgRenderer svgRenderer(texturePath);
    QImage noteImage(noteWidth, noteHeight, QImage::Format_ARGB32);
    noteImage.fill(0xFFFFFF);
    QPainter painter(&noteImage);
    painter.setRenderHint(QPainter::Antialiasing);
    svgRenderer.render(&painter);
    if(destination[i] != NULL) {
        delete destination[i];
    }
    if(color != black) {
        QImage newImage = noteImage.copy(noteImage.rect());
        newImage = colorizeQImage(newImage, color);
        destination[i] = window()->createTextureFromImage(newImage);
    } else {
        destination[i] = window()->createTextureFromImage(noteImage);
    }
}

bool ScoreView::calculateLines()
{
    // Calculate the number of pitches to draw, also ignore sharp ones
    QList<int> tmpPitchSet;
    for(int i = 0; i < this->game->getTargetTrack()->notes.length(); i++) {
        if(!tmpPitchSet.contains(this->game->getTargetTrack()->notes.at(i)->getPitch())) {
            tmpPitchSet.append(this->game->getTargetTrack()->notes.at(i)->getPitch());
        }
    }
    if(!tmpPitchSet.contains(65)) {
        tmpPitchSet.append(65);
    }
    if(!tmpPitchSet.contains(52)) {
        tmpPitchSet.append(52);
    }

    qSort(tmpPitchSet.begin(), tmpPitchSet.end());

    int actualPitches = tmpPitchSet.last() - tmpPitchSet[0] + 1;
    int drawablePitches = actualPitches - numberOfDoubles(tmpPitchSet[0], tmpPitchSet.last());
    drawablePitches += 2; // add for headroom on top and bottom

    cout << "first " << tmpPitchSet.first() << endl;
    cout << "last " << tmpPitchSet.last() << endl;

    toneStep = boundingRect().height()/drawablePitches;
    xOffset = toneStep;
    noteWidth = toneStep*6;
    noteHeight = toneStep*12;
    noteXOffset = toneStep*3;
    noteYOffset = ((toneStep/4)*3)*12;

    for(int i = 0; i < actualPitches; i++) {
        int pitch = (tmpPitchSet[0] + i);
        if(!isSharp(pitch)) {
            if(i != 0) {
                yPositions[pitch] = yPositions[pitch - 1] - (toneStep/2);
            } else {
                yPositions[pitch] = boundingRect().height() - (toneStep*3);
            }
        } else {
            if(i != 0) {
                yPositions[pitch] = yPositions[pitch - 1];
            } else {
                yPositions[pitch] = boundingRect().height() - (toneStep*3);
            }
        }
    }

    lowerBound = getDrawPitchForPitch(52); // E1
    upperBound = getDrawPitchForPitch(65); // F2

    /*for(int i = 0; i < tmpPitchSet.count(); i++) {
        qDebug() << "pitch: " << tmpPitchSet[i] << " Y: " << yPositions[tmpPitchSet[i]];
    }*/

    return true;
}

qreal ScoreView::getDrawPitchForPitch(int pitch)
{
    if(yPositions.contains(pitch)) {
        return yPositions[pitch];
    } else {
        return -1;
    }
}

bool ScoreView::isSharp(int pitch)
{
    int nPitch = pitch % 12;
    if(nPitch == 1 || nPitch == 3 || nPitch == 6 || nPitch == 8 || nPitch == 10) {
        return true;
    } else {
        return false;
    }
}

int ScoreView::numberOfDoubles(int startPitch, int pitch)
{
    int unneededPitches = 0;
    for (int i = startPitch; i < pitch; i++) {
        if(isSharp(i)) {
            unneededPitches++;
        }
    }
    return unneededPitches;
}
