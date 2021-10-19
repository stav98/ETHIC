void help(int, char**);
void print_version(int, char**);
void eeclear(int, char**);
void set_speed(int, char**);

void add_commands()
{
 cmdAdd((char*) "?", help);
 cmdAdd((char*) "version", print_version);
 cmdAdd((char*) "eeclear", eeclear);
 cmdAdd((char*) "setspeed", set_speed);
}


void help(int arg_cnt, char **args)
{
 cmd_t *cmd_entry;
 for (cmd_entry = cmd_tbl; cmd_entry != NULL; cmd_entry = cmd_entry->next)
     {
      Serial.println(cmd_entry -> cmd);     
     }
}


void print_version(int arg_cnt, char **args)
{
 Serial.println("fffff");
}

void eeclear(int arg_cnt, char **args)
{
 Serial.println("kkkk");
}


//Αλλάζει τις ζητούμενες στροφές του κινητήρα
void set_speed(int arg_cnt, char **args)
{
 if (arg_cnt > 1)
    {
     Serial.print("SPEED ");
     Serial.println(atoi(args[1]));
    }
 else
     Serial.println("Too few arguments");
}
