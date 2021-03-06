#include "types.h"
#include "stat.h"
#include "user.h"

#define NULL ((void*)0)
#define BUF_SIZE 8
#define CACHE_BUF_SIZE 16
#define CACHE_EMPTY '\0'

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef DEBUGX
#define DEBUGX 0
#endif

char LAST_WRITE_CHAR = '\0';

typedef struct Cache_s {
    struct Cache_s *next;
    struct Cache_s **head;
    struct Cache_s **tail;
    char *buf;
    int len;
    int mark;
} Cache;

void debug(char msg[]) { if(DEBUG) printf(2, "DEBUG: %s\n", msg); }

void __printCache(Cache *cache, char name[]) {
    if(NULL == cache) {
        debug("*(0x0) = { }");
        return;
    }
    int i;
    printf(2, "DEBUG: %s = *(0x%x) {\n", name, cache);
    printf(2, "DEBUG:     next = (0x%x);\n", cache->next);
    printf(2, "DEBUG:     head = (0x%x) --> (0x%x);\n", cache->head, *(cache->head));
    printf(2, "DEBUG:     tail = (0x%x) --> (0x%x);\n", cache->tail, *(cache->tail));
    printf(2, "DEBUG:     len = %d;\n", cache->len);
    printf(2, "DEBUG:     mark = %d;\n", cache->mark);
    printf(2, "DEBUG:     buf = [");
    if(cache->len) {
        printf(2, " ");
        for(i = 0; i < cache->len; ++i) {
            char c = cache->buf[i];
            if(c == '\n') printf(2, "'\\n'");
            else printf(2, "'%c'", c);
            if(i != (cache->len - 1)) printf(2, ",");
            printf(2, " ");
        }
    }
    printf(2, "];\n");
    printf(2, "DEBUG: }\n");
}

void debugXCache(Cache *cache, char name[]) { if(DEBUGX) return __printCache(cache, name); }

void debugCache(Cache *cache, char name[]) { if(DEBUG) return __printCache(cache, name); }

Cache* getChar(char *c, Cache *cache) {
    debug("entering `getChar`");
    // if(!cache || !cache->len) {
    //     debug("returning CACHE_EMPTY");
    //     *c = CACHE_EMPTY;
    //     return cache;
    // }
    if(cache->len == cache->mark) {
        cache->mark = 0;
        debug("leaving `getChar[1]`");
        return getChar(c, cache->next);
    }
    *c = cache->buf[cache->mark++];
    if(DEBUG) {
        printf(2, "DEBUG: in `getChar`, got c = ");
        if(*c == '\n') printf(2, "'\\n'\n");
        else printf(2, "'%c'\n", *c);
    }
    debug("leaving `getChar[2]`");
    return cache;
}

void disposeCache(Cache *cache) {
    debug("entering `disposeCache`");
    if(cache == NULL) {
        debug("leaving `disposeCache`[1]");
        return;
    }
    Cache *curr = *(cache->head),
          *next = curr->next;
    while(next) {
        free(curr->buf);
        free(curr);
        curr = next;
        next = curr->next;
    }
    free(curr->head);
    free(curr->tail);
    free(curr->buf);
    free(curr);
    debug("leaving `disposeCache`[2]");
}

Cache* growCache(Cache *currNode) {
    debug("entering `growCache`");
    debugCache(currNode, "currNode");
    Cache *tailNode = currNode;
    debug("====(X)");
    if(tailNode) {
        debug("========(X-1)");
        tailNode = *(currNode->tail);
        if(tailNode->next) {  // reuse previously created `Cache` node
            tailNode = tailNode->next;
            *(tailNode->tail) = tailNode;
        } else {  // cache isn't long enough, add new node
            debug("============(X-1-a)");
            tailNode->next = malloc(sizeof(Cache));
            tailNode->next->head = tailNode->head;
            tailNode->next->tail = tailNode->tail;
            tailNode = *(tailNode->next->tail) = tailNode->next;
        }
    } else {
        debug("========(X-2)");
        tailNode = malloc(sizeof(Cache));
        tailNode->head = malloc(sizeof(Cache*));
        tailNode->tail = malloc(sizeof(Cache*));
        *(tailNode->head) = *(tailNode->tail) = tailNode;
    }
    debug("====(Y)");
    tailNode->buf = malloc(CACHE_BUF_SIZE * sizeof(char));
    tailNode->next = NULL;
    tailNode->len = 0;
    tailNode->mark = 0;
    debugCache(tailNode, "tailNode");
    debug("leaving `growCache`");
    return tailNode;
}

Cache* cacheChar(char c, Cache *cache) {
    debug("entering `cacheChar`");
    Cache *cacheTail = cache;
    if(cacheTail->len == CACHE_BUF_SIZE) {
        cacheTail = growCache(cacheTail);
    }
    cacheTail->buf[cacheTail->len++] = c;
    debugCache(cacheTail, "cacheTail");
    debug("leaving `cacheChar`");
    return cacheTail;
}

