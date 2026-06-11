#ifndef AUDIOINPUTMANAGER_H
#define AUDIOINPUTMANAGER_H

#include <QStringList>
#include <QPointer>

#include "audioinput.h"

class MainGame;

class AudioInputManager
{
public:
    AudioInputManager();
    static QStringList getInputPortList();
    static QList<QPointer<Input>> getInputs(MainGame* game, QObject *parent);
};

#endif // AUDIOINPUTMANAGER_H
