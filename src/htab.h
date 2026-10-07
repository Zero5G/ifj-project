typedef enum
{
    Symbol_var,
    Symbol_val,
    Symbol_def,
} Symbol_kind;


typedef enum
{
    Data_type_int,
    Data_type_string,
    Data_type_double,
    Data_type_unit,
    Data_type_boolean,
} Symbol_data_type;


typedef enum
{
    State_empty,
    State_full,
    State_delete,
} Symbol_state;

//Struktura zaznam o symbolu
typedef struct
{
    char *key;
    Symbol_kind kind;
    Symbol_data_type data_type;
    Symbol_state state;
    int param_count;
    Symbol_data_type *param_types;
} Hash_item;


//Struktura hash tabulky
typedef struct
{
    Hash_item *array;
    int capacity;
    int active_size;
} Hash_table;

