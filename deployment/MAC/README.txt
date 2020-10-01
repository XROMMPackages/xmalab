The scripts assume that xmalab was built in the xmalab/build/bin folder.

To build the app and dmg just run the build.sh script. For input it requires the folder in which the macdeployqtfix resides (https://github.com/arl/macdeployqtfix), qtbuild folder and the version number, e.g

sh deployment/MAC/build.sh /Users/ben/compiled_libraries/qt5/install /Users/ben/compiled_libraries/macdeployqtfix 2.0.0

The final dmg can be found in xmalab/build/bin
