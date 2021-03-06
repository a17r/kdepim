#! /bin/sh
$EXTRACT_GRANTLEE_TEMPLATE_STRINGS `find -name \*.html` >> src/html.cpp
$EXTRACTRC src/*.kcfg >> rc.cpp || exit 11
$EXTRACTRC $(find . -name "*.ui" -o -name "*.rc") >> rc.cpp || exit 12
$XGETTEXT rc.cpp src/*.cpp -o $podir/kontact.pot
rm -f rc.cpp src/html.cpp ./grantlee-extractor-pot-scripts/grantlee_strings_extractor.pyc
