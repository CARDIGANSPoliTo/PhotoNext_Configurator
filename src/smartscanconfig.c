/*******************************************************************************
* included libraries
*******************************************************************************/
#include "../include/smartscanconfig.h"

/*******************************************************************************
* global variables
*******************************************************************************/
volatile sig_atomic_t stop_process;

/*******************************************************************************
* signal handling
*******************************************************************************/
void sigint_handler(int signal) {
  printf("Exiting configurator\n");

  stop_process = 1;
  // exit(0);
}

/*******************************************************************************
* thread functions
*******************************************************************************/
void *menu_th(void * args)
{
  char choice[STD_CHOICE_SIZE];

  uint8_t message[MSG_LIMIT_MTU];
  uint8_t state = 0;

  int error = 0;
  int len;

  int socket_fd;
  struct sockaddr_in src, dest;

  printf("Menu thread started\n");

  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
  {
    printf("Unable to open send socket\n");
    exit(1);
  }

  src.sin_family = AF_INET;
  src.sin_port = htons(30050);

  dest.sin_family = AF_INET;
  dest.sin_port = htons(PORT_RX_DIAG);

  if (inet_aton(SRC_ADD, &(src.sin_addr)) == 0)
  {
    printf("Invalid source IP address\n");
    exit(1);
  }

  if (inet_aton(DEST_ADD, &(dest.sin_addr)) == 0)
  {
    printf("Invalid destination IP address\n");
    exit(1);
  }

  if((bind(socket_fd, (struct sockaddr*)&(src), sizeof(src))) == -1)
  {
    printf("Socket bind failed\n");
    exit(1);
  }

  printf("Open tx socket on %s\n", inet_ntoa(src.sin_addr));

  while(!stop_process)
  {
    memset((void *)message, 0, MSG_LIMIT_MTU);
    error = 0;

    printf("\n* SMARTSCAN CONFIGURATOR MENU *\n*\n");
    printf("* - a: send diagnostic message\n");
    printf("* - b: send default maintenance message\n");
    printf("* - c: send custom maintenance message\n");
    printf("*\n");
    printf("* - z: exit\n");
    printf("* - Choice: ");

    fgets(choice, STD_CHOICE_SIZE - 1, stdin);

    switch(choice[0]) {
      case 'a':
        printf("* - Choose state:\n");
        printf("*   - 0: no change\n");
        printf("*   - 1: standby\n");
        printf("*   - 2: operational\n");
        printf("*   - 3: maintenance\n");
        printf("*   - 4: test\n");
        printf("*   - Choice: ");

        fgets(choice, STD_CHOICE_SIZE - 1, stdin);

        switch(choice[0]) {
          case '0':
            printf("*   - Set state no change\n");
            state = SSI_STATE_NO_CHANGE;
            break;
          case '1':
            printf("*   - Set state standby\n");
            state = SSI_STATE_STAND_BY;
            break;
          case '2':
            printf("*   - Set state operational\n");
            state = SSI_STATE_OPERATIONAL;
            break;
          case '3':
            printf("*   - Set state maintenance\n");
            state = SSI_STATE_MAINTENANCE;
            break;
          case '4':
            printf("*   - Set state test\n");
            state = SSI_STATE_TEST;
            break;
          default:
            printf("*   - Invalid state\n");
            error = 1;
            break;
        }
        if( !error && ((ssi_create_diagnostic_msg(message, MSG_DIAGNOSTIC_SIZE, state)) == STATUS_OK))
        {
          #if EMU_LOCAL == 1
            dest.sin_port = htons(30011);
          #else
            dest.sin_port = htons(PORT_RX_DIAG);
          #endif
          if((sendto(socket_fd, message, MSG_DIAGNOSTIC_SIZE, 0, (struct sockaddr *) &dest, (socklen_t) sizeof(dest))) == -1)
          {
            printf("Unable to send message\n");
          }
          else
          {
            printf("Sent packet of length %ld from %s:%d to %s:%d\n", (size_t) MSG_DIAGNOSTIC_SIZE, inet_ntoa(src.sin_addr), ntohs(src.sin_port), inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
          }
        }
        else
        {
          printf("Unable to create diagnostic message\n");
        }
        break;

      case 'b':
        #if EMU_LOCAL == 1
          dest.sin_port = htons(30012);
        #else
          dest.sin_port = htons(PORT_RX_MAIN);
        #endif
        if((sendto(socket_fd, default_maintenance_config, sizeof(default_maintenance_config), 0, (struct sockaddr *) &dest, (socklen_t) sizeof(dest))) == -1)
        {
          printf("Unable to send message\n");
        }
        else
        {
          printf("Sent packet of length %ld from %s:%d to %s:%d\n", sizeof(default_maintenance_config), inet_ntoa(src.sin_addr), ntohs(src.sin_port), inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
        }
        break;

      case 'c':
        #if EMU_LOCAL == 1
          dest.sin_port = htons(30012);
        #else
          dest.sin_port = htons(PORT_RX_MAIN);
        #endif
        if((len = create_maintenance_message(message)) > 0)
        {
          if((sendto(socket_fd, message, (size_t) len, 0, (struct sockaddr *) &dest, (socklen_t) sizeof(dest))) == -1)
          {
            printf("Unable to send message\n");
          }
          else
          {
            printf("Sent packet of length %ld from %s:%d to %s:%d\n", (size_t) len, inet_ntoa(src.sin_addr), ntohs(src.sin_port), inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
          }
          break;
        }
        break;

      case 'z':
        printf("* - Exit\n");
        stop_process = 1;
        break;

      default:
        printf("* - Invalid option\n");
        break;
    }

  }
  return (void *)0;
};

/*******************************************************************************
* menu entries
*******************************************************************************/
size_t create_maintenance_message(uint8_t *message)
{
  char choice[STD_CHOICE_SIZE];
  int exit_loop = 0;
  int send_msg = 0;

  size_t current_index = 0;

  current_index += ssi_write_maint_header(message);

  printf("\n* SMARTSCAN MAINTENANCE MESSAGE MENU *\n*\n");
  printf("* Preparing new configuration message\n");

  while(!exit_loop && !send_msg)
  {
    printf("*\n");
    printf("* Choose one of the following options:\n");
    printf("* - a : set state\n");
    printf("* - b : set demo mode\n");
    printf("* - c : configure raw scan rate\n");
    printf("* - d : configure continous data tx rate\n");
    printf("* - e : configure channel format\n");
    printf("* - f : configure channel thresholds\n");
    printf("* - g : configure start channel frequency\n");
    printf("* - h : configure scan speed\n");
    printf("* - i : configure ip address\n");
    printf("* - j : configure subnet mask\n");
    printf("* - k : configure gateway address\n");
    printf("* - l : configure generic AGC slot\n");
    printf("* - m : configure single AGC slot\n");
    printf("* - n : configure AGC update speed\n");
    printf("* - o : set local time\n");
    printf("*\n");
    printf("* - w : print message \n");
    printf("* - x : send message \n");
    printf("* - y : discard \n");
    printf("*\n");
    printf("* - z : exit\n");

    printf("* - Choice: ");

    // scanf("%c%*c", &choice);
    fgets(choice, 19, stdin);

    switch(choice[0])
    {
      case 'a':
        current_index += configure_state(message + current_index);
        break;

      case 'b':
        current_index += configure_demo(message + current_index);
        break;

      case 'c':
        current_index += configure_raw_scan_rate(message + current_index);
        break;

      case 'd':
        current_index += configure_cont_tx_rate(message + current_index);
        break;

      case 'e':
        current_index += configure_ch_format(message + current_index);
        break;

      case 'f':
        current_index += configure_ch_thresold(message + current_index);
        break;

      case 'g':
        current_index += configure_start_ch_freq(message + current_index);
        break;

      case 'h':
        current_index += configure_scan_speed(message + current_index);
        break;

      case 'i':
        current_index += configure_ip_addr(message + current_index);
        break;

      case 'j':
        current_index += configure_subnet(message + current_index);
        break;

      case 'k':
        current_index += configure_gateway(message + current_index);
        break;

      case 'l':
        current_index += configure_generic_slot(message + current_index);
        break;

      case 'm':
        current_index += configure_single_slot(message + current_index);
        break;

      case 'n':
        current_index += configure_agc(message + current_index);
        break;

      case 'o':
        current_index += configure_utc_local(message + current_index);
        break;

      case 'w':
        dump_raw_message(message, current_index);
        break;

      case 'x':
        printf("* - Sending message...\n");
        if(current_index > HD_MAINTENANCE_SIZE)
        {
          current_index += ssi_write_maint_padding(message + current_index, current_index);
          dump_raw_message(message, current_index);

          // send message
          send_msg = 1;
        }
        else
        {
          printf("* - Message payload is empty\n");
        }
        break;

      case 'y':
        printf("* - Discard message\n");
        memset((void *)(message + HD_MAINTENANCE_SIZE), 0, MSG_LIMIT_MTU - HD_MAINTENANCE_SIZE);
        current_index = HD_MAINTENANCE_SIZE;
        break;

      case 'z':
        printf("* - Exit\n");
        current_index = 0;
        exit_loop = 1;
        break;

      default:
        printf("* - Invalid command\n");
        break;
    }
  }

  printf("\n\n");

  return current_index;
};

size_t configure_state(uint8_t *message)
{
  char choice[STD_CHOICE_SIZE];
  size_t increment = 0;
  uint8_t state = 0;

  printf("* - Choose state:\n");
  printf("*   - 0: no change\n");
  printf("*   - 1: standby\n");
  printf("*   - 2: operational\n");
  printf("*   - 3: maintenance\n");
  printf("*   - 4: test\n");
  printf("*   - Choice: ");

  fgets(choice, STD_CHOICE_SIZE - 1, stdin);

  switch(choice[0]) {
    case '0':
      printf("*   - Set state no change\n");
      state = 0;
      increment = ssi_write_state(message, state);
      break;
    case '1':
      printf("*   - Set state standby\n");
      state = 1;
      increment = ssi_write_state(message, state);
      break;
    case '2':
      printf("*   - Set state operational\n");
      state = 2;
      increment = ssi_write_state(message, state);
      break;
    case '3':
      printf("*   - Set state maintenance\n");
      state = 3;
      increment = ssi_write_state(message, state);
      break;
    case '4':
      printf("*   - Set state test\n");
      state = 4;
      increment = ssi_write_state(message, state);
      break;
    default:
      printf("*   - Invalid state\n");
      break;
  }

  return increment;
};
size_t configure_demo(uint8_t *message)
{
  size_t increment = 0;
  char choice[STD_CHOICE_SIZE];

  uint8_t mode;

  printf("* - choose demo mode:\n");
  printf("*   - 00-03: channel voltage\n");
  printf("*   - 04-07: internal generated peaks\n");
  printf("*   - 08-11: slot positions\n");
  printf("*   - 12-15: agc gain slots\n");
  printf("*   - 16-STD_CHOICE_SIZE - 1: test ramp\n");
  printf("*   - Choice: ");

  fgets(choice, STD_CHOICE_SIZE - 1, stdin);

  mode = atoi(choice);

  switch(mode) {
    case 0:
    case 1:
    case 2:
    case 3:
      printf("*   - Set channel %d voltage output\n", mode);
      increment = ssi_write_demo(message, mode);
      break;
    case 4:
    case 5:
    case 6:
    case 7:
      printf("*   - Set internal generated peaks\n");
      increment = ssi_write_demo(message, mode);
      break;
    case 8:
    case 9:
    case 10:
    case 11:
      printf("*   - Set slot positions\n");
      increment = ssi_write_demo(message, mode);
      break;
    case 12:
    case 13:
    case 14:
    case 15:
      printf("*   - Set ACG slot gains\n");
      increment = ssi_write_demo(message, mode);
      break;
    case 16:
    case 17:
    case 18:
    case 19:
      printf("*   - Set test ramps\n");
      increment = ssi_write_demo(message, mode);
      break;
    default:
      printf("*   - Invalid demo mode\n");
      break;
  }

  return increment;
};

size_t configure_raw_scan_rate(uint8_t *message)
{
  size_t increment = 0;
  char choice[STD_CHOICE_SIZE];

  uint16_t rate;

  printf("* - Select raw scan rate:\n");
  printf("*   - 0: OFF\n");
  printf("*   - 1-32: approximate number of raw scans per second\n");
  printf("*   - Choice: ");

  fgets(choice, STD_CHOICE_SIZE - 1, stdin);

  rate = atoi(choice);

  if(rate < 0 || rate > 32)
  {
    printf("*   - Invalid scan rate (must be between 0 and 32)\n");
  }
  else
  {
    increment += ssi_write_raw_data_scan(message, rate);
    printf("*   - Set scan rate to %d scans per second\n", rate);
  }

  return increment;
};

size_t configure_cont_tx_rate(uint8_t *message)
{
  size_t increment = 0;
  char choice[STD_CHOICE_SIZE];

  uint16_t rate;

  printf("* - Select continous data transmission rate:\n");
  printf("*   - 0: disable\n");
  printf("*   - 1-255: \n");
  printf("*   - Choice: ");

  fgets(choice, STD_CHOICE_SIZE - 1, stdin);

  rate = atoi(choice);

  if(rate < 0 || rate > 255)
  {
    printf("*   - Invalid transmission rate (must be between 0 and 255)\n");
  }
  else
  {
    increment += ssi_write_cont_tx_rate(message, rate);
    printf("*   - Set transmission rate to %d\n", rate);
  }

  return increment;
};

size_t configure_ch_format(uint8_t *message)
{
  size_t increment = 0;
  char choice[STD_CHOICE_SIZE];

  int error = 0;

  uint8_t cpu_mode = 0;
  uint8_t dsp_mode = 0;
  uint8_t gratings = 16;
  uint8_t channels = 4;

  printf("* - Select channel format:\n");

  printf("* - Select CPU read method:\n");
  printf("*   - 0: select channel (DEFAULT)\n");
  printf("*   - 1: all 64\n");
  printf("*   - Choice: ");

  fgets(choice, STD_CHOICE_SIZE - 1, stdin);
  cpu_mode = atoi(choice);

  printf("* - Select DSP read method:\n");
  printf("*   - 0: slot address\n");
  printf("*   - 1: contiguous (disable AGC)\n");
  printf("*   - Choice: ");

  fgets(choice, STD_CHOICE_SIZE - 1, stdin);
  dsp_mode = atoi(choice);

  if(dsp_mode < 0 || dsp_mode > 1)
  {
    printf("*   - Invalid DSP read method (only 0 or 1 allowed)\n");
    error = 1;
  }

  if(cpu_mode == 0)
  {
    printf("* - Insert number of gratings (1-16): ");
    fgets(choice, STD_CHOICE_SIZE - 1, stdin);
    gratings = atoi(choice);

    printf("* - Insert number of channels (1-4) : ");
    fgets(choice, STD_CHOICE_SIZE - 1, stdin);
    channels = atoi(choice);
  }
  else if(cpu_mode == 1)
  {
    printf("* - Select number of gratings:\n");
    printf("*   - a: 1\n");
    printf("*   - b: 2\n");
    printf("*   - c: 4\n");
    printf("*   - d: 8\n");
    printf("*   - e: 16\n");
    printf("*   - Choice: ");

    fgets(choice, STD_CHOICE_SIZE - 1, stdin);

    switch(choice[0])
    {
      case 'a':
        gratings = 1;
        break;
      case 'b':
        gratings = 2;
        break;
      case 'c':
        gratings = 4;
        break;
      case 'd':
        gratings = 8;
        break;
      case 'e':
        gratings = 16;
        break;
      default:
        printf("*   - Invalid choice\n");
        error = 1;
        break;
    }

    printf("* - Select number of channels:\n");
    printf("*   - a: 1\n");
    printf("*   - b: 2\n");
    printf("*   - c: 4\n");
    printf("*   - Choice: ");

    fgets(choice, STD_CHOICE_SIZE - 1, stdin);

    switch(choice[0])
    {
      case 'a':
        channels = 1;
        break;
      case 'b':
        channels = 2;
        break;
      case 'c':
        channels = 4;
        break;
      default:
        printf("*   - Invalid choice\n");
        error = 1;
        break;
    }
  }
  else
  {
    printf("*   - Invalid CPU read method (only 0 or 1 allowed)\n");
    error = 1;
  }

  if(!error)
  {
    increment = ssi_write_ch_format(message, cpu_mode, dsp_mode, gratings, channels);

    printf("*   - Set channel format to:\n");
    printf("*     - CPU read method : %d\n", cpu_mode);
    printf("*     - DSP read mode   : %d\n", dsp_mode);
    printf("*     - Gratings        : %d\n", gratings);
    printf("*     - Channels        : %d\n", channels);
  }

  return increment;
};

size_t configure_ch_thresold(uint8_t *message)
{
  size_t increment = 0;
  char choice[STD_CHOICE_SIZE];

  uint8_t ch = 9;
  uint16_t th;

  printf("* - Insert threshold: ");

  fgets(choice, STD_CHOICE_SIZE - 1, stdin);

  th = atoi(choice);

  printf("* - Select channel:\n");
  printf("*   - 0: channel 0\n");
  printf("*   - 1: channel 1\n");
  printf("*   - 2: channel 2\n");
  printf("*   - 3: channel 3\n");
  printf("*   - 9: all\n");
  printf("*   - Choice: ");

  fgets(choice, STD_CHOICE_SIZE - 1, stdin);
  ch = atoi(choice);

  switch(ch)
  {
    case 0:
    case 1:
    case 2:
    case 3:
      increment = ssi_write_channel_th(message, ch, th);
      printf("*  - Set threshold of channel %d to %d\n", ch, th);
      break;
    case 9:
      increment = ssi_write_all_channel_th(message, th);
      printf("*  - Set all channels threshold to %d\n", th);
      break;
    default:
      printf("*  - Invalid choice\n");
      break;
  }

  return increment;
};

size_t configure_start_ch_freq(uint8_t *message)
{
  size_t increment = 0;
  char choice[STD_CHOICE_SIZE];

  uint16_t freq;

  printf("* - Insert start channel frequency (0-399): ");

  fgets(choice, STD_CHOICE_SIZE - 1, stdin);

  freq = atoi(choice);

  if(freq < 0 || freq > 399)
  {
    printf("*  - Invalid choice, must be between 0 and 399)\n");
  }
  else
  {
    increment = ssi_write_start_laser_ch(message, freq);
    printf("*  - Set start channel frequency to %d\n", freq);
  }

  return increment;
};

size_t configure_scan_speed(uint8_t *message)
{
  size_t increment = 0;
  char choice[STD_CHOICE_SIZE];

  int error = 0;

  uint8_t mode = 0;
  uint16_t speed = 0, cycle = 0;

  printf("* - Select scan speed mode:\n");
  printf("*   - 0: fixed steps\n");
  printf("*   - 1: variable steps\n");
  printf("*   - Choice: ");
  fgets(choice, STD_CHOICE_SIZE - 1, stdin);

  mode = atoi(choice);

  if(mode == 0)
  {
    printf("* - Select number of scan steps:\n");
    printf("*   - a: 400\n");
    printf("*   - b: 200\n");
    printf("*   - c: 100\n");
    printf("*   - d: 50\n");
    printf("*   - Choice: ");

    fgets(choice, STD_CHOICE_SIZE - 1, stdin);

    switch(choice[0])
    {
      case 'a':
        speed = 0;
        break;
      case 'b':
        speed = 1;
        break;
      case 'c':
        speed = 2;
        break;
      case 'd':
        speed = 3;
        break;
      default:
        printf("*   - Invalid choice\n");
        error = 1;
        break;
    }
  }
  else if(mode == 1)
  {
    printf("* - Insert number of scan steps (0-400): ");

    fgets(choice, STD_CHOICE_SIZE - 1, stdin);
    speed = atoi(choice);

    if(speed < 0 || speed > 400)
    {
      printf("*   - Invalid choice, must between 0 and 400)\n");
      error = 1;
    }
  }
  else
  {
    printf("*   - Invalid choice, must 0 or 1)\n");
    error = 1;
  }

  if(!error)
  {
    printf("* - Select cycle step:\n");
    printf("*   - a: 1 us\n");
    printf("*   - b: 2 us\n");
    printf("*   - c: 5 us\n");
    printf("*   - d: 10 us\n");
    printf("*   - e: 20 us\n");
    printf("*   - f: 50 us\n");
    printf("*   - Choice: ");

    fgets(choice, STD_CHOICE_SIZE - 1, stdin);

    switch(choice[0])
    {
      case 'a':
        cycle = 0;
        break;
      case 'b':
        cycle = 1;
        break;
      case 'c':
        cycle = 2;
        break;
      case 'd':
        cycle = 3;
        break;
      case 'e':
        cycle = 4;
        break;
      case 'f':
        cycle = 5;
        break;
      default:
        printf("*   - Invalid choice\n");
        error = 1;
        break;
    }
  }

  if(!error)
  {
    increment = ssi_write_scan_speed(message, mode, speed, cycle);
  }

  return increment;
};

size_t configure_ip_addr(uint8_t *message)
{
  size_t increment = 0;
  char choice[STD_CHOICE_SIZE];
  char *ptr;

  int index = 0;
  uint32_t ip_addr;
  uint8_t tmp, *ui_ptr = (uint8_t *) &(ip_addr);

  printf("* - Insert IP address: ");

  fgets(choice, STD_CHOICE_SIZE - 1, stdin);

  ptr = strtok(choice, ".");

  while(ptr && index < 4)
  {
    tmp = atoi(ptr);
    memset(ui_ptr + 4 - index - 1, tmp, 1);

    ptr = strtok(NULL, ".");

    index++;
  }

  if(index != 4)
  {
    printf("* - Invalid IP address format\n");
  }
  else
  {
    printf("* - The ip address is : %.8x\n", ip_addr);
    increment = ssi_write_ip_add(message, ip_addr);
  }

  return increment;
};

size_t configure_subnet(uint8_t *message)
{
  size_t increment = 0;
  char choice[STD_CHOICE_SIZE];
  char *ptr;

  int index = 0;
  uint32_t subnet;
  uint8_t tmp, *ui_ptr = (uint8_t *) &(subnet);

  printf("* - Insert subnet mask: ");

  fgets(choice, STD_CHOICE_SIZE - 1, stdin);

  ptr = strtok(choice, ".");

  while(ptr && index < 4)
  {
    tmp = atoi(ptr);
    memset(ui_ptr + 4 - index - 1, tmp, 1);

    ptr = strtok(NULL, ".");

    index++;
  }

  if(index != 4)
  {
    printf("* - Invalid subnet mask format\n");
  }
  else
  {
    printf("* - The subnet mask is : %.8x\n", subnet);
    increment = ssi_write_netmask(message, subnet);
  }

  return increment;
};

size_t configure_gateway(uint8_t *message)
{
  size_t increment = 0;
  char choice[STD_CHOICE_SIZE];
  char *ptr;

  int index = 0;

  uint32_t gateway;
  uint8_t *ui_ptr = (uint8_t *) &(gateway);

  uint8_t tmp;

  printf("* - Insert gateway address: ");

  fgets(choice, STD_CHOICE_SIZE - 1, stdin);

  ptr = strtok(choice, ".");

  while(ptr && index < 4)
  {
    tmp = atoi(ptr);
    memset(ui_ptr + 4 - index - 1, tmp, 1);

    ptr = strtok(NULL, ".");

    index++;
  }

  if(index != 4)
  {
    printf("* - Invalid gateway address format\n");
  }
  else
  {
    printf("* - The gateway is : %.8x\n", gateway);
    increment = ssi_write_gateway(message, gateway);
  }

  return increment;
};

size_t configure_generic_slot(uint8_t *message)
{
  size_t increment = 0;
  char choice[STD_CHOICE_SIZE];

  int error = 0;

  uint8_t channel = 0;
  uint8_t type = 0;
  uint8_t slot = 0;
  uint8_t data = 0;

  printf("* - Configure generic AGC slot:\n");

  printf("*   - Insert channel (0-3): ");
  fgets(choice, STD_CHOICE_SIZE - 1, stdin);

  channel = atoi(choice);

  if(channel > 3)
  {
    printf("*   - Invalid channel number (must be between 0 and 3)\n");
    error = 1;
  }

  if(!error)
  {
    printf("*   - Choose type:\n");
    printf("*   - 0: channel position\n");
    printf("*   - 1: gain\n");
    printf("*   - Choice: ");

    fgets(choice, STD_CHOICE_SIZE - 1, stdin);

    switch (choice[0])
    {
      case '0':
        type = 0;
        break;
      case '1':
        type = 1;
        break;
      default:
        printf("*   - Invalid type number (must be between 0 or 1)\n");
        error = 1;
        break;
    }
  }

  if(!error)
  {
    printf("*   - Insert slot (0-15): ");
    fgets(choice, STD_CHOICE_SIZE - 1, stdin);

    slot = atoi(choice);

    if(slot > 15)
    {
      printf("*   - Invalid slot number (must be between 0 and 15)\n");
      error = 1;
    }
  }

  if(!error)
  {
    printf("*   - Insert data (0-255)\n");
    printf("*   - (when indicating channel number, use channel / 2): ");
    fgets(choice, STD_CHOICE_SIZE - 1, stdin);

    data = atoi(choice);

    increment = ssi_write_gain_slot(message, channel, type, slot, data);
  }

  return increment;

};

size_t configure_single_slot(uint8_t *message)
{
  size_t increment = 0;
  char choice[STD_CHOICE_SIZE];

  int error = 0;

  uint8_t channel = 0;
  uint8_t slot = 0;
  uint16_t data = 0;

  printf("* - Configure single AGC slot:\n");

  printf("*   - Insert channel (0-3): ");
  fgets(choice, STD_CHOICE_SIZE - 1, stdin);

  channel = atoi(choice);

  if(channel > 3)
  {
    printf("*   - Invalid channel number (must be between 0 and 3)\n");
    error = 1;
  }

  if(!error)
  {
    printf("*   - Insert slot (0-15): ");
    fgets(choice, STD_CHOICE_SIZE - 1, stdin);

    slot = atoi(choice);

    if(slot > 15)
    {
      printf("*   - Invalid slot number (must be between 0 and 15)\n");
      error = 1;
    }
  }

  if(!error)
  {
    printf("*   - Insert data (0-400): ");
    fgets(choice, STD_CHOICE_SIZE - 1, stdin);

    data = atoi(choice);

    if(data > 400)
    {
      printf("*   - Invalid data (must be between 0 and 400)\n");
      error = 1;
    }
  }

  if(!error)
  {
    increment = ssi_write_gain_slot_single(message, channel, slot, data);
  }

  return increment;
};

size_t configure_agc(uint8_t *message)
{
  size_t increment = 0;

  int error = 0;

  char choice[STD_CHOICE_SIZE];
  uint16_t data;

  printf("*   - Insert AGC:\n");
  printf("*   - 0: disable AGC (use fixed gain slots)\n");
  printf("*   - 1-255: update rate (1 is fastest, main loop speed)\n");
  printf("*   - Choice: ");
  fgets(choice, STD_CHOICE_SIZE - 1, stdin);

  data = atoi(choice);

  if(data > 255)
  {
    printf("*   - Invalid data (must be between 0 and 255)\n");
    error = 1;
  }

  if(!error)
  {
    increment = ssi_write_agc(message, data);
  }

  return increment;
};

size_t configure_utc_local(uint8_t *message)
{
  return ssi_write_utc_local_time(message);
};

/*******************************************************************************
* main process
*******************************************************************************/
int main (int argc, char **argv){
  signal(SIGINT, sigint_handler);

  pthread_t send_tid = -1; // thread IDs
  void *result; // thread exit result

  printf("Configurator started\n");

  stop_process = 0; // TODO remove

  // create threads
  if((pthread_create(&send_tid, NULL, menu_th, NULL)) != 0)
  {
    printf("Unable to create menu thread\n");
    exit(1);
  }

  // joining threads (end)
  pthread_join(send_tid, &result);
  printf("Menu thread terminated\n");

  return 0;
}
