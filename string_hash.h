//static unsigned long map_hash(const char *str) {
//    unsigned long hash = 5381;
//    while (*str) {
//        hash = ((hash << 5) + hash) + *str++;
//    }
//    return hash;
//}

struct string_hash_t {
    unsigned long hash_val;
    struct string_hash_t *next;
};

static unsigned long
map_hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

// BKDR Hash Function
unsigned long
BKDRHash(char *str, unsigned int len) {
    unsigned long seed = 131; /* 31 131 1313 13131 131313 etc.. */
    unsigned long hash = 0;
    unsigned int i = 0;
    for (i = 0; i < len; str++, i++) {
        hash = (hash * seed) + (*str);
    }
    return hash;
}

