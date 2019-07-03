// Copyright (c) 2011-2018 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_QT_UPDATENOTIFICATIONDIALOG_H
#define POCKETCOIN_QT_UPDATENOTIFICATIONDIALOG_H

#include <QDialog>

namespace Ui {
    class UpdateNotificationDialog;
}

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE

class UpdateNotificationDialog : public QDialog
{
    Q_OBJECT

public:

    explicit UpdateNotificationDialog(QString url, QString cur_version, QString new_version, QWidget *parent = 0);
    ~UpdateNotificationDialog();

public Q_SLOTS:
    void accept();

private:
    Ui::UpdateNotificationDialog *ui;
    QDataWidgetMapper *mapper;

    QString _url;
};

#endif // POCKETCOIN_QT_UPDATENOTIFICATIONDIALOG_H
