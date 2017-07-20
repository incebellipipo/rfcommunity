#include "rfcommunity.hxx"


Rfcommunity::Rfcommunity() {
  ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_RFCOMM);
  std::cout << "Rfcommunity construstor " << ctl << std::endl;
}

Rfcommunity::~Rfcommunity() {
  std::cout << "Rfcommunity destructor " << ctl << std::endl;
  close(ctl);
}

bool Rfcommunity::Connect(const char *dev_addr, const char *remote_addr, int channel) {

  struct sockaddr_rc laddr, raddr;
  struct rfcomm_dev_req req;
  struct termios ti;
  struct sigaction sa;
  struct pollfd p;
  sigset_t sigs;
  socklen_t alen;
  char dst[18], devname[MAXPATHLEN];
  int sk, file_descriptor, try_t = 30;

  // TODO experimental
  int linger = 1;


  // Device the you want to connect WITH.
  bdaddr_t bdaddr;
  str2ba(dev_addr, &bdaddr);
  laddr.rc_family = AF_BLUETOOTH;
  bacpy(&laddr.rc_bdaddr, &bdaddr);
  laddr.rc_channel = 0;

  // Device that you want to connect TO.
  raddr.rc_family = AF_BLUETOOTH;
  str2ba(remote_addr, &raddr.rc_bdaddr);
  raddr.rc_channel = channel;


  sk = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if (sk < 0) {
    perror("Can't create RFCOMM socket");
    return false;
  }
  if (linger) {
    struct linger l = { .l_onoff = 1, .l_linger = linger };
    if (setsockopt(sk, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) < 0) {
      perror("Can't set linger option");
      return false;
    }
  }
  if (bind(sk, (struct sockaddr *) &laddr, sizeof(laddr)) < 0) {
    perror("Can't bind RFCOMM socket");
    close(sk);
    return false;
  }
  if (connect(sk, (struct sockaddr *) &raddr, sizeof(raddr)) < 0) {
    perror("Can't connect RFCOMM socket");
    close(sk);
    return false;
  }
  alen = sizeof(laddr);
  if (getsockname(sk, (struct sockaddr *)&laddr, &alen) < 0) {
    perror("Can't get RFCOMM socket name");
    close(sk);
    return false;
  }
  memset(&req, 0, sizeof(req));
  req.dev_id = (int16_t)dev_number;
  req.flags = (1 << RFCOMM_REUSE_DLC) | (1 << RFCOMM_RELEASE_ONHUP);
  bacpy(&req.src, &laddr.rc_bdaddr);
  bacpy(&req.dst, &raddr.rc_bdaddr);
  req.channel = raddr.rc_channel;
  dev_number = ioctl(sk, RFCOMMCREATEDEV, &req);
  if (dev_number < 0) {
    perror("Can't create RFCOMM TTY");
    close(sk);
    return false;
  }
  snprintf(devname, MAXPATHLEN - 1, "/dev/rfcomm%d", dev_number);
  while ((file_descriptor = open(devname, O_RDONLY | O_NOCTTY)) < 0) {
    if (errno == EACCES) {
      perror("Can't open RFCOMM device");
    }
    snprintf(devname, MAXPATHLEN - 1, "/dev/bluetooth/rfcomm/%d", dev_number);
    if ((file_descriptor = open(devname, O_RDONLY | O_NOCTTY)) < 0) {
      if (try_t--) {
        snprintf(devname, MAXPATHLEN - 1, "/dev/rfcomm%d", dev_number);
        usleep(100 * 1000);
        continue;
      }
      perror("Can't open RFCOMM device");
    }
  }
  if (rfcomm_raw_tty) {
    tcflush(file_descriptor, TCIOFLUSH);
    cfmakeraw(&ti);
    tcsetattr(file_descriptor, TCSANOW, &ti);
  }
  close(sk);
  ba2str(&req.dst, dst);
  printf("Connected %s to %s on channel %d\n", devname, dst, req.channel);

  p.fd = file_descriptor;
  p.events = POLL_ERR | POLL_HUP;
  p.revents = 0;
  if(poll(&p, 1, NULL) > 0){
    std::cout << "something good happend but i don't know what" << std::endl;
    // todo you can check for what it is on following url
    // http://pubs.opengroup.org/onlinepubs/009695399/functions/poll.html
    return true;
  }
  return true;
}

bool Rfcommunity::Disconnect() {
    close(file_descriptor);
}

bool Rfcommunity::Release(const char *dev_addr) {

}

bool Rfcommunity::release_dev() {
  struct rfcomm_dev_req req;
  int err;

  memset(&req, 0, sizeof(req));
  req.dev_id = (int16_t) dev_number;

  if (ioctl(ctl, RFCOMMRELEASEDEV, &req) < 0){
    perror("Can't release device");
    return false;
  }
  return true;
}