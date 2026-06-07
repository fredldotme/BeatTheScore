#include <iostream>
#include <memory>
#include <algorithm>

#include <QQuickItem>
#include <QSGGeometry>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGSimpleRectNode>
#include <QSGSimpleTextureNode>
#include <QTimer>
#include "pianoroll.h"
#include "../music/note.h"
#include "../music/section.h"
#include "visuals.h"
#include "qqi_based_visuals.h"

using namespace std;


PianoRoll::PianoRoll(QQuickItem *parent) : QQI_Based_Visuals(parent)
{
    // Important, otherwise the paint method is never called
    setFlag(ItemHasContents);
    rightTriangleTexture = NULL;
    leftTriangleTexture = NULL;
    pianoTexture = NULL;
}

PianoRoll::~PianoRoll()
{
    if (pianoTexture)
        delete pianoTexture;
}


qreal PianoRoll::getYForPitch(int pitch) {
    //qreal pitchPosition[] = {0, 2, 3, 5, 6, 8, 10, 11, 13, 14, 16, 17}; // 19
    //return boundingRect().bottom() - ((pitchPosition[pitch % 12] / 19 + pitch / 12) * 12 - pitchOffset) * pitchHeight;
    return boundingRect().bottom() - (pitch - pitchOffset) * pitchHeight;
}


QSGNode *PianoRoll::getDisplayOptionSpecificNodes()
{

    QSGNode* node = new QSGNode();

    pitchOffset = this->game->getTargetTrack()->getMinPitch() - 3; // show 3 extra pitches
    pitchesPerScreen = this->game->getTargetTrack()->getMaxPitch() + 2 - pitchOffset;
    pitchHeight = (qreal) boundingRect().height() / pitchesPerScreen;
    qreal octaveHeight = pitchHeight * 12;


    // false = black, true = white
    bool keyColor[12] = {true, false, true, false, true, true, false, true, false, true, false, true};
    for (int i=0; i<128; i+=12)
    {
        for (int j=0; j<12; j++)
        {
            int pitch = i + j;
            QRectF keyLine = QRectF(boundingRect().left(), getYForPitch(pitch), // TODO
                                    boundingRect().width(), getYForPitch(pitch) - getYForPitch(pitch + 1));
            QSGSimpleRectNode* newRectNode = new QSGSimpleRectNode(keyLine, keyColor[j]?QColor(255,255,255,0):QColor(0,0,0,63));
            node->appendChildNode(newRectNode);

            if (noteLight.contains(pitch))
            {
                QSGSimpleRectNode* lightRectNode = new QSGSimpleRectNode(keyLine, QColor(128,128,255,noteLight[pitch]));
                node->appendChildNode(lightRectNode);

                noteLight[pitch] -= 127 / 0.25 / 60; // Hacky? Maybe. Definitely.
                if (noteLight[pitch] <= 0) {
                    noteLight.remove(pitch);
                }
            }
        }

        QRectF pianoRect = QRectF(boundingRect().left(),
                                  getYForPitch(i - 1),
                                  octaveHeight / 2,
                                  octaveHeight);

        if (pianoTexture)
        {
            QSGSimpleTextureNode* pianoNode = new QSGSimpleTextureNode();
            pianoNode->setTexture(pianoTexture);
            pianoNode->setRect(pianoRect);
            node->appendChildNode(pianoNode);
        }

        auto numberNode = getNumberNode(i / 12 - 2, pitchHeight, pitchHeight, pianoRect.bottom() - pitchHeight);
        if (numberNode)
            node->appendChildNode(numberNode);
    }

    return node;
}


QSGSimpleRectNode* PianoRoll::getNodeForNote(QPointer<Note> note, bool isFromScore)
{
    if (note->getMuted())
    {
        return NULL;
    }

    qreal left = calculatePosition(note, isFromScore);

    qreal top = getYForPitch(note->getPitch());
    qreal width;
    qreal height = top - getYForPitch(note->getPitch() + 1);

    width = getNoteRectangleWidth(note);

    QSGSimpleRectNode* newRectNode;
    QRectF noteRectangle = QRectF(left, top, width, height);

    if (isFromScore)
    {
        newRectNode = getBorderedRectangleNode(left, top, width, height);
    }
    else
    {
        newRectNode = getPlayerNoteRectanlgeNode(note, left, top, width, height);

        if (note->getDuration() > 0)
        {
            QSGSimpleRectNode *number = getNumberNode(note->getScore(),
                                                  newRectNode->rect().height(),
                                                  newRectNode->rect().right() - newRectNode->rect().height() * 2,
                                                  newRectNode->rect().top());
            newRectNode->appendChildNode(number);
        }
    }

    return newRectNode;
}


void PianoRoll::loadDisplayOptionSpecificTextures()
{
    if (window() != NULL)
    {
        QImage myImage;

        #ifdef Q_OS_ANDROID
            cout << myImage.load("assets:/graphics/piano.png");
        #else
            cout << myImage.load("graphics/piano.png");
        #endif
        pianoTexture = window()->createTextureFromImage(myImage.copy());
    }
}


void PianoRoll::noteOn(Note *note)
{
    noteLight[note->getPitch()] = 127;
}




