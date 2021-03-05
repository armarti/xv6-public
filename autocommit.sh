#!/usr/bin/env bash
PATH="/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin"

function autocommit() {
    local interval="${1:-60}"
    while [ 1 ]; do
        git add . >> .git/autocommit.log                                  \
            && git commit -m "Autocommit $(date)" >> .git/autocommit.log  \
            && git push origin work
        sleep $interval
    done
}

autocommit
