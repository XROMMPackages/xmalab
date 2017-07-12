BASEDIR=${PWD}
SCRIPTDIR=$(dirname $0)
cd $SCRIPTDIR
rm -r -f ../../build/bin/*.dmg
cp -r -f ../../build/bin/XMALab.app .
cp -r -f xmalab_template.json xmalab.json
sed -i '' 's/VERSION/'$1'/g'  xmalab.json
appdmg xmalab.json ../../build/bin/XMALab_$1.dmg
cd $BASEDIR