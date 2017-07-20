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

    bool Connect(const char *dev_addr, const char *remote_addr, int channel);

    bool Bind(const char *dev_name, const char *remote_addr, int channel);

    bool Release(int dev_id);
    bool Release();

    int Show(const char *dev_name);

    int Listen(const char *dev_name, int channel);

    int Watch(const char *dev_name, int channel);

    bool Disconnect();

    bool isConnected();

  private:
    int dev_number_ = 0;
    char devname_[MAXPATHLEN];
    int rfcomm_raw_tty_ = 1;

    int s_socket_conn_;
    int c_socket_conn_;
    int b_socket_conn_;
    int fd_socket_conn_;

    int socket_conn_;


    int connect_val_;
    int auth = 0;
    int encryption = 0;
    int secure = 0;
    int master = 0;
    int linger = 0;
    int ctl_;
    int file_descriptor_;
    struct rfcomm_dev_req current_dev_req_;


  private:
    bool is_connected_ = false;
    bool release_dev_(int dev);
    bool release_all_(void);

};

#endif
