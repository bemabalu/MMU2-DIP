#ifndef _MAIN_H
#define _MAIN_H

#include <inttypes.h>
#include <stdio.h>

void manual_extruder_selector();
void unrecoverable_error();
void drive_error();
void check_filament_not_present();
void signal_load_failure();
void signal_ok_after_load_failure();
void lcd (char const *status="");
extern uint8_t tmc_mode;
extern FILE* uart_com;

#endif //_MAIN_H
