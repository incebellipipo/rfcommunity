#include "rfcommunity.hxx"



Rfcommunity::Rfcommunity() {
  ctl_ = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_RFCOMM);
}

Rfcommunity::~Rfcommunity() {
  close(ctl_);
}

bool Rfcommunity::Connect(const char *dev_addr, const char *remote_addr, int channel) {

  struct sockaddr_rc laddr, raddr;
  struct rfcomm_dev_req req; // TODO Not used
  struct termios ti;
  socklen_t alen;
  char dst[18];
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
    throw std::runtime_error("Can't create RFCOMM socket "+strerror(errno));
  }
  if (bind(socket_conn_, (struct sockaddr *) &laddr, sizeof(laddr)) < 0) {
    close(socket_conn_);
    throw std::runtime_error("Can't bind RFCOMM socket "+strerror(errno));
  }
  if (connect(socket_conn_, (struct sockaddr *) &raddr, sizeof(raddr)) < 0) {
    close(socket_conn_);
    throw std::runtime_error("Can't connect RFCOMM socket" + strerror(errno));
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
    // return false;
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
//  return true;

  // TODO this poll function used in standart rfcomm tool
  struct pollfd p;
  p.fd = file_descriptor_;
  p.events = POLL_ERR | POLL_HUP;
  p.revents = 0;
  if(poll(&p, 1, NULL) > 0){
    // todo you can check for what it is on following
    // man poll
  }
  return true;
}

// TODO not finished
bool Rfcommunity::Disconnect() {
  if(close(file_descriptor_) < 0){
    throw std::runtime_error(std::string("Can't disconnect device: ") + strerror(errno));
  }
  is_connected_ = false;
  return true;
}

bool Rfcommunity::Release() {
  try {
    release_all_();
  } catch (std::runtime_error e){
    std::cout << "WARN : Can't release device:" << e.what() << std::endl;
    std::cout << "INFO : Trying to release current device with id: " << current_dev_req_.dev_id << std::endl;
    release_dev_(current_dev_req_.dev_id);
    return false;
  }
  return true;
}

bool Rfcommunity::Release(int dev_id) {
  release_dev_(dev_id);
}

bool Rfcommunity::release_dev_(int dev) {
  struct rfcomm_dev_req req;
  req.dev_id = (int16_t) dev;
  memset(&req, 0, sizeof(req));
  if (ioctl(ctl_, RFCOMMRELEASEDEV, &req) < 0){
    throw std::runtime_error(strerror(errno));
  }
  is_connected_ = false;
  return true;
}

bool Rfcommunity::release_all_() {
  struct rfcomm_dev_list_req *dl;
  struct rfcomm_dev_info *di;
  int i;
  dl = (rfcomm_dev_list_req *) malloc(sizeof(*dl) + RFCOMM_MAX_DEV * sizeof(*di));
  if (!dl) {
    throw std::runtime_error(strerror(errno));
  }
  dl->dev_num = RFCOMM_MAX_DEV;
  di = dl->dev_info;
  if (ioctl(ctl_, RFCOMMGETDEVLIST, (void *) dl) < 0) {
    free(dl);
    throw std::runtime_error(strerror(errno));
  }
  for (i = 0; i < dl->dev_num; i++) {
    try{
      release_dev_((di + i)->id);
    } catch (std::runtime_error e){
      std::cout << "ERROR: Cant' release device with id: " << (di+i)->id << " caused: " << e.what() << std::endl;
      continue;
    }
    std::cout << (di+i)->id << std::endl;
  }
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