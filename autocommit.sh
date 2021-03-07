#!/usr/bin/env bash
export PATH="/usr/local/bin:/usr/bin:/bin:/home/linuxbrew/.linuxbrew/bin:/usr/local/sbin:/usr/sbin:/sbin:/home/linuxbrew/.linuxbrew/sbin"

function autocommit() {
    local interval="${1:-60}"
    while [ 1 ]; do
        D="$(date)"
        L="$PWD/.git/autocommit.log"
        echo "$D" 2>&1 >> "$L"
        git add .                                            2>&1 >> "$L"  \
            && git commit -m "Autocommit $D"                 2>&1 >> "$L"  \
            && git push origin $(git branch --show-current)  2>&1 >> "$L"
        echo -e '\n--------------------------------------------------------------------------------\n' >> "$L"
        sleep $interval
    done
}

autocommit
