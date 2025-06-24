#!/bin/sh

STYLE="{IndentWidth: 4, ColumnLimit: 200}"

# Format .cpp, .h files in specific directories
for dir in mythapps mythpluginLoader mythapps/plugins; do
    find "$dir" -maxdepth 1 -type f \( -name "*.cpp" -o -name "*.h" \) -exec clang-format -i -style="$STYLE" {} +
done

# Format .pl files
for file in mythapps/*.pl; do
    [ -e "$file" ] && clang-format -i "$file" -style="$STYLE"
done

# Format specific XML files
for xml in mythapps-input mythapps-settings mythapps-ui; do
    file="themes/default/${xml}.xml"
    [ -e "$file" ] && xmlindent -l 10000 -nba -t -w "$file"
done
