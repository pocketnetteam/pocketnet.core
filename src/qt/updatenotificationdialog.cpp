// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/updatenotificationdialog.h>
#include <qt/forms/ui_updatenotificationdialog.h>

#include <qt/guiutil.h>

#include <QDataWidgetMapper>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>


UpdateNotificationDialog::UpdateNotificationDialog(QString url, QString cur_version, QString new_version, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdateNotificationDialog),
    mapper(0)
{
    ui->setupUi(this);

    QString label_text = "New version of Pocketnet Core available\n";
    label_text += "To update node you have to download new version. Stop node. Install new version and run it.\n\n";
    label_text += "New version - " + new_version + "\nCurrent version - " + cur_version + "\n\n";
    label_text += "Click OK to go to the latest version download page.";
    ui->label_1->setText(label_text);

    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);

    GUIUtil::ItemDelegate* delegate = new GUIUtil::ItemDelegate(mapper);
    connect(delegate, &GUIUtil::ItemDelegate::keyEscapePressed, this, &UpdateNotificationDialog::reject);
    mapper->setItemDelegate(delegate);

    _url = url;
}

UpdateNotificationDialog::~UpdateNotificationDialog()
{
    delete ui;
}

void UpdateNotificationDialog::accept()
{
    QDesktopServices::openUrl(QUrl(_url));    
    QDialog::accept();
}
