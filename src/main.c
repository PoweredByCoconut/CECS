#include <stdio.h>
#include "server.h"
#include "client.h"
#include "cecs.h"

int main(void) {
    printf("Do you want to host [y/N]? ");
    char host = get_char_clear();

    if (host == 'y' || host == 'Y') {
        run_server();
    } else {
        start_client();
    }
}
