#include <stdio.h>
#include <unistd.h>

#include "jabbah.h"

int
main ()
{
        jabbah_connect("bazyl", "n3ssQick", "Moja biblioteczka", 0, "bazyl.net", 5222, 0);
        presence_set_status(STATE_ONLINE, "Dziala biblioteczka :P");
 //       sleep(30);
        message_send_chat("bazyl@bazyl.net/Ajbuk", "To jest wiadomosc testowa z biblioteki:P");
        jabbah_close("Rozlaczanko");
}
