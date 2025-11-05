#!/bin/bash

# --- Configuration ---
APP_BUNDLE_PATH="./XMALab.app"
EXECUTABLE_NAME="XMALab"
SYSTEM_SEARCH_PATHS=("/opt/homebrew" "/usr/local")
DMG_NAME="XMALab2.1.3p_MacOS15.4_workaround.dmg" # Incremented version
VOLUME_NAME="XMALab 2.1.3p"
# --- End Configuration ---

# --- Safety Checks & Setup ---
set -e
echo "Using Bash Version: $(bash --version)"
if [ ! -d "$APP_BUNDLE_PATH" ]; then echo "Error: App bundle not found at '$APP_BUNDLE_PATH'"; exit 1; fi
MACOS_DIR="$APP_BUNDLE_PATH/Contents/MacOS"
FRAMEWORKS_DIR="$APP_BUNDLE_PATH/Contents/Frameworks"
EXECUTABLE_PATH="$MACOS_DIR/$EXECUTABLE_NAME"
if [ ! -f "$EXECUTABLE_PATH" ]; then echo "Error: Executable not found at '$EXECUTABLE_PATH'"; exit 1; fi
mkdir -p "$FRAMEWORKS_DIR"

# --- Helper: Find system library (Handle Symlinks) ---
find_system_dependency() {
    local dep_name="$1"; local found_path=""
    # echo "[DEBUG FIND_SYS] Searching for '$dep_name' (allowing symlinks)..." # Optional debug

    # 1. Try searching standard search paths first
    for search_dir in "${SYSTEM_SEARCH_PATHS[@]}"; do
        found_path=$(find "$search_dir" -name "$dep_name" \( -type f -o -type l \) -print -quit 2>/dev/null)
        if [ -n "$found_path" ]; then
            # echo "[DEBUG FIND_SYS] Found '$dep_name' in standard path: $found_path"
            echo "$found_path"
            return 0
        fi
    done

    # 2. If not found, try searching Cellar path (heuristic)
    local formula_name=""
    if [[ "$dep_name" == libvtk* ]]; then formula_name="vtk"; elif [[ "$dep_name" == libopencv* ]]; then formula_name="opencv"; elif [[ "$dep_name" == libjpeg* || "$dep_name" == libjxl* ]]; then formula_name="jpeg-xl"; elif [[ "$dep_name" == libwebp* || "$dep_name" == libsharpyuv* ]]; then formula_name="webp"; elif [[ "$dep_name" == libprotobuf* || "$dep_name" == libutf8* ]]; then formula_name="protobuf"; elif [[ "$dep_name" == libgfortran* || "$dep_name" == libgcc* || "$dep_name" == libquadmath* ]]; then formula_name="gcc"; fi

    if [ -n "$formula_name" ]; then
        # echo "[DEBUG FIND_SYS] Attempting to search Cellar for formula '$formula_name'..."
        local formula_prefix=$(brew --prefix "$formula_name" 2>/dev/null)
        local exit_status=$?
        if [ $exit_status -eq 0 ] && [ -n "$formula_prefix" ] && [ -d "$formula_prefix/lib" ]; then
            # echo "[DEBUG FIND_SYS] Searching in '$formula_prefix/lib'..."
            found_path=$(find "$formula_prefix/lib" -name "$dep_name" \( -type f -o -type l \) -print -quit 2>/dev/null)
            if [ -n "$found_path" ]; then
                # echo "[DEBUG FIND_SYS] Found '$dep_name' in formula prefix: $found_path"
                echo "$found_path"
                return 0
            # else echo "[DEBUG FIND_SYS] Did not find '$dep_name' in '$formula_prefix/lib'.";
            fi
        # else echo "[DEBUG FIND_SYS] Could not get prefix or lib dir for '$formula_name' (Exit: $exit_status Prefix: '$formula_prefix').";
        fi
    # else echo "[DEBUG FIND_SYS] Could not guess formula name for '$dep_name'.";
    fi

    # 3. If still not found, return error
    # echo "[DEBUG FIND_SYS] Failed to find source for '$dep_name'."
    return 1
}

echo "--- Iterative Dependency Bundling v5 ---"

