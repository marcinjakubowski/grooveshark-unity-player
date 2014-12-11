#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog
{

    Q_OBJECT
    Q_PROPERTY(bool autoLogin READ isAutoLogin);
    Q_PROPERTY(QString username READ getUsername);
    Q_PROPERTY(QString password READ getPassword);

public:
    explicit OptionsDialog(QWidget *parent = 0);
    ~OptionsDialog();

public:
    bool isAutoLogin() const;
    const QString& getUsername() const;
    const QString& getPassword() const;

public slots:
    void saveSettings();

private:
    Ui::OptionsDialog *ui;
    QString settingsPath;
    bool autoLogin;
    QString username, password;

    static const QString SETTINGS_AUTOLOGIN;
    static const QString SETTINGS_USERNAME;
    static const QString SETTINGS_PASSWORD;

};

#endif // OPTIONSDIALOG_H
