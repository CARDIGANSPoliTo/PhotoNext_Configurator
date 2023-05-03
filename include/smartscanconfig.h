#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

/*******************************************************************************
* included libraries
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include <libutils/utils.h>
#include <libsmartscan/smartscan_utils.h>

/*******************************************************************************
* constants
*******************************************************************************/
#define DIAGNOSTIC_TIMEOUT 4

#define SRC_ADD "127.0.0.1"
#define DEST_ADD "127.0.0.1"

#define EMU_LOCAL 1

#define STD_CHOICE_SIZE 20

/*******************************************************************************
* menu entries
*******************************************************************************/
size_t create_maintenance_message(uint8_t *message);

size_t configure_state(uint8_t *message);
size_t configure_demo(uint8_t *message);
size_t configure_raw_scan_rate(uint8_t *message);
size_t configure_cont_tx_rate(uint8_t *message);
size_t configure_ch_format(uint8_t *message);
size_t configure_ch_thresold(uint8_t *message);
size_t configure_start_ch_freq(uint8_t *message);
size_t configure_scan_speed(uint8_t *message);
size_t configure_ip_addr(uint8_t *message);
size_t configure_subnet(uint8_t *message);
size_t configure_gateway(uint8_t *message);
size_t configure_generic_slot(uint8_t *message);
size_t configure_single_slot(uint8_t *message);
size_t configure_agc(uint8_t *message);
size_t configure_utc_local(uint8_t *message);

static const uint8_t default_maintenance_config[] =
{
  0xaa, 0x55, 0xe0, 0x0e, 0x00, 0x00,
  // 0x02, 0x01, 0x00,  // demo mode
  0x05, 0x02, 0x40, 0x24,  // CPU, DSP, 2 gratings, 4 channels
  0x04, 0x02, 0x00, 0xff,  // tx time 10 ms
  0x08, 0x02, 0x00, 0x00,  // start channel freq
  0x09, 0x02, 0x81, 0x90,  // 100 nr scans, 10 us
  0x01, 0x01, 0x02,  // set operational state
  // 0x00, 0x00,
  0xff
};

#endif
