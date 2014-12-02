cd build
rm -r -f bin/
make
macdeployqt bin/XMALab.app/ -dmg
cd ..