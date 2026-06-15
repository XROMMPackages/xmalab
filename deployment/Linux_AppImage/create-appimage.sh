#!/usr/bin/env bash
# No set -e: linuxdeploy emits "ERROR:" on strip warnings (non-fatal),
# and a single timeout shouldn't abort the whole pipeline.

PROJECT_DIR="$(realpath "$(dirname "$0")/../..")"
BUILD_DIR="${PROJECT_DIR}/build/linux-system"
BINARY="${BUILD_DIR}/bin/Release/XMALab"
APPDIR="${PROJECT_DIR}/XMALab.AppDir"
DEPLOY_DIR="$(realpath "$(dirname "$0")")"

LINUXDEPLOY="/tmp/linuxdeploy-extracted/AppRun"
APPIMAGETOOL="/tmp/appimagetool-extracted/usr/bin/appimagetool"

OUTPUT="${PROJECT_DIR}/XMALab-x86_64.AppImage"

echo "=== XMALab AppImage Builder ==="
echo "Project: ${PROJECT_DIR}"
echo "Binary: ${BINARY}"
echo "AppDir: ${APPDIR}"
echo "Output: ${OUTPUT}"

if [ ! -f "${BINARY}" ]; then
    echo "ERROR: Binary not found at ${BINARY}"
    echo "Build the project first with: cmake --build --preset linux-system-release"
    exit 1
fi

echo ""
echo "=== Step 1: Create AppDir structure ==="
rm -rf "${APPDIR}"
mkdir -p "${APPDIR}/usr/bin"
mkdir -p "${APPDIR}/usr/lib"
mkdir -p "${APPDIR}/usr/lib64"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/256x256/apps"
mkdir -p "${APPDIR}/etc"

echo "=== Step 2: Copy binary, desktop file, icon, and AppRun ==="
cp "${BINARY}" "${APPDIR}/usr/bin/XMALab"
cp "${DEPLOY_DIR}/XMALab.desktop" "${APPDIR}/"
cp "${DEPLOY_DIR}/XMALab.png" "${APPDIR}/"
cp "${DEPLOY_DIR}/XMALab.png" "${APPDIR}/usr/share/icons/hicolor/256x256/apps/"
cp "${DEPLOY_DIR}/AppRun" "${APPDIR}/"
chmod +x "${APPDIR}/AppRun"

echo "=== Step 3: Bundle dependencies with linuxdeploy ==="
LINUXDEPLOY_PLUGIN_CONFIG_TYPE=qt6 \
    "${LINUXDEPLOY}" \
    --appdir "${APPDIR}" \
    --executable "${APPDIR}/usr/bin/XMALab" \
    --desktop-file "${APPDIR}/XMALab.desktop" \
    --icon-file "${APPDIR}/XMALab.png" \
    --plugin qt \
    --exclude-library="*samba*" \
    --exclude-library="*libnss_*" \
    --exclude-library="*libpam*" \
    --exclude-library="*libsss*" \
    --exclude-library="*libxkbcommon*" \
    -v1 2>&1

