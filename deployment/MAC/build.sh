BASEDIR=${PWD}
SCRIPTDIR=$(dirname $0)
cd $SCRIPTDIR/../../build
rm -r -f bin/
make
macdeployqt bin/XMALab.app/ -dmg
cd $BASEDIR