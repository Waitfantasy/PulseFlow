static unsigned map_hash(const char *str) {
    unsigned hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) ^ *str++;
    }
    return hash;
}