#include <QApplication>
#include "qtquick2applicationviewer.h"
#include <QQmlContext>

#include <string>
#include <vector>
#include <iostream>
#include <QObject>
#include <QQmlEngine>
#include <QBuffer>
#include <QDir>
#include <QThread>
#ifdef Q_WS_WIN
    #include <Windows.h>
#else
    #include <unistd.h>
#endif

#include "visuals/pianoroll.h"
#include "visuals/scoreview.h"
#include "menu/loadingthread.h"
#include "visuals/tablatureview.h"

#ifdef Q_OS_ANDROID
    #include <jni.h>
    #include <input-android/androidglue.h>
#endif
#ifdef Q_OS_LINUX
    #include <stdlib.h>
#endif

using namespace std;

#ifdef Q_OS_ANDROID
static AndroidGlue *androidGlue = new AndroidGlue();
#endif

//Q_DECLARE_METATYPE(shared_ptr<Note>)
Q_DECLARE_METATYPE(Track*)

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    if (const auto snapEnv = getenv("SNAP")) {
        const auto resourceRoot = std::string(snapEnv) + "/opt/BeatTheScore";
        chdir(resourceRoot.c_str());
    }
#endif

    QApplication app(argc, argv);
    QtQuick2ApplicationViewer viewer;

    // We really should set a predictable path for settings
    #if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
        QDir configPath(QString(getenv("HOME")) + "/.config/beatthescore");
        if(!configPath.exists()) {
            configPath.mkdir(configPath.path());
        }
        viewer.engine()->setOfflineStoragePath(configPath.path());
    #elif defined(Q_OS_ANDROID)
        viewer.engine()->setOfflineStoragePath("/data/data/at.beatthescore/");
    #else
    // Windows here
    #endif

    cout << "viewer.engine()->offlineStoragePath() " << viewer.engine()->offlineStoragePath().toStdString() << endl;

    #ifdef Q_OS_ANDROID
    //freopen ("/storage/emulated/0/bts_log.txt","w",stdout);
    //freopen ("/storage/emulated/0/bts_err.txt","w",stderr);
    //cout << "BTS Midi Log:" << endl;
    #endif

    qmlRegisterType<Visuals>("Visuals", 1, 0, "Visuals");
    qmlRegisterType<PianoRoll>("PianoRoll", 1, 0, "PianoRoll");
    qmlRegisterType<ScoreView>("ScoreView", 1, 0, "ScoreView");
    qmlRegisterType<TablatureView>("TablatureView", 1, 0, "TablatureView");

    //qRegisterMetaType<shared_ptr<Note>>(new shared_ptr<Note>(new Note));

    // Prevent Console outout if argument -noConsoleOut provided
    QList<string> argumentList = QList<string>::fromStdList(std::list<string>(argv, argv + argc));
    if (argumentList.contains("-noConsoleOut"))
    {
        streambuf* consolebuffer = std::cout.rdbuf ();
        std::cout.rdbuf (NULL);
        //freopen(NULL, "w", stdout);
    }

    // Will be deleted automatically before GUI
    SoundNavigationHandler* uiCommandHandler = new SoundNavigationHandler(& viewer);

    QPointer<MainGame> game = new MainGame();

    #ifndef Q_OS_ANDROID
    QObject::connect(&app, SIGNAL(aboutToQuit()), game.data(), SLOT(cleanup()), Qt::DirectConnection);
    QObject::connect(&viewer, SIGNAL(closing(QQuickCloseEvent*)), game.data(), SLOT(cleanup()), Qt::DirectConnection);
    #endif

    game->setUiCommandHandler(uiCommandHandler);

    #ifdef Q_OS_ANDROID
    game->setAndroidGlue(androidGlue);
    #endif

    qmlRegisterType<MainGame>("BeatTheScore", 1, 0, "MainGame");
    viewer.rootContext()->setContextProperty("mainGame", game);

    QPointer<LicenseManager> licenseManager = new LicenseManager();
    game->setLicenseManager(licenseManager);
    qmlRegisterType<LicenseManager>("LicenseManager", 1, 0, "LicenseManager");
    viewer.rootContext()->setContextProperty("licenseManager", licenseManager);

    viewer.setMainQmlFile(QStringLiteral("qml/DisplayLoader.qml"));
    viewer.showExpanded();

    if (argc > 1 && strcmp(argv[1], "-f") == 0)
        viewer.showFullScreen();

    LoadingThread loadingThread(0);
    loadingThread.game = game;
    loadingThread.start(QThread::HighestPriority);

    //viewer.rootContext()->setContextProperty("deviceModel", QVariant::fromValue(inputs));
    //viewer.showExpanded();
    //viewer.showMaximized();

    if (!(argc > 1 && strcmp(argv[1], "-w") == 0))
        viewer.showFullScreen();

    /*
    viewer.showFullScreen();
    bool fs = viewer.visibility() == viewer.FullScreen;
    viewer.showNormal();
    bool nfs = viewer.visibility() == viewer.FullScreen;*/

    return app.exec();
}
