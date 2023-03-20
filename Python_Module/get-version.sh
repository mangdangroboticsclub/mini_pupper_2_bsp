#!/usr/bin/bash
TAG_COMMIT=$(git rev-list --abbrev-commit --tags --max-count=1)
TAG=$(git describe --abbrev=0 --tags ${TAG_COMMIT} 2>/dev/null || true)
COMMIT=$(git rev-parse --short HEAD)
DATE=$(git log -1 --format=%cd --date=format:"%Y%m%d")
VERSION=$(echo ${TAG} | sed "s/^v//")
if [[ ! "${COMMIT}" = "${TAG_COMMIT}" ]];
then
  VERSION=$(echo ${VERSION}-next-${COMMIT}-${DATE})
fi
if [[ "${VERSION}" = "" ]];
then
 VERSION=${COMMIT}-${DATA}
fi
if [[ $(git status --porcelain|wc -l) > 0 ]];
then
  VERSION=${VERSION}-dirty
fi
echo -n ${VERSION::32}
