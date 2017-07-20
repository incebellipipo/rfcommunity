#include "rfcommunity.hxx"


Rfcommunity::Rfcommunity() {
  ctl_ = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_RFCOMM);
  std::cout << "Rfcommunity construstor " << ctl_ << std::endl;
}

Rfcommunity::~Rfcommunity() {
  std::cout << "Rfcommunity destructor " << ctl_ << std::endl;
  close(ctl_);
}

bool Rfcommunity::Connect(const char *dev_addr, const char *remote_addr, int channel) {

  struct sockaddr_rc laddr, raddr;
  struct rfcomm_dev_req req;
  struct termios ti;
  struct sigaction sa;
  sigset_t sigs;
  socklen_t alen;
  char dst[18], devname[MAXPATHLEN];
  int try_t = 30;


  // Device the you want to connect WITH.
  bdaddr_t bdaddr;
  str2ba(dev_addr, &bdaddr);
  laddr.rc_family = AF_BLUETOOTH;
  bacpy(&laddr.rc_bdaddr, &bdaddr);
  laddr.rc_channel = 0;

  // Device that you want to connect TO.
  raddr.rc_family = AF_BLUETOOTH;
  str2ba(remote_addr, &raddr.rc_bdaddr);
  raddr.rc_channel = (uint8_t )channel;


  socket_conn_ = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if (socket_conn_ < 0) {
    perror("Can't create RFCOMM socket");
    return false;
  }
  if (bind(socket_conn_, (struct sockaddr *) &laddr, sizeof(laddr)) < 0) {
    perror("Can't bind RFCOMM socket");
    close(socket_conn_);
    return false;
  }
  if (connect(socket_conn_, (struct sockaddr *) &raddr, sizeof(raddr)) < 0) {
    perror("Can't connect RFCOMM socket");
    close(socket_conn_);
    return false;
  }
  connect_val_ = socket_conn_;
  alen = sizeof(laddr);
  if (getsockname(socket_conn_, (struct sockaddr *)&laddr, &alen) < 0) {
    perror("Can't get RFCOMM socket name");
    close(socket_conn_);
    return false;
  }

  int ioctl_result;
  memset(&current_dev_req_, 0, sizeof(current_dev_req_));
  current_dev_req_.dev_id = (int16_t)dev_number_;
  current_dev_req_.flags = (1 << RFCOMM_REUSE_DLC) | (1 << RFCOMM_RELEASE_ONHUP);
  bacpy(&current_dev_req_.src, &laddr.rc_bdaddr);
  bacpy(&current_dev_req_.dst, &raddr.rc_bdaddr);
  current_dev_req_.channel = raddr.rc_channel;
  ioctl_result = ioctl(socket_conn_, RFCOMMCREATEDEV, &current_dev_req_);

  if (ioctl_result < 0) {
    if( errno == EADDRINUSE){
      std::cout << "WARN : Can't create RFCOMM TTY " << strerror(errno) << std::endl;
      while(ioctl_result < 0){
        dev_number_++;
        std::cout << "INFO : Trying to connect to port: " << dev_number_ << "." << std::endl;
        current_dev_req_.dev_id = (int16_t)dev_number_;
        ioctl_result = ioctl(socket_conn_, RFCOMMCREATEDEV, &current_dev_req_);
      }

    }
    return false;
  }
  dev_number_ = ioctl_result;
  snprintf(devname_, MAXPATHLEN - 1, "/dev/rfcomm%d", dev_number_);
  while ((file_descriptor_ = open(devname_, O_RDONLY | O_NOCTTY)) < 0) {
    if (errno == EACCES) {
      std::cout << "Can't open RFCOMM device " << strerror(errno) << std::endl;
      Disconnect();
      release_dev_(dev_number_);
      return false;
    }
    snprintf(devname_, MAXPATHLEN - 1, "/dev/bluetooth/rfcomm/%d", dev_number_);
    if ((file_descriptor_ = open(devname_, O_RDONLY | O_NOCTTY)) < 0) {
      if (try_t--) {
        snprintf(devname_, MAXPATHLEN - 1, "/dev/rfcomm%d", dev_number_);
        usleep(100 * 1000);
        std::cout << try_t << std::endl;
        continue;
      }
      std::cout << "Can't open RFCOMM device " << strerror(errno) << std::endl;
      Disconnect();
      release_dev_(dev_number_);
      return false;
    }
  }
  if (rfcomm_raw_tty_) {
    tcflush(file_descriptor_, TCIOFLUSH);
    cfmakeraw(&ti);
    tcsetattr(file_descriptor_, TCSANOW, &ti);
  }
  close(socket_conn_);
  ba2str(&current_dev_req_.dst, dst);
  printf("Connected %s to %s on channel %d\n", devname_, dst, current_dev_req_.channel);
  is_connected_ = true;
  return true;
}

