// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_QT_POCKETCOINADDRESSVALIDATOR_H
#define POCKETCOIN_QT_POCKETCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class PocketcoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit PocketcoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Pocketcoin address widget validator, checks for a valid pocketcoin address.
 */
class PocketcoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit PocketcoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // POCKETCOIN_QT_POCKETCOINADDRESSVALIDATOR_H
