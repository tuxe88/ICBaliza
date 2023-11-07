#ifndef STATUS_MANAGER_H
#define STATUS_MANAGER_H

typedef struct status_manager
{
  int connection_status;
  int build_status;
} status_manager;

int get_connection_status();
void set_connection_status(int newStatus);

int get_build_status();
void set_build_status(int newStatus);

#endif //STATUS_MANAGER_H