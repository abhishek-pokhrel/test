#!/bin/sh
#counts lines, words, characters & the longest line length (new advanced feature made in)

file="$1"

if [ ! -f "$file" ]; then
  echo "File not found!"
  exit 1
fi

lines=$(wc -l < "$file")
words=$(wc -w < "$file")
chars=$(wc -c < "$file")
longest=$(awk '{ if (length > max) max = length } END { print max+0 }' "$file")

echo "line word chars longline name"
echo "$lines $words $chars $longest $file"