echo ""
echo "=== Step 4: Copy FlexiBLAS config and backends ==="
# Use netlib as default (most compatible)
echo "default = netlib" > "${APPDIR}/etc/flexiblasrc"
mkdir -p "${APPDIR}/usr/lib/flexiblas"
cp -r /usr/lib64/flexiblas/*.so* "${APPDIR}/usr/lib/flexiblas/" 2>/dev/null || true
mkdir -p "${APPDIR}/usr/lib64/flexiblas"
cp -r /usr/lib64/flexiblas/*.so* "${APPDIR}/usr/lib64/flexiblas/" 2>/dev/null || true

echo ""
echo "=== Step 5: Copy missing OpenBLAS OpenMP libraries ==="
cp -a /lib64/libopenblaso*.so* "${APPDIR}/usr/lib/" 2>/dev/null || true
cp -a /lib64/libopenblas64*.so* "${APPDIR}/usr/lib/" 2>/dev/null || true

echo ""
echo "=== Step 6: Copy Qt6 platform plugins ==="
QT_PLUGINS_SRC="/usr/lib64/qt6/plugins"
QT_PLUGINS_DST="${APPDIR}/usr/lib/qt6/plugins"

if [ -d "${QT_PLUGINS_SRC}" ]; then
    mkdir -p "${QT_PLUGINS_DST}"
    cp -r "${QT_PLUGINS_SRC}/platforms" "${QT_PLUGINS_DST}/"
    cp -r "${QT_PLUGINS_SRC}/platformthemes" "${QT_PLUGINS_DST}/" 2>/dev/null || true
    cp -r "${QT_PLUGINS_SRC}/imageformats" "${QT_PLUGINS_DST}/" 2>/dev/null || true
    cp -r "${QT_PLUGINS_SRC}/iconengines" "${QT_PLUGINS_DST}/" 2>/dev/null || true
    cp -r "${QT_PLUGINS_SRC}/xcbglintegrations" "${QT_PLUGINS_DST}/" 2>/dev/null || true
    cp -r "${QT_PLUGINS_SRC}/tls" "${QT_PLUGINS_DST}/" 2>/dev/null || true
    cp -r "${QT_PLUGINS_SRC}/generic" "${QT_PLUGINS_DST}/" 2>/dev/null || true
    echo "Qt6 plugins copied from ${QT_PLUGINS_SRC}"
else
    echo "WARNING: Qt6 plugins source not found at ${QT_PLUGINS_SRC}"
fi

echo ""
echo "=== Step 7: Copy XCB/X11 utility libraries for Qt xcb plugin ==="
for lib in \
    libxcb-cursor.so.0 libxcb-icccm.so.4 libxcb-image.so.0 \
    libxcb-keysyms.so.1 libxcb-randr.so.0 libxcb-render-util.so.0 \
    libxcb-shape.so.0 libxcb-sync.so.1 libxcb-xfixes.so.0 \
    libxcb-xkb.so.1 libxcb-util.so.1 libxkbcommon-x11.so.0 \
    libX11-xcb.so.1 libSM.so.6 libICE.so.6 libfontconfig.so.1 \
    libXext.so.6 libXcursor.so.1 libXfixes.so.3 libXrender.so.1 \
    libXau.so.6; do
    # Copy the SONAME symlink (preserving symlink chain)
    found=$(find /lib64 /usr/lib64 -name "$lib" 2>/dev/null | head -1)
    if [ -n "$found" ]; then
        cp -a "$found" "${APPDIR}/usr/lib/" 2>/dev/null || true
    fi
done
echo "XCB/X11 libraries copied"

echo ""
echo "=== Step 7b: Copy Qt internal libraries (Qt6XcbQpa, etc.) ==="
for lib in libQt6XcbQpa; do
    found=$(find /usr/lib64 -name "${lib}.so*" -not -name "*.prl" 2>/dev/null | head -1)
    if [ -n "$found" ]; then
        cp -a "$found" "${APPDIR}/usr/lib/" 2>/dev/null || true
        # Copy all symlinks for this lib
        basename="${lib%.so*}"
        for link in $(find /usr/lib64 -name "${lib}*" -not -name "*.prl" 2>/dev/null); do
            cp -a "$link" "${APPDIR}/usr/lib/" 2>/dev/null || true
        done
    fi
done
echo "Qt internal libraries copied"

echo ""
echo "=== Step 8: Aggressively remove Samba libraries ==="
find "${APPDIR}" -type f \( \
    -name "*samba*" -o -name "*smbclient*" -o -name "*libsmb*" -o \
    -name "*libndr*" -o -name "*libdcerpc*" -o -name "*libsamdb*" -o \
    -name "*libldb*" -o -name "*libtalloc*" -o -name "*libtdb*" -o \
    -name "*libtevent*" -o -name "*libwbclient*" -o -name "*libgensec*" -o \
    -name "*libkrb5samba*" -o -name "*libreplace*" -o -name "*libmsrpc3*" -o \
    -name "*libcli-*" -o -name "*libcliauth*" -o -name "*libnpa-tstream*" -o \
    -name "*libsecrets3*" -o -name "*libstable-sort*" -o -name "*libgenrand*" -o \
    -name "*libgse*" -o -name "*libdbwrap*" -o -name "*libsocket-blocking*" -o \
    -name "*libcommon-auth*" -o -name "*libsmbd-shim*" -o -name "*libsamba-cluster*" -o \
    -name "*libserver-id*" -o -name "*libtalloc-report*" -o -name "*libutil-reg*" -o \
    -name "*libinterfaces*" -o -name "*libCHARSET3*" -o -name "*libsys-rw*" -o \
    -name "*libmessages-*" -o -name "*libtdb-wrap*" -o -name "*libserver-role*" -o \
    -name "*libtime-basic*" -o -name "*libutil-setid*" -o -name "*libsmb-transport*" -o \
    -name "*libauthkrb5*" -o -name "*libldbsamba*" -o -name "*libsamdb-common*" -o \
    -name "*libndr-nbt*" -o -name "*libcli-cldap*" -o -name "*libaddns*" -o \
    -name "*libcli-nbt*" -o -name "*libiov-buf*" -o -name "*libndr-krb5pac*" -o \
    -name "*libasn1util*" -o -name "*libsamba-modules*" -o -name "*libMESSAGING*" -o \
    -name "*libmsghdr*" -o -name "*libflag-mapping*" -o -name "*libcli-ldap*" -o \
    -name "*libclidns*" -o -name "*libcluster*" -o -name "*libdcerpc-binding*" -o \
    -name "*libsamba-util*" -o -name "*libsamba-credentials*" -o -name "*libsamba-errors*" -o \
    -name "*libsamba-hostconfig*" -o -name "*libsamba-security*" \
\) -delete 2>/dev/null || true
rm -rf "${APPDIR}/usr/translations" 2>/dev/null || true
# Remove bulk data from share but keep .desktop and icons for desktop integration
rm -rf "${APPDIR}/usr/share/doc" "${APPDIR}/usr/share/man" "${APPDIR}/usr/share/info" \
       "${APPDIR}/usr/share/licenses" "${APPDIR}/usr/share/gtk-doc" \
       "${APPDIR}/usr/share/help" "${APPDIR}/usr/share/locale" 2>/dev/null || true
echo "Samba libraries and unused files removed"

echo ""
echo "=== Step 9: Create AppImage ==="
rm -f "${OUTPUT}"
export ARCH=x86_64
export PATH="/tmp/appimagetool-extracted/usr/bin:${PATH}"
"${APPIMAGETOOL}" "${APPDIR}" "${OUTPUT}" 2>&1

echo ""
echo "=== Done! ==="
echo "AppImage created: ${OUTPUT}"

if [ -f "${OUTPUT}" ]; then
    ls -lh "${OUTPUT}"
    file "${OUTPUT}"
fi
