void init_commands(void);
void printErr1(void);
void setwspeed(int, char**);
void win_op(int, char**);
void win_cl(int, char**);
void win_fop(int, char**);
void win_fcl(int, char**);
void skia_op(int, char**);
void skia_cl(int, char**);
void skia_fop(int, char**);
void skia_fcl(int, char**);
void print_version(int, char**);
void help(int, char**);

const char PROGMEM \
   //        1         |           2         |         3            |         4            |          5          |
   ct01[] = "setwspeed", ct02[] = "winop"    , ct03[] = "wincl"     , ct04[] = "winfop"    , ct05[] = "winfcl"   ,
   ct06[] = "skiaop"   , ct07[] = "skiacl"   , ct08[] = "skiafop"   , ct09[] = "skiafcl"   , ct10[] = "version"  , 
   ct11[] = "?";
   

//Δομή συνδεδεμένης λίστας
const L_cmd PROGMEM cmd11 = {(char*) ct11   , help,             NULL};
const L_cmd PROGMEM cmd10 = {(char*) ct10   , print_version,  &cmd11};
const L_cmd PROGMEM cmd09 = {(char*) ct09   , skia_fcl,       &cmd10};
const L_cmd PROGMEM cmd08 = {(char*) ct08   , skia_fop,       &cmd09};
const L_cmd PROGMEM cmd07 = {(char*) ct07   , skia_cl,        &cmd08};
const L_cmd PROGMEM cmd06 = {(char*) ct06   , skia_op,        &cmd07};
const L_cmd PROGMEM cmd05 = {(char*) ct05   , win_fcl,        &cmd06};
const L_cmd PROGMEM cmd04 = {(char*) ct04   , win_fop,        &cmd05};
const L_cmd PROGMEM cmd03 = {(char*) ct03   , win_cl,         &cmd04};
const L_cmd PROGMEM cmd02 = {(char*) ct02   , win_op,         &cmd03};
const L_cmd PROGMEM cmd01 = {(char*) ct01   , setwspeed,      &cmd02};

void init_commands(void)
{
 cmd_tbl = (L_cmd*) &cmd01;
}

void printErr1()
{
 Serial.println(F("Too few arguments"));
}

//Αλλάζει τις ζητούμενες στροφές του κινητήρα
void setwspeed(int arg_cnt, char **args)
{
 if (arg_cnt > 1)
    {
     win_speed = atoi(args[1]);
    }
 else
     printErr1();
}

//Ανοίγει παράθυρα για κάποιο χρόνο
void win_op(int arg_cnt, char **args)
{
 byte w;
 unsigned int t;
 if (arg_cnt > 2)
    {
     w = atoi(args[1]);
     t = atoi(args[2]);
     if (w == 1)
         win1_open(t);
     else if (w == 2)
         win2_open(t);
    }
 else
     printErr1();
}

//Κλείνει παράθυρα για κάποιο χρόνο
void win_cl(int arg_cnt, char **args)
{
 byte w;
 unsigned int t;
 if (arg_cnt > 2)
    {
     w = atoi(args[1]);
     t = atoi(args[2]);
     if (w == 1)
         win1_close(t);
     else if (w == 2)
         win2_close(t);
    }
 else
     printErr1();
}

//Ανοίγει πλήρως παράθυρα
void win_fop(int arg_cnt, char **args)
{
 byte w;
 if (arg_cnt > 1)
    {
     w = atoi(args[1]);
     if (w == 1)
         win1_open(MAX_WIN_TIME);
     else if (w == 2)
         win2_open(MAX_WIN_TIME);
    }
 else
     printErr1();
}

//Κλείνει πλήρως παράθυρα
void win_fcl(int arg_cnt, char **args)
{
 byte w;
 if (arg_cnt > 1)
    {
     w = atoi(args[1]);
     if (w == 1)
         win1_close(MAX_WIN_TIME);
     else if (w == 2)
         win2_close(MAX_WIN_TIME);
    }
 else
     printErr1();
}

//Μαζεύει την σκίαση για κάποιο χρόνο
void skia_op(int arg_cnt, char **args)
{
 unsigned int t;
 if (arg_cnt > 1)
    {
     t = atoi(args[1]);
     skia_open(t);
    }
 else
     printErr1();
}

//Απλώνει την σκίαση για κάποιο χρόνο
void skia_cl(int arg_cnt, char **args)
{
 unsigned int t;
 if (arg_cnt > 1)
    {
     t = atoi(args[1]);
     skia_close(t);
    }
 else
     printErr1();
}

//Μαζεύει πλήρως την σκίαση 
void skia_fop(int arg_cnt, char **args)
{
 skia_open(MAX_SKIA_TIME);
}

//Απλώνει πλήρως την σκίαση 
void skia_fcl(int arg_cnt, char **args)
{
 skia_close(MAX_SKIA_TIME);
}

void print_version(int arg_cnt, char **args)
{
 Serial.print(F("Version: "));
 Serial.println(VERSION);
}

void help(int arg_cnt, char **args)
{
 char buf[10];
 L_cmd *cmd_entry;
 for (cmd_entry = cmd_tbl; cmd_entry != NULL; cmd_entry = (L_cmd*)pgm_read_word(&cmd_entry->next))
     {
      strcpy_P(buf, (char*)pgm_read_word(&cmd_entry->cmd));
      Serial.println(buf);
     }
}
