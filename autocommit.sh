#!/usr/bin/env bash
export PATH="/usr/local/bin:/usr/bin:/bin:/home/linuxbrew/.linuxbrew/bin:/usr/local/sbin:/usr/sbin:/sbin:/home/linuxbrew/.linuxbrew/sbin"

function autocommit() {
    local interval="${1:-60}"
    while [ 1 ]; do
        D="$(date)"
        L="$PWD/.git/autocommit.log"
        echo "$D" &>> "$L"
        git add .                            &>> "$L"  \
            && git commit -m "Autocommit $D" &>> "$L"  \
            && git push origin work          &>> "$L"
        echo -e '\n--------------------------------------------------------------------------------\n' >> "$L"
        sleep $interval
    done
}

autocommit
