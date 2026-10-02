struct dstr {
    char *raw;
    unsigned len;
    unsigned cap;
};

typedef struct dstr *dstr;

dstr dstr_create(char *str);

dstr dstr_concat_raw(dstr str_a, char *str_b);

dstr dstr_concat(dstr str_a, dstr str_b);

unsigned dstr_len(dstr str);

// cmp
