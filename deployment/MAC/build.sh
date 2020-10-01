BASEDIR=${PWD}
SCRIPTDIR=$(dirname $0)
BUILDDIR=$SCRIPTDIR/../../build
QT_DIR=$1
MACDEPLOYQTFIXDIR=$2

echo $BUILDDIR

cd $BUILDDIR
BUILDDIR=${PWD}
rm -r -f bin/
make
$QT_DIR/bin/macdeployqt bin/XMALab.app
python $MACDEPLOYQTFIXDIR/macdeployqtfix.py -nl $BUILDDIR/bin/XMALab.app/Contents/MacOS/XMALab /Users/ben/compiled_libraries/qt5/install 
cd $BASEDIR
cd $SCRIPTDIR
rm -r -f ../../build/bin/*.dmg
cp -r -f ../../build/bin/XMALab.app .
cp -r -f xmalab_template.json xmalab.json
sed -i '' 's/VERSION/'$3'/g'  xmalab.json
appdmg xmalab.json ../../build/bin/XMALab_$3.dmg
cd $BASEDIR
