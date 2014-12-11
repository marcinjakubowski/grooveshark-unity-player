#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>

const QString OptionsDialog::SETTINGS_AUTOLOGIN = "autologin";
const QString OptionsDialog::SETTINGS_USERNAME = "username";
const QString OptionsDialog::SETTINGS_PASSWORD = "password";

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDialog),
    settingsPath(QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).filePath("settings.ini"))
{
    ui->setupUi(this);

    QSettings settings(settingsPath, QSettings::NativeFormat, this);

    autoLogin = settings.value(SETTINGS_AUTOLOGIN, false).toBool();
    username = settings.value(SETTINGS_USERNAME, QString("")).toString();
    password = settings.value(SETTINGS_PASSWORD, QString("")).toString();

    ui->autoLogin->setChecked(autoLogin);
    ui->formGroupBox->setEnabled(autoLogin);

    ui->username->setText(username);
    ui->password->setText(password);

}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::saveSettings()
{
    autoLogin = ui->autoLogin->isChecked();
    username = ui->username->text();
    password = ui->password->text();

    QSettings settings(settingsPath, QSettings::NativeFormat, this);

    settings.setValue(SETTINGS_AUTOLOGIN, autoLogin);
    settings.setValue(SETTINGS_USERNAME, username);
    settings.setValue(SETTINGS_PASSWORD, password);
}


bool OptionsDialog::isAutoLogin() const
{
    return autoLogin;
}

const QString& OptionsDialog::getPassword() const
{
    return password;
}

const QString& OptionsDialog::getUsername() const
{
    return username;
}
