#ifndef QQI_BASED_VISUALS_H
#define QQI_BASED_VISUALS_H

#include <QQuickItem>
#include <QSGGeometry>
#include <QSGFlatColorMaterial>
#include <QSGSimpleRectNode>
#include <QMutex>
#include <QMutexLocker>

#include "../music/track.h"
#include "../game/maingame.h"
#include "visuals.h"



class QQI_Based_Visuals : public Visuals
{
    Q_OBJECT

public:
    QQI_Based_Visuals(QQuickItem* parent = 0);
    ~QQI_Based_Visuals();

    // Set Methods
    void setGame(const QObject *game);

    void loadDigitTextures();

    QSGNode *getLineNode(qreal x0, qreal y0, qreal x1, qreal y1);
    QSGNode *getLineNode(qreal x0, qreal y0, qreal x1, qreal y1, QColor color);
    QSGNode *getLineNode(qreal x0, qreal y0, qreal x1, qreal y1, QColor color, int lineWidth);
    QSGNode *getNowLine();
 

    QSGNode *getPlayedNotes();
protected:
    qreal scale = 1.0; // spread notes apart
    qreal xOffset = 0.0;
    bool reloadSpecificTexturesOnResize = false;
    qreal oldHeight = 0.0;

    QImage createSubImage(QImage *image, const QRect &rect);

    QList<int> numberToDigits(int number);
    QSGSimpleRectNode *getNumberNode(int number, int size, int left, int top);

    void loadNonSpecificTextures();

    QList<QSGTexture*> digitTextures;
    QSGNode *getTargetNotes();

    virtual QSGSimpleRectNode* getNodeForNote(QPointer<Note> note, bool isFromScore = false);
    virtual QSGNode* getDisplayOptionSpecificNodes();
    virtual void loadDisplayOptionSpecificTextures();
    virtual void displayOptionSpecificPreparations();

    void loadTextures();

    int getLastIndex();

    qreal calculatePosition(QPointer<Note> note, bool isFromScore);
    qreal calculatePosition(int time, bool isFromScore);

    int trackIndex;
    int startIndex;
    int startMs;

    bool texturesAvailable = false;

    QSGTexture* loadTextureFromSVG(QString texturePath, qreal width, qreal height);

    QSGTexture* leftTriangleTexture = NULL;
    QSGTexture* rightTriangleTexture = NULL;

    qreal gameTime;
    qreal getNoteRectangleWidth(QPointer<Note> note);
    QSGSimpleRectNode *getBorderedRectangleNode(qreal left, qreal top, qreal width, qreal height);
    QSGSimpleRectNode *getPlayerNoteRectanlgeNode(QPointer<Note> note, qreal left, qreal top, qreal width, qreal height);
    QSGNode *getBarsNode();

    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data);
};

#endif // QQI_BASED_VISUALS_H
