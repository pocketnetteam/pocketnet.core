#!/usr/bin/env bash

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

POCKETCOIND=${POCKETCOIND:-$BINDIR/pocketcoind}
POCKETCOINCLI=${POCKETCOINCLI:-$BINDIR/pocketcoin-cli}
POCKETCOINTX=${POCKETCOINTX:-$BINDIR/pocketcoin-tx}
POCKETCOINQT=${POCKETCOINQT:-$BINDIR/qt/pocketcoin-qt}

[ ! -x $POCKETCOIND ] && echo "$POCKETCOIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
POCVER=($($POCKETCOINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for pocketcoind if --version-string is not set,
# but has different outcomes for pocketcoin-qt and pocketcoin-cli.
echo "[COPYRIGHT]" > footer.h2m
$POCKETCOIND --version | sed -n '1!p' >> footer.h2m

for cmd in $POCKETCOIND $POCKETCOINCLI $POCKETCOINTX $POCKETCOINQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${POCVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${POCVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
