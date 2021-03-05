#include "types.h"
#include "stat.h"
#include "user.h"

#define NULL ((void*)0)
#define DEBUG 1
#define BUF_SIZE 8
#define CACHE_BUF_SIZE 16
#define CACHE_EMPTY '\0'

typedef struct Cache_s {
    struct Cache_s *next;
    struct Cache_s **head;
    char *buf;
    int len;
    int mark;
} Cache;

void debug(char msg[]) { if(DEBUG) printf(2, "DEBUG: %s\n", msg); }

void debugCache(Cache *cache) {
    if(DEBUG) {
        if(NULL == cache) {
            debug("*(0x0) = { }");
            return;
        }
        int i;
        printf(2, "DEBUG: *(0x%x) = {\n", cache);
        printf(2, "DEBUG:     next = (0x%x);\n", cache->next);
        printf(2, "DEBUG:     head = (0x%x);\n", cache->head);
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
        printf(2, "DEBUG:     len = %d;\n", cache->len);
        printf(2, "DEBUG:     mark = %d;\n", cache->mark);
        printf(2, "DEBUG: }\n");
    }
}

Cache* rewindCache(Cache *cache) {
    cache->mark = 0;
    return *(cache->head);
}

Cache* eraseNode(Cache *node) {
    if(node) {
        node->len = 0;
        node->mark = 0;
    }
    return node;
}

Cache* getChar(char *c, Cache *cache) {

    debug("in `getChar`");

    if(!cache || !cache->len) {
        debug("returning CACHE_EMPTY");
        *c = CACHE_EMPTY;
        return cache;
    }
    
    if(cache->len == cache->mark) {
        cache->mark = 0;
        return getChar(c, cache->next);
    }

    *c = cache->buf[cache->mark++];

    if(DEBUG) {
        printf(2, "DEBUG: in `getChar`, got c = ");
        if(*c == '\n') printf(2, "'\\n'\n");
        else printf(2, "'%c'\n", *c);
    }

    return cache;
}

Cache* disposeCacheNode(Cache *node) {

    debug("in `disposeCacheNode`");

    if(node == NULL) return NULL;
    Cache *nextNode = node->next;
    if(nextNode != NULL && node == *(node->head)) {
        *(nextNode->head) = nextNode;
    }
    free(node->buf);
    free(node);
    return nextNode;
}

Cache* growCache(Cache *cacheEnd) {

    debug("in `growCache`");

    Cache *cache = malloc(sizeof(Cache));
    cache->buf = malloc(CACHE_BUF_SIZE * sizeof(char));
    eraseNode(cache);
    cache->next = NULL;

    if(cacheEnd == NULL) {
        // store ptr on heap
        cache->head = malloc(sizeof(Cache*));
        *(cache->head) = cache;
    } else {
        cache->head = cacheEnd->head;
        cacheEnd->next = cache;
    }

    return cache;
}

Cache* cacheChar(char c, Cache *cache) {

    Cache *cacheTail = cache;

    debug("in `cacheChar`");
    // debug("in `cacheChar`, initial `cacheTail`:");
    // debugCache(cacheTail);

    if(cacheTail->len == CACHE_BUF_SIZE) {
        cacheTail = cacheTail->next ? cacheTail->next : growCache(cacheTail);
    }

    cacheTail->buf[cacheTail->len++] = c;

    // debug("in `cacheChar`, final `cacheTail`:");
    // debugCache(cacheTail);

    return cacheTail;
}

char lastCharWritten = '\0';

Cache* dumpCache(Cache *cacheEnd) {

    debug("in `dumpCache`");

    Cache *cacheHead = *(cacheEnd->head), *currNode = cacheHead;
    while(currNode) {
        if(currNode->len) {
            write(1, currNode->buf, currNode->len);
            lastCharWritten = currNode->buf[(currNode->len)-1];
        }
        eraseNode(currNode);
        if(currNode == cacheEnd) break;
        currNode = currNode->next;
    }
    return cacheHead;
}

void uniq(const int fd) {

    debug("in `uniq`");

    char buf[BUF_SIZE];
    int i, n, fillPrevLine = 1;
    char currChar, prevChar;
    Cache *currLine = growCache(NULL),
          *prevLine = growCache(NULL);

    while((n = read(fd, buf, BUF_SIZE))) {
        debug("in while");

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
                    prevLine = rewindCache(prevLine);
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

                    Cache *emptyCache = dumpCache(prevLine);
                    prevLine = currLine;
                    fillPrevLine = 1;
                    currLine = emptyCache;

                }
                // B. previous line is equal to current line up to and not
                // including `currChar`, then it continues
                else if(prevChar != '\n' && currChar == '\n') {
                    debug("------------(I-2-B)");

                    Cache *emptyCache = dumpCache(prevLine);
                    prevLine = currLine;
                    currLine = emptyCache;

                }
                // C. full previous line identical to full current line
                else if(currChar == '\n' && prevChar == '\n') {
                    debug("------------(I-2-C)");

                    // erase currLine and reset position to head
                    Cache *tmp = *(currLine->head);
                    do {
                        if(eraseNode(tmp) == currLine) break;
                    } while((tmp = tmp->next));
                    currLine = rewindCache(currLine);

                    // reset prevLine position to head
                    prevLine = rewindCache(prevLine);

                }
                // D. previous line is equal to current line up to and not
                // including `currChar`, then it continues
                else if(currChar != prevChar) {
                    debug("------------(I-2-D)");

                    Cache *emptyCache = dumpCache(prevLine);
                    prevLine = currLine;
                    currLine = emptyCache;
                    fillPrevLine = 1;

                }

            }

        }

    }

    // purge buffers/caches
    prevLine = dumpCache(prevLine);
    if(!fillPrevLine) currLine = dumpCache(currLine);

    // account for file not ending in newline
    if(lastCharWritten != '\n') write(1, "\n", 1);

    // cleanup
    currLine = *(currLine->head);
    while((currLine = disposeCacheNode(currLine)));
    prevLine = *(prevLine->head);
    while((prevLine = disposeCacheNode(prevLine)));
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
