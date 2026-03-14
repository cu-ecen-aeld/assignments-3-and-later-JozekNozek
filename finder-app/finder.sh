#!/bin/sh

if [ -z "$1" ]; then
    echo "Directory PATH missing!"
    exit 1
fi

filesdir="$1"

if [ -z "$2" ]; then
    echo "Search string missing!"
    exit 1
fi

searchstr="$2"

filecount=$(find "$filesdir" -type f | wc -l)
matchcount=$(grep -row "$searchstr" "$filesdir" 2>/dev/null | wc -l)

echo "The number of files are $filecount and the number of matching lines are $matchcount"
exit 0