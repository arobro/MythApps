#!/bin/bash
#Purpose: Forks the default kodi skin with a mythbuntu seek bar.

# Define folder and file names
SOURCE_DIR="skin.estuary"
TARGET_DIR="skin.mythapps_mythbuntu"
XML_FILES=("VideoOSD.xml" "DialogSeekBar.xml")
DEST_SUBDIR="xml"
ADDON_FILE="addon.xml"
ZIP_FILE="${TARGET_DIR}.zip"

# Step 1: Rename the folder if necessary
if [ -d "$SOURCE_DIR" ]; then
    if [ -d "$TARGET_DIR" ]; then
        echo "Target directory '$TARGET_DIR' already exists. Rename aborted."
    else
        mv "$SOURCE_DIR" "$TARGET_DIR"
        echo "Renamed '$SOURCE_DIR' to '$TARGET_DIR'."
    fi
else
    echo "Source directory '$SOURCE_DIR' does not exist. Skipping rename."
fi

# Step 2: Copy XML files to the xml/ subdirectory inside the target folder
for XML_FILE in "${XML_FILES[@]}"; do
    if [ -f "$XML_FILE" ]; then
        if [ -d "$TARGET_DIR/$DEST_SUBDIR" ]; then
            cp "$XML_FILE" "$TARGET_DIR/$DEST_SUBDIR/"
            echo "Copied '$XML_FILE' to '$TARGET_DIR/$DEST_SUBDIR/'."
        else
            echo "Destination subdirectory '$TARGET_DIR/$DEST_SUBDIR' does not exist. Creating it..."
            mkdir -p "$TARGET_DIR/$DEST_SUBDIR"
            cp "$XML_FILE" "$TARGET_DIR/$DEST_SUBDIR/"
            echo "Copied '$XML_FILE' to newly created '$TARGET_DIR/$DEST_SUBDIR/'."
        fi
    else
        echo "File '$XML_FILE' not found in current directory. Copy aborted."
    fi
done

# Step 3: Modify the second line of addon.xml inside the target folder
ADDON_PATH="$TARGET_DIR/$ADDON_FILE"
if [ -f "$ADDON_PATH" ]; then
    echo "Modifying second line of '$ADDON_PATH'..."
    awk 'NR==2 {
        gsub("skin.estuary", "skin.mythapps_mythbuntu");
        gsub("Estuary", "MythApps -Mythbuntu");
    } { print }' "$ADDON_PATH" > "${ADDON_PATH}.tmp" && mv "${ADDON_PATH}.tmp" "$ADDON_PATH"
    echo "Second line updated."
else
    echo "File '$ADDON_PATH' not found. Skipping modification."
fi

# Step 4: Zip the entire target folder
if [ -d "$TARGET_DIR" ]; then
    echo "Creating zip archive '$ZIP_FILE'..."
    zip -r "$ZIP_FILE" "$TARGET_DIR" > /dev/null
    echo "Archive created successfully."
else
    echo "Target directory '$TARGET_DIR' not found. Zip aborted."
fi

