#include "grooveshark.h"
#include "ui_grooveshark.h"



#include <QtWebKit>
#include <QWebView>
#include <QWebFrame>
#include <QMessageBox>

static Grooveshark* instance = NULL;

static UnityMusicPlayer *unityMusicPlayer = NULL;
static UnityTrackMetadata *trackMetadata = NULL;
static UnityLauncherEntry *unityLauncher = NULL;
static NotifyNotification *notification = NULL;

const QUrl Grooveshark::URL_DEFAULT = QUrl("http://html5.grooveshark.com");
const QUrl Grooveshark::URL_LOGIN = QUrl("http://html5.grooveshark.com/#!/login");



void onPlayPause(GtkWidget *, gpointer )
{
    if (instance->getState() == Grooveshark::PLAYING) instance->pause();
    else instance->play();
}

void onPrevious(GtkWidget *, gpointer )
{
    instance->prev();
}

void onRaise(GtkWidget *, gpointer )
{
    instance->setWindowState( (instance->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    instance->raise();
    instance->activateWindow();
}

void onNext(GtkWidget *, gpointer )
{
    instance->next();
}


Grooveshark::Grooveshark(QWidget *parent) :
    QMainWindow(parent),
    initialized(false),
    playerState(PAUSED),
    ui(new Ui::Grooveshark),
    options(this),
    hasTriedLogin(false)
{
    QCoreApplication::setApplicationName("grooveshark");
    QCoreApplication::setApplicationVersion("1.0");
    ui->setupUi(this);

    webView = new QWebView(this);
    frame = webView->page()->mainFrame();

    artDownloader = new QNetworkAccessManager(this);
    QString settingsFile = QStandardPaths::locate(QStandardPaths::DataLocation, "settings.ini");


    QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    if (!cacheDir.exists())
    {
        cacheDir.mkpath(".");
    }

    QUrl loadUrl = QUrl(URL_DEFAULT);


    if (options.isAutoLogin())
    {
        loadUrl = QUrl(URL_LOGIN);
    }

    webView->setUrl(loadUrl);
    ui->verticalLayout->addWidget(webView);

    connect(webView, SIGNAL(loadFinished(bool)), this, SLOT(finished()));

}



Grooveshark::~Grooveshark()
{
    delete parseTimer;

    if (unityMusicPlayer) unity_music_player_unexport(unityMusicPlayer);
    delete artDownloader;
    delete webView;
    delete ui;
}

void Grooveshark::onPlayerStateChanged(PlayerState newState)
{
    unity_music_player_set_playback_state(unityMusicPlayer, newState == PAUSED ? UNITY_PLAYBACK_STATE_PAUSED : UNITY_PLAYBACK_STATE_PLAYING);
}

void Grooveshark::initialize()
{
    instance = this;
    unityLauncher = unity_launcher_entry_get_for_desktop_file("grooveshark-unity-player.desktop");
    unity_launcher_entry_set_progress_visible(unityLauncher, true);
    unityMusicPlayer = unity_music_player_new("grooveshark-unity-player.desktop");
    if (!unityMusicPlayer)
    {
        QMessageBox::critical(this, "Error", "Error creating Unity music player object");
        return ;
    }

    // Register music player
    unity_music_player_set_title(unityMusicPlayer, "Grooveshark");
    unity_music_player_set_can_go_next(unityMusicPlayer, true);
    unity_music_player_set_can_play(unityMusicPlayer, true);
    unity_music_player_export(unityMusicPlayer);

    trackMetadata = unity_track_metadata_new();

    // connect signals from media indicator
    g_signal_connect (G_OBJECT (unityMusicPlayer), "play_pause", G_CALLBACK (onPlayPause), NULL);
    g_signal_connect (G_OBJECT (unityMusicPlayer), "next", G_CALLBACK (onNext), NULL);
    g_signal_connect (G_OBJECT (unityMusicPlayer), "previous", G_CALLBACK (onPrevious), NULL);
    g_signal_connect (G_OBJECT (unityMusicPlayer), "raise", G_CALLBACK (onRaise), NULL);



    connect(this, SIGNAL(songChanged(QString,QString,QString,QString)), this, SLOT(onSongChanged(QString,QString,QString,QString)));
    connect(this, SIGNAL(stateChanged(PlayerState)), this, SLOT(onPlayerStateChanged(PlayerState)));
    connect(this, SIGNAL(progressChanged(float)), this, SLOT(onProgressChanged(float)));

    notify_init("Grooveshark");
    notification = notify_notification_new("", "", "");



    parseTimer = new QTimer(this);
    connect(parseTimer, SIGNAL(timeout()), this, SLOT(parse()));
    parseTimer->start(1000);
    emit stateChanged(PAUSED);

    initialized = true;
}

void Grooveshark::onProgressChanged(float progress)
{
    unity_launcher_entry_set_progress(unityLauncher, progress);
}

void Grooveshark::parse()
{
    static float lastProgress = 999999999999999;
    QVariant state = frame->evaluateJavaScript("window.GS.audio.getState();");

    if (state.type() != QVariant::List)
        return;

    QVariantList stateList = state.toList();

    if (stateList.size() != 5)
    {
        //ui->statusBar->showMessage(tr("Error when parsing state: incorrent state list size."), 2000);
        return;
    }

    float progress = stateList[2].toFloat();
    float total = stateList[3].toFloat();



    if (progress != lastProgress)
    {
        if (playerState != PLAYING)
        {
            playerState = PLAYING;
            emit playing();
            emit stateChanged(PLAYING);
        }
        emit progressChanged(progress/total);
    }
    else if (playerState != PAUSED)
    {
        playerState = PAUSED;
        emit paused();
        emit stateChanged(PAUSED);
    }
    else
    {
        return ;
    }

    // next song?
    if (progress < lastProgress)
    {
        QVariant songInfo = frame->evaluateJavaScript("window.GS.audio.model.toJSON();");
        if (songInfo.type() != QVariant::Map)
        {
            ui->statusBar->showMessage(tr("Error when parsing state: model is not a map."), 2000);
        }
        else
        {
            QVariantMap songMeta = songInfo.toMap();
            int songId = songMeta["SongID"].toInt();

            if (songId != currentSongId)
            {
                currentSongAlbum = songMeta["AlbumName"].toString();
                currentSongArtist = songMeta["ArtistName"].toString();
                currentSongTitle = songMeta["name"].toString();
                currentSongId = songId;
                currentSongArtwork = songMeta["AlbumID"].toString().append(".jpg");

                QString artCache = QStandardPaths::locate(QStandardPaths::CacheLocation, currentSongArtwork);
                // cache miss
                if (artCache.isEmpty())
                {
                    QString artworkUrl = songMeta["coverURL500"].toString();

                    QEventLoop loop;
                    QNetworkReply *reply = artDownloader->get(QNetworkRequest(QUrl(artworkUrl)));
                    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
                    loop.exec();

                    if (reply->error() == QNetworkReply::NoError)
                    {
                        artCache = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath(currentSongArtwork);
                        {
                            QFile artCacheFile(artCache);
                            artCacheFile.open(QIODevice::WriteOnly);
                            artCacheFile.write(reply->readAll());
                        }
                    }
                    else
                    {
                        ui->statusBar->showMessage(tr("Error downloading artwork for %1 by %2: %3.").arg(currentSongTitle, currentSongArtist, reply->errorString()), 2000);
                        artCache = "unity-grooveshark-webapp";
                    }

                    delete reply;
                }
                currentSongArtwork = artCache;
                emit songChanged(currentSongTitle, currentSongArtist, currentSongAlbum, currentSongArtwork);
            }

        }
    }

    lastProgress = progress;
}

void Grooveshark::showOptions()
{
    options.show();
}

void Grooveshark::onSongChanged(const QString &title, const QString &artist, const QString &album, const QString &artwork)
{
    ui->statusBar->showMessage(tr("Playing %1 by %2 from %3.").arg(title, artist, album), 2000);
    notify(title, QString("%1\n%2").arg(album, artist), artwork);
    unity_track_metadata_set_title(trackMetadata, title.toUtf8().data());
    unity_track_metadata_set_artist(trackMetadata, artist.toUtf8().data());
    unity_track_metadata_set_album(trackMetadata, album.toUtf8().data());

    GFile *coverFile = g_file_new_for_path(artwork.toLatin1().data());
    unity_track_metadata_set_art_location(trackMetadata, coverFile);
    g_object_unref(coverFile);

    unity_music_player_set_current_track(unityMusicPlayer,trackMetadata);

}

void Grooveshark::notify(const QString &title, const QString &body, const QString &icon)
{
    notify_notification_update(notification, title.toUtf8().data(), body.toUtf8().data(), icon.toUtf8().data());
    notify_notification_show(notification, NULL);
}

void Grooveshark::finished()
{
    if (!initialized)
    {
        initialize();
    }
    if (webView->url() == URL_LOGIN && !hasTriedLogin) {
        // we assume jquery is available, fill in username & password and submit the form
        frame->evaluateJavaScript(QString("$('input[name=username]').val('%1')").arg(options.getUsername()));
        frame->evaluateJavaScript(QString("$('input[name=password]').val('%1')").arg(options.getPassword()));
        frame->evaluateJavaScript("$('form.login-form').submit()");
        hasTriedLogin = true;
    }
}

void Grooveshark::play()
{
    if (playerState == PAUSED)
    {
        frame->evaluateJavaScript("window.GS.audio.pauseResume();");
        playerState = PLAYING;
        emit playing();
        emit stateChanged(PLAYING);
    }
}

void Grooveshark::pause()
{
    if (playerState == PLAYING)
    {
        frame->evaluateJavaScript("window.GS.audio.pauseResume();");
        playerState = PAUSED;
        emit paused();
        emit stateChanged(PAUSED);
    }
}

void Grooveshark::next()
{
    frame->evaluateJavaScript("window.GS.audio.playNext();");

}

void Grooveshark::prev()
{

}
