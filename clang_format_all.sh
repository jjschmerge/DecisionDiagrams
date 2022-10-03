#!/bin/bash

for file in $(find libteddy tests -type f -name "*.cpp" -or -name "*.hpp"); do
    clang-format -style=file --dry-run $file 2>&1 > /dev/null
    if [ $? -eq 1 ]; then
        echo "Error for: $file"
    else
        clang-format -style=file --dry-run -Werror $file 2> /dev/null > /dev/null
        if [ $? -eq 1 ]; then
            echo "Formatting $file"
            clang-format -i -style=file $file
        fi
    fi
done
