#ifndef TEMP_STR_MAX_SIZE
    #define TEMP_STR_MAX_SIZE 500
#endif

#ifndef TEMP_STR_MAX_SIZE_LESS
    #define TEMP_STR_MAX_SIZE_LESS 499
#endif

#ifndef TRACE_STR_MAX_SIZE
    #define TRACE_STR_MAX_SIZE 102400
#endif

#ifndef CLASS_DISABLED_HASH_LIST_SIZE
    #define CLASS_DISABLED_HASH_LIST_SIZE 200
#endif


#ifndef FUNC_DISABLED_HASH_LIST_SIZE
    #define FUNC_DISABLED_HASH_LIST_SIZE 200
#endif

#ifndef SVIPC_PAK_MAX_SIZE
    #define SVIPC_PAK_MAX_SIZE TRACE_STR_MAX_SIZE
#endif

#ifndef CLASS_NAME_MAX_SIZE
    #define CLASS_NAME_MAX_SIZE 20
#endif

#ifndef FUNC_NAME_MAX_SIZE
    #define FUNC_NAME_MAX_SIZE 20
#endif


#ifndef FUNCTION_PROF_LIST_SIZE
    #define FUNCTION_PROF_LIST_SIZE 50
#endif

struct message_text {
    char buf[SVIPC_PAK_MAX_SIZE];
};

struct message {
    long message_type;
    unsigned int size;
    struct message_text message_text;
};



