#! /bin/sh
$XGETTEXT `find . -name '*.cpp' -o -name '*.h'` -o $podir/libgrantleethemeeditor.pot
rm -f rc.cpp
