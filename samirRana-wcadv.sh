#!/bin/sh

# command:  wcadv file1 file2 file3.. 

# check if at least one file is provided
if [ $# -eq 0 ]; then
    echo "Usage: wcadv <filename(s)>"
    exit 1
fi

# looping through all file provided
for file in "$@"
do
    # check if file exists & readable
    if [ ! -f "$file" ]; then
        echo "wcadv: $file: No such file eixts"
        continue
    fi

    # count lines
    lines=$(wc -l < "$file")

    # count words
    words=$(wc -w < "$file")

    # count characters
    chars=$(wc -m < "$file")

    # advanced feature is it can calculate length of longest line in file
    longest=$(awk '{ if (length > max) max = length } END { print max }' "$file")

    # print infos of the fils 
    echo "$lines $words $chars $longest $file"

done