iteration=0
max_iterations=20

while true; do
    iteration=$((iteration + 1))
    echo ""; echo "--- Iteration $iteration ---"
    if [ "$iteration" -gt "$max_iterations" ]; then echo "[ERROR] Max iterations ($max_iterations). Aborting."; exit 1; fi

    # Find all Mach-O files and ensure they are executable
    binaries=()
    # echo "[DEBUG FIND] Starting find command..." # Verbose debug
    while IFS= read -r -d $'\0' file_path; do
        is_macho=0; file_output="N/A"
        if [ ! -e "$file_path" ]; then continue; fi
        if [ -r "$file_path" ]; then
             file_output=$(file "$file_path"); if echo "$file_output" | grep -q 'Mach-O'; then is_macho=1; fi
        else echo "[WARN] Cannot read: '$file_path'"; continue; fi

        if [[ $is_macho -eq 1 ]]; then
             # if [[ "$file_path" == *libgfortran* ]]; then echo "[DEBUG FIND] Candidate: '$file_path'"; fi # Verbose debug
             if [ ! -x "$file_path" ]; then
                 # echo "[DEBUG FIND] Adding execute permission to '$file_path'" # Verbose debug
                 chmod u+x "$file_path"; if [ $? -ne 0 ]; then echo "[ERROR] FAILED chmod '$file_path'. Skipping."; continue; fi
             # else if [[ "$file_path" == *libgfortran* ]]; then echo "[DEBUG FIND] Already executable."; fi # Verbose debug
             fi
             # if [[ "$file_path" == *libgfortran* ]]; then echo "[DEBUG FIND] >>> Adding to binaries array <<<"; fi # Verbose debug
             binaries+=("$file_path")
        fi
    done < <(find "$APP_BUNDLE_PATH" \( -path '*/Headers' -prune \) -o \( -type f -o -type l \) -print0) # Process Substitution
    echo "Found ${#binaries[@]} Mach-O binaries to scan."


    if [ ${#binaries[@]} -eq 0 ]; then echo "[WARN] No Mach-O binaries found."; break; fi

    # Declare Task Arrays
    declare -a tasks_copy_external=()
    declare -a tasks_copy_rpath_target=()
    declare -a tasks_fix_reference=()

    work_found_this_iteration=0

    echo "Scanning ${#binaries[@]} binaries..."
    for bin_path_orig in "${binaries[@]}"; do
        current_bin_path="$bin_path_orig"; bin_path="$bin_path_orig"
        if [ -L "$bin_path" ]; then
             real_bin_path=$(readlink "$bin_path"); if [[ "$real_bin_path" != /* ]]; then real_bin_path="$(dirname "$bin_path")/$real_bin_path"; fi
             if [ ! -e "$real_bin_path" ]; then echo "[WARN] Skipping broken symlink $current_bin_path"; continue; fi
             bin_path="$real_bin_path"
        fi
        if [ ! -r "$bin_path" ]; then echo "[WARN] Cannot read '$bin_path', skipping otool."; continue; fi

        while IFS= read -r line; do
            dep_path=$(echo "$line" | awk '{print $1}'); if [ -z "$dep_path" ]; then continue; fi
            dep_name=$(basename "$dep_path"); if [ -z "$dep_name" ]; then continue; fi

            # --- Initial Skip Logic ---
            if [[ "$dep_path" == /System/* ]] || [[ "$dep_path" == /usr/lib/* ]]; then continue; fi
            self_id=$(otool -D "$bin_path" 2>/dev/null)
            if [[ -n "$self_id" && "$self_id" == *"$dep_path"* && "$line" == *"(compatibility version"* ]]; then continue; fi

            # --- Classify Dependency ---
            bundled_dep_path="$FRAMEWORKS_DIR/$dep_name"

            if [[ "$dep_path" == "@rpath/"* ]]; then
                if [ ! -f "$bundled_dep_path" ] && [ ! -L "$bundled_dep_path" ]; then
                     echo "Found @rpath reference to MISSING target: '$dep_name' in '$(basename "$current_bin_path")'"
                     is_already_tasked=0; for tasked_name in "${tasks_copy_rpath_target[@]}"; do [[ "$tasked_name" == "$dep_name" ]] && is_already_tasked=1 && break; done
                     if [[ $is_already_tasked -eq 0 ]]; then tasks_copy_rpath_target+=("$dep_name"); fi
                     work_found_this_iteration=1
                fi
            elif [[ "$dep_path" == "@executable_path/"* ]] || [[ "$dep_path" == "@loader_path/"* ]] || [[ "$dep_path" =~ ^"$APP_BUNDLE_PATH"/.* ]]; then :
            else # EXTERNAL
                 echo "Found external reference: '$dep_path' in '$(basename "$current_bin_path")'"
                 is_already_tasked=0; for tasked_ext_path in "${tasks_copy_external[@]}"; do [[ "$tasked_ext_path" == "$dep_path" ]] && is_already_tasked=1 && break; done
                 if [[ $is_already_tasked -eq 0 ]]; then tasks_copy_external+=("$dep_path"); fi
                 tasks_fix_reference+=("$bin_path|$dep_path|$dep_name")
                 work_found_this_iteration=1
            fi
        done < <(otool -L "$bin_path" | tail -n +2) # Process Substitution
    done # End loop through binaries


    # --- Process Findings for This Iteration ---
    if [ $work_found_this_iteration -eq 0 ]; then echo "No new bundling tasks found. Final check or finish."; break; fi

    # --- Task 1: Copy libraries for missing @rpath targets ---
    echo "Processing ${#tasks_copy_rpath_target[@]} missing @rpath targets..."
    processed_rpath_targets=()
    set +e # Temporarily disable exit on error
    for dep_name in "${tasks_copy_rpath_target[@]}"; do # dep_name is the name needed inside the bundle (e.g., libvtkCommonSystem-9.4.1.dylib)
        is_processed=0; for processed in "${processed_rpath_targets[@]}"; do [[ "$processed" == "$dep_name" ]] && is_processed=1 && break; done; if [[ $is_processed -eq 1 ]]; then continue; fi

        destination_path="$FRAMEWORKS_DIR/$dep_name" # This is the path/name we need inside the bundle
        if [ -e "$destination_path" ]; then echo " -> Target '$dep_name' now exists. Skipping copy."; processed_rpath_targets+=("$dep_name"); continue; fi

        echo "[TASK] Handling missing target: $dep_name"
        # Find the source path (could be a file or a link)
        system_source_path_or_link=$(find_system_dependency "$dep_name")
        find_exit_status=$?

        if [ $find_exit_status -ne 0 ] || [ -z "$system_source_path_or_link" ]; then
             echo "[ERROR] Cannot find source for '$dep_name' (find exit status: $find_exit_status). Skipping."
             processed_rpath_targets+=("$dep_name"); continue
        fi

        # <<< CHANGE: Resolve symlink and copy target >>>
        real_source_path="$system_source_path_or_link"
        if [ -L "$system_source_path_or_link" ]; then
            echo " -> Source '$system_source_path_or_link' is a symlink."
            real_source_path=$(readlink "$system_source_path_or_link")
            # Handle relative symlinks if necessary
            if [[ "$real_source_path" != /* ]]; then
                real_source_path="$(dirname "$system_source_path_or_link")/$real_source_path"
            fi
            echo " -> Resolved symlink target: '$real_source_path'"
        fi

        # Check if the resolved source actually exists
        if [ ! -e "$real_source_path" ]; then
            echo "[ERROR] Resolved source file '$real_source_path' does not exist! Skipping."
            processed_rpath_targets+=("$dep_name"); continue
        fi

        # Copy the real file to the destination path (which has the dependency name)
        echo " -> Copying content of '$real_source_path' to '$destination_path'"
        cp "$real_source_path" "$destination_path"
        if [ $? -ne 0 ]; then echo "[ERROR] Copy failed for $dep_name"; processed_rpath_targets+=("$dep_name"); continue; fi

        # Set permissions and ID on the *newly created file*
        chmod u+w,a+rx "$destination_path"
        echo " -> Setting ID of '$destination_path' to '@rpath/$dep_name'"
        install_name_tool -id "@rpath/$dep_name" "$destination_path"
        if [ $? -ne 0 ]; then echo "[ERROR] Failed to set ID for $dep_name"; fi

        processed_rpath_targets+=("$dep_name")
    done
    set -e # Re-enable exit on error

    # --- Task 2: Copy libraries for external references ---
     echo "Processing ${#tasks_copy_external[@]} external libraries..."
     processed_external_libs=()
     set +e # Temporarily disable exit on error
     for ext_dep_path in "${tasks_copy_external[@]}"; do # ext_dep_path is the full /opt/... path
        is_processed=0; for processed in "${processed_external_libs[@]}"; do [[ "$processed" == "$ext_dep_path" ]] && is_processed=1 && break; done; if [[ $is_processed -eq 1 ]]; then continue; fi

        ext_dep_name=$(basename "$ext_dep_path")
        destination_path="$FRAMEWORKS_DIR/$ext_dep_name" # This is the path/name we need inside the bundle

        if [ -e "$destination_path" ]; then echo " -> External lib '$ext_dep_name' already exists now. Skipping copy."; processed_external_libs+=("$ext_dep_path"); continue; fi

        echo "[TASK] Handling external lib: $ext_dep_name (from $ext_dep_path)"

        # <<< CHANGE: Resolve symlink and copy target >>>
        real_source_path="$ext_dep_path"
        if [ -L "$ext_dep_path" ]; then
            echo " -> Source '$ext_dep_path' is a symlink."
            real_source_path=$(readlink "$ext_dep_path")
            if [[ "$real_source_path" != /* ]]; then real_source_path="$(dirname "$ext_dep_path")/$real_source_path"; fi
             echo " -> Resolved symlink target: '$real_source_path'"
        fi

        if [ ! -e "$real_source_path" ]; then echo "[ERROR] External source '$real_source_path' not found. Skipping."; processed_external_libs+=("$ext_dep_path"); continue; fi

        echo " -> Copying content of '$real_source_path' to '$destination_path'"
        cp "$real_source_path" "$destination_path"
        if [ $? -ne 0 ]; then echo "[ERROR] Copy failed for $ext_dep_name"; processed_external_libs+=("$ext_dep_path"); continue; fi

        chmod u+w,a+rx "$destination_path"
        echo " -> Setting ID of '$destination_path' to '@rpath/$ext_dep_name'"
        install_name_tool -id "@rpath/$ext_dep_name" "$destination_path"
        if [ $? -ne 0 ]; then echo "[ERROR] Failed to set ID for $ext_dep_name"; fi

        processed_external_libs+=("$ext_dep_path")
    done
    set -e # Re-enable exit on error

    # --- Task 3: Fix references pointing to external paths ---
    echo "Processing ${#tasks_fix_reference[@]} references to external paths..."
    declare -a fixed_refs_keys=(); declare -a fixed_refs_values=()
    set +e # Temporarily disable exit on error
    for task in "${tasks_fix_reference[@]}"; do
        IFS='|' read -r referrer_path ext_dep_path dep_name <<< "$task"
        fix_key="$referrer_path|$dep_name"
        is_processed=0; for processed_key in "${fixed_refs_keys[@]}"; do [[ "$processed_key" == "$fix_key" ]] && is_processed=1 && break; done; if [[ $is_processed -eq 1 ]]; then continue; fi

        current_ref=$(otool -L "$referrer_path" | grep "$dep_name" | awk '{print $1}' | head -n 1)
        if [[ "$current_ref" == "$ext_dep_path" ]]; then
             echo " -> Fixing '$dep_name' ref in '$(basename "$referrer_path")'"
             chmod u+w "$referrer_path"
             install_name_tool -change "$ext_dep_path" "@rpath/$dep_name" "$referrer_path"
             if [ $? -ne 0 ]; then echo "[ERROR] Failed to change ref for $dep_name in $(basename "$referrer_path")"; fi
             fixed_refs_keys+=("$fix_key")
        # else # Optional Debug
        #    echo " -> Ref for '$dep_name' in '$(basename "$referrer_path")' already changed? (Current: '$current_ref')"
        fi
    done
    set -e # Re-enable exit on error

done # End main while loop

# --- Final Steps ---
echo "Final Check: Ensuring all Frameworks libs have @rpath ID..."
# <<< CHANGE: Use Process Substitution in final check too >>>
while IFS= read -r -d $'\0' file; do
     target_file="$file"
     if [ -L "$file" ]; then real_file=$(readlink "$file"); if [[ "$real_file" != /* ]]; then real_file="$(dirname "$file")/$real_file"; fi; if [ ! -e "$real_file" ]; then continue; fi; target_file="$real_file"; fi
     if [ ! -f "$target_file" ]; then continue; fi

     if file "$target_file" | grep -q 'Mach-O'; then
        dep_name=$(basename "$file"); current_id=$(otool -D "$target_file" 2>/dev/null | tail -n 1 | awk '{print $1}')
        expected_id1="@rpath/$dep_name"; expected_id2=""
        if [[ "$dep_name" != *.dylib ]] && [[ -d "$FRAMEWORKS_DIR/${dep_name}.framework" ]]; then expected_id2="@rpath/${dep_name}.framework/Versions/5/${dep_name}"; fi
        if [[ "$current_id" != "$expected_id1" ]] && [[ "$current_id" != "$expected_id2" ]]; then
             echo " -> Fixing ID for final check: $dep_name (Current: $current_id -> $expected_id1)"
             chmod u+w "$target_file"
             install_name_tool -id "$expected_id1" "$target_file"; if [ $? -ne 0 ]; then echo "[ERROR] Failed final ID fix for $dep_name"; fi
        fi
     fi
done < <(find "$FRAMEWORKS_DIR" -maxdepth 1 \( -type f -o -type l \) -print0)


echo "Ensuring RPATH for executable '$EXECUTABLE_NAME'..."
RPATH_NEEDED="@executable_path/../Frameworks"
if ! otool -l "$EXECUTABLE_PATH" | grep -A2 LC_RPATH | grep -Fq -- "path $RPATH_NEEDED "; then
    echo "Adding RPATH '$RPATH_NEEDED'..."
    chmod u+w "$EXECUTABLE_PATH"; install_name_tool -add_rpath "$RPATH_NEEDED" "$EXECUTABLE_PATH"; if [ $? -ne 0 ]; then echo "[ERROR] add_rpath failed!"; exit 1; fi
else echo "Executable RPATH already correct."; fi
echo "Dependency bundling iterations complete."

# --- Code Signing & DMG Creation ---
echo "Performing ad-hoc code signing..."
find "$APP_BUNDLE_PATH" \( -type f -or -type l \) -print0 | xargs -0 -I {} chmod u+w {} || echo "[WARN] chmod u+w failed pre-sign"
codesign --deep --force --sign - "$APP_BUNDLE_PATH"
echo "Code signing complete."

read -p "Do you want to create a DMG now? (y/N) " -n 1 -r; echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Creating DMG disk image..."
    TEMP_DMG_DIR=$(mktemp -d -t dmg-build-XXXXXX)
    rsync -aE "$APP_BUNDLE_PATH/" "$TEMP_DMG_DIR/${APP_BUNDLE_PATH##*/}/"
    ln -s /Applications "$TEMP_DMG_DIR/Applications"
    SIZE_KB=$(du -sk "$TEMP_DMG_DIR" | cut -f1); PADDING_KB=$(( (SIZE_KB / 10) + 10240 )); FINAL_SIZE_KB=$(( SIZE_KB + PADDING_KB )); FINAL_SIZE_MB=$(( (FINAL_SIZE_KB + 1023) / 1024 ))
    echo "DMG size: ${FINAL_SIZE_MB}m"; FINAL_DMG_PATH="./${DMG_NAME}"
    rm -f "$FINAL_DMG_PATH"; hdiutil create -srcfolder "$TEMP_DMG_DIR" -volname "$VOLUME_NAME" -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDBZ -size ${FINAL_SIZE_MB}m "$FINAL_DMG_PATH"
    echo "DMG creation complete: $FINAL_DMG_PATH"; rm -rf "$TEMP_DMG_DIR"; echo "Cleanup complete."
fi

echo "-------------------------------------------------------"; echo " Iterative bundling process v5 finished!"; echo "-------------------------------------------------------"
exit 0