#!/bin/bash

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
    NEWINFO="// No build information available"
fi

SCRIPT_DIR=$(dirname "${BASH_SOURCE[0]}")
VERSION="$(grep PACKAGE_VERSION $SCRIPT_DIR/../build_msvc/pocketcoin_config.h | sed 's/.*PACKAGE_VERSION "\(.*\)"/\1/')-$NEWINFO"

if [ "${BITCOIN_GENBUILD_NO_GIT}" != "1" ] && [ -e "$(command -v git)" ] && [ "$(git rev-parse --is-inside-work-tree 2>/dev/null)" = "true" ]; then
  RAWDESC=$(git describe --abbrev=0 2>/dev/null)
  if [ "$(git rev-parse HEAD)" = "$(git rev-list -1 $RAWDESC 2>/dev/null)" ]; then
    git diff-index --quiet HEAD -- && GIT_TAG=$RAWDESC
  fi
fi

if [ -n "$GIT_TAG" ]; then
  VERSION=$GIT_TAG
fi

echo $VERSION
