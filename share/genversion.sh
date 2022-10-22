#!/bin/sh
# Copyright (c) 2012-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

GIT_TAG=""
GIT_COMMIT=""
if [ "${BITCOIN_GENBUILD_NO_GIT}" != "1" ] && [ -e "$(command -v git)" ] && [ "$(git rev-parse --is-inside-work-tree 2>/dev/null)" = "true" ]; then
    # clean 'dirty' status of touched files that haven't been modified
    git diff >/dev/null 2>/dev/null

    # if latest commit is tagged and not dirty, then override using the tag name
    RAWDESC=$(git describe --abbrev=0 2>/dev/null)
    if [ "$(git rev-parse HEAD)" = "$(git rev-list -1 $RAWDESC 2>/dev/null)" ]; then
        git diff-index --quiet HEAD -- && GIT_TAG=$RAWDESC
    fi

    # otherwise generate suffix from git, i.e. string like "59887e8-dirty"
    GIT_COMMIT=$(git rev-parse --short HEAD)
    git diff-index --quiet HEAD -- || GIT_COMMIT="$GIT_COMMIT-dirty"
fi

if [ -n "$GIT_TAG" ]; then
    NEWINFO=$GIT_TAG
elif [ -n "$GIT_COMMIT" ]; then
    NEWINFO=$GIT_COMMIT
else
    NEWINFO="---"
fi

echo $NEWINFO
