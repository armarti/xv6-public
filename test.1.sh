#!/sh

BIN_DIR="$HOME/git/xv6-public"
declare -A SRC_FILES=(                        \
    [uniq.c]="$HOME/git/xv6-public/uniq.c"    \
    [hello.c]="$HOME/git/xv6-public/hello.c"  \
)

export PATH="$HOME/git/xv6-public:$PATH"
NPTS=100
recordresult() {
    local _RES=${1:-X}
    local _PTS=${2:-1000}
    if [[ "x$_RES" == "xX" ]]; then
        echo "ERROR"
        NPTS=$(( $NPTS - $_PTS ))
    elif [[ "x$_RES" != "x0" ]]; then
        echo "FAIL"
        NPTS=$(( $NPTS - $_PTS ))
    else
        echo "PASS"
    fi
}

### Test 1 #####################################################################
PTS=50
echo -n "$PTS pts: hello works - "

hello

recordresult $? $PTS
################################################################################

### Test 2 #####################################################################
PTS=10
echo -n "$PTS pts: exit() at the end of hello.c - "

grep -qE '\bexit\s*\(' "${SRC_FILES[hello.c]}"

recordresult $? $PTS
################################################################################

### Test 3 #####################################################################
PTS=50
echo -n "$PTS pts: uniq works - "



recordresult 1 $PTS
################################################################################

### Test 4 #####################################################################
PTS=10
echo -n "$PTS pts: uniq handles lines of more than 512 characters - "



recordresult 1 $PTS
################################################################################

### Test 5 #####################################################################
PTS=10
echo -n "$PTS pts: No debug printf left in code - "



recordresult 1 $PTS
################################################################################

### Test 6 #####################################################################
PTS=10
echo -n "$PTS pts: 'cat example.txt | uniq' work - "



recordresult 1 $PTS

### Test 7 #####################################################################
PTS=10
echo -n "$PTS pts: 'uniq -c example.txt' works - "



recordresult 1 $PTS
################################################################################

### Test 8 #####################################################################
PTS=10
echo -n "$PTS pts: 'uniq -d example.txt' works - "



recordresult 1 $PTS
################################################################################

### Test 9 #####################################################################
PTS=10
echo -n "$PTS pts: 'uniq -i example.txt' works - "



recordresult 1 $PTS
################################################################################

if [[ "x$NPTS" != "x100" ]]; then
    echo "Failed: $NPTS points."
    exit 1
fi
echo "Passed: $NPTS points."
exit 0
