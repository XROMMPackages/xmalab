#!/bin/bash

# --- Configuration ---
APP_BUNDLE_PATH="./XMALab.app"
EXECUTABLE_NAME="XMALab"
FRAMEWORKS_DIR="$APP_BUNDLE_PATH/Contents/Frameworks"
SYSTEM_SEARCH_PATHS=(
  "/Users/peterfalkingham/Qt/6.9.1/macos"
  "$(brew --prefix qt6)"
  "/opt/homebrew"
  "/usr/local"
)
EXCLUDE_PATHS=("/usr/lib/" "/System/Library/")
LOG_FILE="bundle_log.txt"

# --- Helpers ---
log() { echo "$@" | tee -a "$LOG_FILE"; }

is_system_lib() {
  for exclude in "${EXCLUDE_PATHS[@]}"; do
    [[ "$1" == "$exclude"* ]] && return 0
  done
  return 1
}

resolve_dependency() {
  local dep_basename="$1"
  for search_path in "${SYSTEM_SEARCH_PATHS[@]}"; do
    found=$(find "$search_path" -type f -name "$dep_basename" 2>/dev/null | head -n 1)
    [[ -n "$found" ]] && echo "$found" && return 0
  done
  return 1
}

add_rpath_if_missing() {
  local binary="$1"
  rpath="@executable_path/../Frameworks"
  if ! otool -l "$binary" | grep -q "$rpath"; then
    install_name_tool -add_rpath "$rpath" "$binary"
    log "Added RPATH to $binary"
  fi
}

bundle_dependency() {
  local dep="$1"
  local target="$FRAMEWORKS_DIR/$(basename "$dep")"
  if [[ ! -f "$target" ]]; then
    cp "$dep" "$target"
    chmod +w "$target"
    install_name_tool -id "@rpath/$(basename "$dep")" "$target"
    codesign --force --sign - "$target"
    log "Bundled $(basename "$dep")"
  fi
}

fix_dependency_reference() {
  local binary="$1"
  local original="$2"
  local replacement="@rpath/$(basename "$original")"
  install_name_tool -change "$original" "$replacement" "$binary"
  log "Relinked $binary: $original → $replacement"
}

process_binary() {
  local binary="$1"
  log "Processing $binary"
  add_rpath_if_missing "$binary"
  deps=$(otool -L "$binary" | awk 'NR>1 {print $1}' | grep -v "^@")
  for dep in $deps; do
    if is_system_lib "$dep"; then
      log "Skipping system lib: $dep"
      continue
    fi
    dep_basename=$(basename "$dep")
    local_path="$FRAMEWORKS_DIR/$dep_basename"
    if [[ -f "$local_path" ]]; then
      fix_dependency_reference "$binary" "$dep"
      continue
    fi
    resolved=$(resolve_dependency "$dep_basename")
    if [[ -n "$resolved" ]]; then
      bundle_dependency "$resolved"
      fix_dependency_reference "$binary" "$dep"
    else
      log "❌ Missing dependency: $dep_basename (not found)"
    fi
  done
}

# --- Main ---
log "🧩 Starting bundling process..."
mkdir -p "$FRAMEWORKS_DIR"

# Find all Mach-O binaries
binaries=$(find "$APP_BUNDLE_PATH" -type f | while read -r f; do
  file "$f" | grep -q "Mach-O" && echo "$f"
done)

for bin in $binaries; do
  process_binary "$bin"
done

log "✅ Bundling complete."

