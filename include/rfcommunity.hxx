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

    bool Connect(const char *local_addr, const char *remote_addr, int channel);
    bool Connect();

    bool Bind(const char *local_addr, const char *remote_addr, int channel);
    bool Bind();

    bool Release(int dev);
    bool Release();

    // This three methods implemented in future
    bool Show(const char *dev_name);
    bool Show(int16_t dev);
    bool Show();

    int Listen(const char *dev_name, int channel);
    int Watch(const char *dev_name, int channel);


    void setLocalAddr(const char *local_addr);
    void setRemoteAddr(const char *remote_addr);
    void setRemoteAddr(const char *remote_addr, uint8_t channel);
    void setChannel(int channel);

    int getDev(){return m_dev;}
    bool isReleased(){return m_is_released;}
    bool isConnected(){return m_is_connected;}
    bool isBinded(){return m_is_binded;}

    bool removeDuplicates();

    std::string getRemoteAddr(){return m_remote_addr;}
    std::string getLocalAddr(){return m_local_addr;}

  private:
    char m_devname[MAXPATHLEN];
    int  m_rfcomm_raw_tty = 0;
    int  m_ctl;
    int  m_file_descriptor;


  private:
    bool m_is_connected = false;
    bool m_is_binded    = false;
    bool m_is_released  = false;

    uint16_t m_dev = 0;
    bdaddr_t m_local_bdaddr;
    bdaddr_t m_remote_bdaddr;
    struct sockaddr_rc m_local_sockaddr_rc;
    struct sockaddr_rc m_remote_sockaddr_rc;

    std::string m_local_addr;
    std::string m_remote_addr;

    std::string *rfcomm_state = new std::string[10]{
        "unknown",
        "connected",
        "clean",
        "bound",
        "listening",
        "connecting",
        "connecting",
        "config",
        "disconnecting",
        "closed"
    };

    int f_create_dev();
    int f_release_dev();
    int f_release_dev(int dev);
    int f_release_all();
    bool f_cmd_show(int16_t dev);
    bool f_cmd_show(const char *dev_name);

    // TODO Connect Method
    void f_cmd_connect(int ctl, int dev, bdaddr_t *bdaddr, int argc, char **argv);
    // TODO Listen Method
    void f_cmd_listen(int ctl, int dev, bdaddr_t *bdaddr, int argc, char **argv);
    // TODO Watch Method
    void f_cmd_watch(int ctl, int dev, bdaddr_t *bdaddr, int argc, char **argv);
    // TODO Bind Method
    void f_cmd_create(int ctl, int dev, bdaddr_t *bdaddr, int argc, char **argv);
    // TODO Release Method
    void f_cmd_release(int ctl, int dev, bdaddr_t *bdaddr, int argc, char **argv);

    //List devices
    bool f_print_dev_list();
    //Print device info
    void f_print_dev_info(struct rfcomm_dev_info *di);

    std::string rfcomm_flagstostr(uint32_t flags);


};

#endif
