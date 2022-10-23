# Simple C Config Reader

## Usage

Config files are read in the format `key DELIM value`. DELIM is defined in the header and must be a character literal.

Spaces before a key or around DELIM are ignored. After the first non-space character in the value, spaces are included. This means a string value with spaces inside is simple `key = string with spaces`

Example config file:

```
str_config=my string
int_config =2
float_config = 3.14
```

Reading the config file would look like this:

```c
char my_str[MAX_VAL_LEN];
int my_int;
float my_float;

cfg_t cfg[] = {
  /* key            format  pointer */
  { "str_config",   "%s",   &my_str },
  { "int_config",   "%d",   &my_int },
  { "float_config", "%f",   &my_float },
};

FILE *f = fopen("config", "r");

/*      FILE*   cfg_t[],  number of items */
readcfg(f,      cfg,      sizeof cfg / sizeof cfg[0]);

fclose(f);
```

`readcfg` simple takes a `FILE*`, meaning it can also read from stdin.
