#ifndef RFCOMMUNITY_HXX_
#define RFCOMMUNITY_HXX_

#include <iostream>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <termios.h>
#include <poll.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

class Rfcommunity {
  public:
    Rfcommunity(void);

    ~Rfcommunity(void);

    bool Connect(const char *dev_name, const char *remote_addr, int channel);
    int Bind(const char *dev_name, const char *remote_addr, int channel);
    int Release(const char *dev_name);
    int Show(const char *dev_name);
    int Listen(const char *dev_name, int channel);
    int Watch(const char *dev_name, int channel);

    int Disconnect(const char *remote_addr);

    bool isConnected();

  private:
    int dev_number;
    int rfcomm_raw_tty = 0;
    int auth = 0;
    int encryption = 0;
    int secure = 0;
    int master = 0;
    int linger = 0;
    int ctl;
    int file_descriptor;

    char *rfcomm_flagstostr(uint32_t flags)
    {
      static char str[100];
      str[0] = 0;
      strcat(str, "[");
      if (flags & (1 << RFCOMM_REUSE_DLC))
        strcat(str, "reuse-dlc ");
      if (flags & (1 << RFCOMM_RELEASE_ONHUP))
        strcat(str, "release-on-hup ");
      if (flags & (1 << RFCOMM_TTY_ATTACHED))
        strcat(str, "tty-attached");
      strcat(str, "]");
      return str;
    }

  private:
    bool is_connected = false;

};

#endif
