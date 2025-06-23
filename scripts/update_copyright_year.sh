#!/bin/bash
# Update copyright year from 2024 to 2025

echo "Updating copyright year from 2024 to 2025..."

# Update in all source files
find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.asm" -o -name "*.s" -o -name "*.md" -o -name "*.tempo" -o -name "LICENSE" \) \
    -not -path "./build/*" \
    -not -path "./.git/*" \
    -exec grep -l "Copyright.*2024.*Ignacio" {} \; | while read file; do
    echo "Updating: $file"
    sed -i.bak 's/Copyright (c) 2024/Copyright (c) 2025/g' "$file"
    rm "${file}.bak"
done

# Also update the add_headers script
sed -i.bak 's/YEAR=\$(date +%Y)/YEAR=2025/g' scripts/add_headers.sh 2>/dev/null || true
rm scripts/add_headers.sh.bak 2>/dev/null || true

echo "Done! All copyright years updated to 2025."