// TODO not finished
bool Rfcommunity::Disconnect() {
  struct pollfd p;
  p.fd = file_descriptor_;
  p.events = POLL_ERR | POLL_HUP;
  p.revents = 0;

  // todo set timeout is weird;
  int time_out = 10;
  if(poll(&p, 1, time_out) > 0){
    // todo you can check for what it is on following url
    // http://pubs.opengroup.org/onlinepubs/009695399/functions/poll.html
    is_connected_ = false;
    return true;
  }
  return false;
}

bool Rfcommunity::Release() {
  try {
    release_all_();
  } catch (std::runtime_error e){
    std::cout << e.what() << std::endl;
    return false;
  }
  return true;
}

bool Rfcommunity::Release(int dev_id) {
  release_dev_(dev_id);
}

bool Rfcommunity::release_dev_(int dev) {
//  if(!is_connected_){
//    throw std::runtime_error(std::string("Can not release a port which is not connected."));
//  }
//  struct rfcomm_dev_req req = current_dev_req_;
  struct rfcomm_dev_req req;

  memset(&req, 0, sizeof(req));
  req.dev_id = (int16_t) dev;

  if (ioctl(ctl_, RFCOMMRELEASEDEV, &req) < 0){
    throw std::runtime_error(std::string("Can't release device: 180: ") + strerror(errno));
  }
  if(close(file_descriptor_) < 0){
    throw std::runtime_error(std::string("Can't release device: 183: ") + strerror(errno));
  }
  return true;
}

bool Rfcommunity::release_all_() {

  struct rfcomm_dev_list_req *dl;
  struct rfcomm_dev_info *di;
  int i;
  dl = (rfcomm_dev_list_req *) malloc(sizeof(*dl) + RFCOMM_MAX_DEV * sizeof(*di));
  if (!dl) {
    throw std::runtime_error(std::string("Can't allocate memory") + strerror(errno));
  }
  dl->dev_num = RFCOMM_MAX_DEV;
  di = dl->dev_info;
  if (ioctl(ctl_, RFCOMMGETDEVLIST, (void *) dl) < 0) {
    free(dl);
    throw std::runtime_error(std::string("Can't get device list ") + strerror(errno));
  }
  for (i = 0; i < dl->dev_num; i++)
    release_dev_((di + i)->id);
  free(dl);
  return true;

}

// TODO not finished
bool Rfcommunity::Bind(const char *dev_addr, const char *remote_addr, int channel) {
  std::cout << "Bind method " << std::endl;

  struct rfcomm_dev_req req;
  int err;
  bdaddr_t bdaddr;
  str2ba(dev_addr, &bdaddr);

  memset(&req, 0, sizeof(req));
  req.dev_id = dev_number_;
  req.flags = (1 << RFCOMM_REUSE_DLC) | (1 << RFCOMM_RELEASE_ONHUP);
  bacpy(&req.src, &bdaddr);

  str2ba(remote_addr, &req.dst);

  req.channel = channel;
  err = ioctl(ctl_, RFCOMMCREATEDEV, &req);
  if (err == -1) {
    err = -errno;
    if (err == -EOPNOTSUPP)
      fprintf(stderr, "RFCOMM TTY support not available\n");
    else
      perror("Can't create device");
  }
  return err;
}

bool Rfcommunity::isConnected() {return is_connected_;}