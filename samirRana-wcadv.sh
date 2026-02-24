#!/bin/sh
# This script counts lines, words, characters, and sentences (advanced feature added new one)

file="$1"

# check if file name exists
if [ ! -f "$file" ]; then
  echo "Error: File not found!"
  exit 1
fi

# atandard wc counts
lines=$(wc -l < "$file")
words=$(wc -w < "$file")
chars=$(wc -c < "$file")

# new advanced  feature: count sentences (., !, ?)
sentences=$(grep -o '[.!?]' "$file" | wc -l)

# display
echo "Lines Words Characters Sentences FileName"
echo "$lines $words $chars $sentences $file"
