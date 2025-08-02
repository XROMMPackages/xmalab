#!/bin/bash

APP_PATH="$1"

if [[ -z "$APP_PATH" || ! -d "$APP_PATH" ]]; then
  echo "❌ Usage: $0 /path/to/YourApp.app"
  exit 1
fi

FRAMEWORKS_DIR="$APP_PATH/Contents/Frameworks"
BINARIES=()

echo "🔍 Scanning .app bundle: $APP_PATH"
echo "📁 Frameworks directory: $FRAMEWORKS_DIR"
echo ""

# Collect all binaries: main executable + plugins + dylibs
echo "📦 Collecting binaries..."
BINARIES+=("$APP_PATH/Contents/MacOS/$(basename "$APP_PATH" .app)")
BINARIES+=($(find "$APP_PATH/Contents/PlugIns" -type f -perm +111 2>/dev/null))
BINARIES+=($(find "$FRAMEWORKS_DIR" -type f -name "*.dylib" 2>/dev/null))

# Check symlinks
echo "🔗 Checking for broken symlinks in Frameworks..."
BROKEN_SYMLINKS=0
find "$FRAMEWORKS_DIR" -type l | while read -r symlink; do
  TARGET=$(readlink "$symlink")
  if [[ ! -e "$(dirname "$symlink")/$TARGET" ]]; then
    echo "❌ Broken symlink: $symlink → $TARGET"
    ((BROKEN_SYMLINKS++))
  fi
done
[[ $BROKEN_SYMLINKS -eq 0 ]] && echo "✅ No broken symlinks found."

# Check each binary's linked libraries
echo ""
echo "🔍 Checking linked libraries in binaries..."
UNREWRITTEN_PATHS=0
for bin in "${BINARIES[@]}"; do
  echo "📄 $bin"
  otool -L "$bin" | tail -n +2 | while read -r line; do
    lib=$(echo "$line" | awk '{print $1}')
    if [[ "$lib" == /opt/homebrew/* || "$lib" == /usr/local/* ]]; then
      echo "⚠️  External path: $lib"
      ((UNREWRITTEN_PATHS++))
    elif [[ "$lib" == @rpath/* ]]; then
      echo "⚠️  Unresolved @rpath: $lib"
      ((UNREWRITTEN_PATHS++))
    elif [[ "$lib" != @executable_path/* && "$lib" != @loader_path/* ]]; then
      echo "❓ Suspicious path: $lib"
    fi
  done
done

# Summary
echo ""
echo "📊 Audit Summary:"
echo "🔗 Broken symlinks: $BROKEN_SYMLINKS"
echo "⚠️  Unrewritten or external paths: $UNREWRITTEN_PATHS"
echo "✅ Total binaries checked: ${#BINARIES[@]}"

if [[ $BROKEN_SYMLINKS -eq 0 && $UNREWRITTEN_PATHS -eq 0 ]]; then
  echo "🎉 Your .app bundle looks clean and self-contained!"
else
  echo "🚨 Issues found — consider fixing symlinks and relinking unresolved paths."
fi

