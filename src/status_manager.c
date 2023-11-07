#include "status_manager.h"

void status_manager_init(status_manager* sm){
    sm->connection_status = 0;
    sm->build_status = 0;
}

int get_connection_status(){
    return 0;
}

void set_connection_status(int newStatus){}

int get_build_status(){
    return 0;
}
void set_build_status(int newStatus){}