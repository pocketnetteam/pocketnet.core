#!/bin/sh

# Install libdb4.8 (Berkeley DB).

export LC_ALL=C
set -e

if [ -z "${1}" ]; then
  echo "Usage: ./install_libdatachannel.sh <base-dir>"
  echo
  echo "Must specify a single argument: the directory in which libdatachannel will be built."
  echo "This is probably \`pwd\` if you're at the root of the pocketcoin repository."
  exit 1
fi

expand_path() {
  echo "$(cd "${1}" && pwd -P)"
}

DATACHANNEL_PREFIX="$(expand_path ${1})/libdatachannel"; shift;
DATACHANNEL_VERSION='0.17.7'
DATACHANNEL_HASH='3c8de85fde74c375fc9acc5e5614567542497d7607d5574ec9ee3cab97587d3f'
DATACHANNEL_FILENAME="libdatachannel-${DATACHANNEL_VERSION}-Source"
DATACHANNEL_URL="https://github.com/lostystyg/libdatachannel/releases/download/v${DATACHANNEL_VERSION}/${DATACHANNEL_FILENAME}.tar.gz"
echo "${DATACHANNEL_URL}"

# https://github.com/lostystyg/libdatachannel/releases/download/v0.17.7/libdatachannel-0.17.7-Source.tar.gz
# https://github.com/lostystyg/libdatachannel/releases/download/v0.17.7/libdatachannel-0.17.7-Source.tar.gz
check_exists() {
  which "$1" >/dev/null 2>&1
}

sha256_check() {
  # Args: <sha256_hash> <filename>
  #
  if check_exists sha256sum; then
    echo "${1}  ${2}" | sha256sum -c
  elif check_exists sha256; then
    if [ "$(uname)" = "FreeBSD" ]; then
      sha256 -c "${1}" "${2}"
    else
      echo "${1}  ${2}" | sha256 -c
    fi
  else
    echo "${1}  ${2}" | shasum -a 256 -c
  fi
}

http_get() {
  # Args: <url> <filename> <sha256_hash>
  #
  # It's acceptable that we don't require SSL here because we manually verify
  # content hashes below.
  #
  if [ -f "${2}" ]; then
    echo "File ${2} already exists; not downloading again"
  elif check_exists curl; then
    curl --insecure -L "${1}" -o "${2}"
  else
    wget --no-check-certificate "${1}" -O "${2}"
  fi

  sha256_check "${3}" "${2}"
}

mkdir -p "${DATACHANNEL_PREFIX}"
http_get "${DATACHANNEL_URL}" "${DATACHANNEL_FILENAME}.tar.gz" "${DATACHANNEL_HASH}"
tar -xzvf ${DATACHANNEL_FILENAME}.tar.gz -C "$DATACHANNEL_PREFIX"


mkdir -p "${DATACHANNEL_PREFIX}/${DATACHANNEL_FILENAME}"
cd "${DATACHANNEL_PREFIX}/${DATACHANNEL_FILENAME}"

# TODO (losty): change url to mainrepo then pulled there
BUILD_STATIC_PATCH_URL="https://raw.githubusercontent.com/lostystyg/pocketnet.core/feature/nat_traversal/depends/patches/libdatachannel/build_static.patch"
BUILD_STATIC_PATCH_HASH="53cccfe59e9a8d9159b355c638e81f024db8b157afc2362de72fc2e61761179f"
http_get "${BUILD_STATIC_PATCH_URL}" build_static.patch "${BUILD_STATIC_PATCH_HASH}"
patch < build_static.patch

DATACHANNEL_BUILDDIR="${DATACHANNEL_PREFIX}/${DATACHANNEL_FILENAME}/build"
mkdir -p "${DATACHANNEL_BUILDDIR}"
cd "${DATACHANNEL_BUILDDIR}"

cmake -DNO_MEDIA=ON -DNO_EXAMPLES=ON -DNO_TESTS=ON ..
cmake --build . -- -j14
cmake --install . --prefix "${DATACHANNEL_PREFIX}"

echo
echo "libdatachannel build complete."
echo
echo 'When compiling pocketcoind, run `./configure` in the following way:'
echo
echo "  export DATACHANNEL_PREFIX='${DATACHANNEL_PREFIX}'"
echo '  ./configure DATACHANNEL_LIBS="-L${DATACHANNEL_PREFIX}/lib -ldatachannel -ljuice -lusrsctp" DATACHANNEL_CPPFLAGS="-I${DATACHANNEL_PREFIX}/include" ...'
