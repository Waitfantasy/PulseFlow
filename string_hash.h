unsigned long
BKDRHash(char *str, unsigned int len) {
    unsigned long seed = 1313; /* 31 131 1313 13131 131313 etc.. */
    unsigned long hash = 0;
    unsigned int i = 0;
    for (i = 0; i < len; str++, i++) {
        hash = (hash * seed) + (*str);
    }
    return hash;
}

