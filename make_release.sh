#!/bin/sh

VERSION=$(cat version.h | perl -pe '/VERSION "([^"]+)"/; $_ = $1;')
svn export . gfract-${VERSION}
rm -rf gfract-${VERSION}/old-stuff
tar zcvf gfract-${VERSION}.tar.gz gfract-${VERSION}
rm -rf gfract-${VERSION}
mv gfract-${VERSION}.tar.gz ~/html/gfract/
cp CHANGES ~/html/gfract/
