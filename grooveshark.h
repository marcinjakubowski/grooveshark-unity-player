#ifndef GROOVESHARK_H
#define GROOVESHARK_H

#include <unity/unity/unity.h>
#include <libnotify/notify.h>
#include <gtk/gtk.h>

#include <QMainWindow>
#include <QWebView>
#include <QUrl>

#include "optionsdialog.h"

namespace Ui {
class Grooveshark;
}

class Grooveshark : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(PlayerState state READ getState NOTIFY stateChanged)

public:
    enum PlayerState
    {
        PLAYING,
        PAUSED
    };

    explicit Grooveshark(QWidget *parent = 0);
    ~Grooveshark();

    PlayerState getState() const { return playerState; }

signals:
    void progressChanged(float progress);
    void songChanged(const QString& title, const QString& artist, const QString& album = QString(), const QString& artwork = QString());
    void paused();
    void playing();
    void stateChanged(PlayerState newState);

private slots:
    void onSongChanged(const QString& title, const QString& artist, const QString& album, const QString& artwork);
    void onPlayerStateChanged(PlayerState newState);
    void onProgressChanged(float progress);
    void finished();
    void parse();

private slots:
    void showOptions();

public slots:
    void notify(const QString& title, const QString& body, const QString& icon = QString());
    void play();
    void pause();
    void next();
    void prev();

private:
    QNetworkAccessManager* artDownloader;
    QString currentSongTitle, currentSongArtist, currentSongAlbum, currentSongArtwork;
    int currentSongId;

    QTimer *parseTimer;
    bool initialized;
    void initialize();

    int lastSongId;
    PlayerState playerState;


    Ui::Grooveshark *ui;
    QWebView *webView;
    QWebFrame *frame;
    OptionsDialog options;
    bool hasTriedLogin;

    static const QUrl URL_DEFAULT;
    static const QUrl URL_LOGIN;
};

#endif // GROOVESHARK_H