Cache* dumpCache(Cache *cache, int destFd) {
    debug("entering `dumpCache`");
    Cache *cacheHead = *(cache->head),
          *cacheTail = *(cache->tail),
          *currNode = cacheHead;
    while(currNode) {
        if(currNode->len && destFd >= 0) {
            if(-1 == write(destFd, currNode->buf, currNode->len)) {
                printf(2,  "Write error");
                exit();
            }
            LAST_WRITE_CHAR = currNode->buf[(currNode->len)-1];
        }
        currNode->len = currNode->mark = 0;
        if(currNode == cacheTail) break;
        currNode = currNode->next;
    }
    debug("leaving `dumpCache`");
    return (*(cacheHead->tail) = *(cacheHead->head) = cacheHead);
}

void uniq(const int fd) {

    debug("entering `uniq`");

    char buf[BUF_SIZE], bufEndChar = '\0';
    int i, n;
    short fillPrevLine = 1, eof = 0;
    char currChar, prevChar;
    Cache *currLine = growCache(NULL),
          *prevLine = growCache(NULL),
          *emptyCache = NULL;

    while(1) {
        debug("in while");

        n = read(fd, buf, BUF_SIZE);
        if(n) {
            bufEndChar = buf[n-1];
        } else {
            eof = 1;
            if('\n' != bufEndChar) {
                buf[0] = '\n';
                n = 1;
            }
        }

        // I.
        for(i = 0; i < n; ++i) {
            debug("----(I)");

            currChar = buf[i];

            // 1. `prevLine` doesn't contain a full line
            if(fillPrevLine) {
                debug("--------(I-1)");

                prevLine = cacheChar(currChar, prevLine);
                // A.
                if(currChar == '\n') {
                    debug("------------(I-1-A)");
                    prevLine = *(prevLine->head);
                    fillPrevLine = 0;
                }

            }
            // 2. `prevLine` contains a full line
            else {
                debug("--------(I-2)");

                currLine = cacheChar(currChar, currLine);
                prevLine = getChar(&prevChar, prevLine);

                if(DEBUG) {
                    printf(2, "DEBUG: --------(I-2) currChar = ", currChar);
                    if(currChar == '\n') printf(2, "'\\n'\n");
                    else printf(2, "'%c'\n", currChar);
                    printf(2, "DEBUG: --------(I-2) prevChar = ", prevChar);
                    if(currChar == '\n') printf(2, "'\\n'\n");
                    else printf(2, "'%c'\n", prevChar);
                }

                // A. previous line is equal to current line up to and not
                // including `currChar`, then it ends
                if(prevChar == '\n' && currChar != '\n') {
                    debug("------------(I-2-A)");

                    emptyCache = dumpCache(prevLine, 1);
                    prevLine = currLine;
                    currLine = emptyCache;
                    emptyCache = NULL;
                    fillPrevLine = 1;

                }
                // B. previous line is equal to current line up to and not
                // including `currChar`, then it continues
                else if(prevChar != '\n' && currChar == '\n') {
                    debug("------------(I-2-B)");

                    emptyCache = dumpCache(prevLine, 1);
                    prevLine = currLine;
                    currLine = emptyCache;
                    emptyCache = NULL;

                }
                // C. full previous line identical to full current line
                else if(currChar == '\n' && prevChar == '\n') {
                    debug("------------(I-2-C)");

                    // reset prevLine position to head
                    prevLine->mark = 0;
                    prevLine = *(prevLine->head);

                    // erase currLine and reset position to head
                    currLine = dumpCache(currLine, -1);

                }
                // D. previous line is equal to current line up to and not
                // including `currChar`, then it continues
                else if(currChar != prevChar) {
                    debug("------------(I-2-D)");

                    debugCache(prevLine, "prevLine");
                    debugCache(currLine, "currLine");

                    emptyCache = dumpCache(prevLine, 1);
                    prevLine = currLine;
                    currLine = emptyCache;
                    emptyCache = NULL;
                    fillPrevLine = 1;

                }

            }

        }

        if(eof) break;

    }

    // // purge buffers/caches
    debugXCache(prevLine, "prevLine");
    prevLine = dumpCache(prevLine, 1);

    debugXCache(currLine, "currLine");
    // if(!fillPrevLine) currLine = dumpCache(currLine, 1);
    currLine = dumpCache(currLine, 1);

    // // account for file not ending in newline
    // if(LAST_WRITE_CHAR != '\n') write(1, "\n", 1);

    // cleanup
    disposeCache(currLine);
    disposeCache(prevLine);
}

int main(int argc, char *argv[]) {
    if(argc <= 1) {
        uniq(0);
        exit();
    }
    int fd, i;
    for(i = 1; i < argc; ++i) {
        if((fd = open(argv[i], 0)) < 0) {
            printf(2, "uniq: cannot open \"%s\"\n", argv[i]);
            exit();
        }
        uniq(fd);
        close(fd);
    }
    exit();
}
