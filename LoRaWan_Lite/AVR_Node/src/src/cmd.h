#include <Arduino.h>
#define MAX_MSG_SIZE    30

typedef void (*Func)(int argc, char **argv);

Func execFunc;

typedef struct _L_cmd
    {
	  const char *cmd;
	  const Func ExecSub;
	  const struct _L_cmd *next;
	} L_cmd;
	
	
const char cmd_prompt[] PROGMEM = "CMD >> ";
const char cmd_unrecog[] PROGMEM = "CMD: Command not recognized.";

void cmdInit(uint32_t);
void cmdPoll(void);
void cmd_handler(void);
void cmd_parse(char*);
void cmd_display(void);

byte msg[MAX_MSG_SIZE], *msg_ptr;

//Συνδεδεμένη λίστα με τις εντολές στην Program Memory
L_cmd *cmd_tbl;

void cmdInit(uint32_t speed)
{    
 msg_ptr = msg; //Αρχικοποίηση του δείκτη msg_ptr
 
    cmd_display(); //Εμφάνισε prompt
    Serial.begin(speed);
}

void cmdPoll(void)
{
 while (Serial.available())
       {
        cmd_handler();
       }
}

void cmd_handler()
{
    char c = Serial.read();

    switch (c)
    {
	//Πατήθηκε το enter
    case '\r':
        *msg_ptr = '\0';	//Τερμάτισε το msg με \0
		Serial.print("\r\n"); //Άλλαξε γραμμή
        cmd_parse((char *)msg);	//Κάλεσε συνάρτηση ελέγχου της εντολής
        msg_ptr = msg;		//Δείξε στην αρχή του μηνύματος για την επόμενη φορά
        break;
    //Πατήθηκε το backspace
    case '\b':
    case 127:  //Αν χρησιμοποιώ το putty //Κανονικά '\b'
        Serial.print(c);
        if (msg_ptr > msg)
        {
            msg_ptr--;
        }
        break;
    //Αποθήκευσε τους χαρακτήρες στο msg[] ώστε να δημιουργηθεί η εντολή
    default:
	    Serial.print(c);
        *msg_ptr++ = c;
        break;
    }
}

void cmd_parse(char *cmd)
{
    byte argc, i = 0;
    char *argv[30];
    char buf[40];
    L_cmd *cmd_entry;
    // parse the command line statement and break it up into space-delimited
    // strings. the array of strings will be saved in the argv array.
    if (strlen(cmd) > 0)
       {
        argv[i] = strtok(cmd, " ");
        do
          {
           argv[++i] = strtok(NULL, " ");
          } while ((i < 30) && (argv[i] != NULL));
    
        //Αποθήκευσε τον αριθμό των ορισμάτων
        argc = i;
	
	    //Αν δεν πατήθηκε απλώς enter
	    if (argv[0] != NULL)
	       {
	        //Ψάξε στο λεξικό να βρεις την εντολή που δόθηκε στο prompt. Το argv[0] έχει την εντολή
	        for (cmd_entry = cmd_tbl; cmd_entry != NULL; cmd_entry = (L_cmd*)pgm_read_word(&cmd_entry->next))
                {
                 if (!strcmp_P(argv[0], (char*)pgm_read_word(&cmd_entry->cmd)))
                    {
                     execFunc = (Func)pgm_read_word(&cmd_entry->ExecSub);
			         execFunc(argc, argv);
			         cmd_display();
                     return;
                    }
                }
            //Η εντολή δεν αναγνωρίστηκε. Εμφάνισε μήνυμα λάθους
            strcpy_P(buf, cmd_unrecog);
            Serial.println(buf);
           }
       }
    
	//Εμφάνισε το prompt
	cmd_display();
}

void cmd_display()
{
    char buf[40];
    strcpy_P(buf, cmd_prompt);
    Serial.print(buf);
}