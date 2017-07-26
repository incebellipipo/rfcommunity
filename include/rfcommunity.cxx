#include "rfcommunity.hxx"



Rfcommunity::Rfcommunity() {
  m_ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_RFCOMM);
  m_is_connected = false;
}

Rfcommunity::~Rfcommunity() {
  close(m_ctl);
}

void Rfcommunity::setLocalAddr(const char *local_addr){
  str2ba(local_addr, &m_local_bdaddr);
  m_local_sockaddr_rc.rc_bdaddr = m_local_bdaddr;
  m_local_sockaddr_rc.rc_family = AF_BLUETOOTH;
  m_local_sockaddr_rc.rc_channel = 0;
  m_local_addr = std::string(local_addr);
}
void Rfcommunity::setRemoteAddr(const char *remote_addr){
  str2ba(remote_addr, &m_remote_bdaddr);
  m_remote_sockaddr_rc.rc_bdaddr = m_remote_bdaddr;
  m_remote_sockaddr_rc.rc_family = AF_BLUETOOTH;
  m_remote_sockaddr_rc.rc_channel = 1;
  m_remote_addr = std::string(remote_addr);
}
void Rfcommunity::setRemoteAddr(const char *remote_addr, uint8_t channel){
 setRemoteAddr(remote_addr);
  m_remote_addr = std::string(remote_addr);
}
void Rfcommunity::setChannel(int channel){m_remote_sockaddr_rc.rc_channel = channel;}


bool Rfcommunity::Connect() {
  if(m_remote_addr.empty() || m_local_addr.empty()){
    return false;
  }
  struct sockaddr_rc laddr, raddr;
  struct rfcomm_dev_req req;
  struct termios ti;
  struct sigaction sa;
  socklen_t alen;
  char dst[18], devname[MAXPATHLEN];
  int sk, fd, try_t = 30;

  laddr = m_local_sockaddr_rc;
  raddr = m_remote_sockaddr_rc;

  sk = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

  if (sk < 0) {
    perror("Can't create RFCOMM socket");
    return false;
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
  m_dev = 0;
  req.dev_id = m_dev;
  req.flags = (1 << RFCOMM_REUSE_DLC) | (1 << RFCOMM_RELEASE_ONHUP);
  bacpy(&req.src, &m_local_sockaddr_rc.rc_bdaddr);
  bacpy(&req.dst, &m_remote_sockaddr_rc.rc_bdaddr);
  req.channel = m_remote_sockaddr_rc.rc_channel;
  m_dev = ioctl(sk, RFCOMMCREATEDEV, &req);
  if (m_dev < 0) {
    perror("Can't create RFCOMM TTY");
    if(errno == EADDRINUSE){
      while(m_dev < 0){
        req.dev_id = req.dev_id + 1;
        m_dev = ioctl(sk,RFCOMMCREATEDEV, &req);
      }
    }
    close(sk);
    return false;
  }
  snprintf(devname, MAXPATHLEN - 1, "/dev/rfcomm%d", m_dev);
  while ((m_file_descriptor = open(devname, O_RDONLY | O_NOCTTY)) < 0) {
    if (errno == EACCES) {
      perror("Can't open RFCOMM device");
      memset(&req, 0, sizeof(req));
      req.dev_id = m_dev;
      req.flags = (1 << RFCOMM_HANGUP_NOW);
      ioctl(m_ctl, RFCOMMRELEASEDEV, &req);
      close(sk);
    }
    snprintf(devname, MAXPATHLEN - 1, "/dev/bluetooth/rfcomm/%d", m_dev);
    if ((m_file_descriptor = open(devname, O_RDONLY | O_NOCTTY)) < 0) {
      if (try_t--) {
        snprintf(devname, MAXPATHLEN - 1, "/dev/rfcomm%d", m_dev);
        usleep(100 * 1000);
        continue;
      }
      perror("Can't open RFCOMM device");
      memset(&req, 0, sizeof(req));
      req.dev_id = m_dev;
      req.flags = (1 << RFCOMM_HANGUP_NOW);
      ioctl(m_ctl, RFCOMMRELEASEDEV, &req);
      close(sk);
    }
  }
  if (m_rfcomm_raw_tty) {
    tcflush(m_file_descriptor, TCIOFLUSH);
    cfmakeraw(&ti);
    tcsetattr(m_file_descriptor, TCSANOW, &ti);
  }
  close(sk);
  ba2str(&req.dst, dst);
  printf("Connected %s to %s on channel %d\n", devname, dst, req.channel);
  printf("Disconnected\n");
  close(m_file_descriptor);
  return true;
}
bool Rfcommunity::Connect(const char *local_addr, const char *remote_addr, int channel){
  setLocalAddr(local_addr);
  setRemoteAddr(remote_addr,channel);
  return (m_is_connected = Connect());
}

bool Rfcommunity::Bind(){
  if(m_local_addr.empty() || m_remote_addr.empty()){
    return false;
  }
  m_is_binded = f_create_dev();
  m_is_released != m_is_binded;
  return m_is_binded;
}
bool Rfcommunity::Bind(const char *local_addr, const char *remote_addr, int channel){
  setLocalAddr(local_addr);
  setRemoteAddr(remote_addr,channel);
  return (m_is_binded = Bind());
}

bool Rfcommunity::Release(int dev){
  return f_release_dev(dev);
}

bool Rfcommunity::Release(){
  m_is_released = f_release_dev();
  m_is_binded != m_is_released;
  return m_is_released;
}

bool Rfcommunity::Show(const char *dev_name){
  f_cmd_show(dev_name);
}

bool Rfcommunity::Show(int16_t dev){
  f_cmd_show(dev);
}

bool Rfcommunity::Show(){
  f_print_dev_list();
}

int Rfcommunity::f_create_dev(){
  struct rfcomm_dev_req req;
  int err;
  memset(&req, 0, sizeof(req));
  req.dev_id = m_dev;
  req.flags = 0;
  bacpy(&req.src, &m_local_bdaddr);

  str2ba(m_remote_addr.c_str(), &req.dst);
  req.channel = m_remote_sockaddr_rc.rc_channel;
  err = ioctl(m_ctl, RFCOMMCREATEDEV, &req);
  if (err == -1) {
    err = -errno;
    if (err == -EOPNOTSUPP)
      fprintf(stderr, "RFCOMM TTY support not available\n");
    else
      perror("Can't create device");
  }
  return err;
}
int Rfcommunity::f_release_dev(){
  struct rfcomm_dev_req req;
  int err;
  memset(&req, 0, sizeof(req));
  req.dev_id = m_dev;
  err = ioctl(m_ctl,RFCOMMRELEASEDEV, &req);
  if(err < 0){
    throw std::runtime_error(std::string("Can't release device ") + strerror(errno));
  }
  return err;
}
int Rfcommunity::f_release_dev(int dev){
  struct rfcomm_dev_req req;
  int err;
  memset(&req, 0, sizeof(req));
  req.dev_id = dev;
  err = ioctl(m_ctl,RFCOMMRELEASEDEV, &req);
  if(err < 0){
    throw std::runtime_error(std::string("Can't release device ") + strerror(errno));
  }
  return err;
}
int Rfcommunity::f_release_all(){
  struct rfcomm_dev_list_req *dl;
  struct rfcomm_dev_info *di;
  int i;
  dl = (rfcomm_dev_list_req*) malloc(sizeof(*dl) + RFCOMM_MAX_DEV * sizeof(*di));
  if (!dl) {
    perror("Can't allocate memory");
    exit(1);
  }
  dl->dev_num = RFCOMM_MAX_DEV;
  di = dl->dev_info;
  if (ioctl(m_ctl, RFCOMMGETDEVLIST, (void *) dl) < 0) {
    perror("Can't get device list");
    free(dl);
    exit(1);
  }
  for (i = 0; i < dl->dev_num; i++){
    f_release_dev((di + i)->id);
  }
  free(dl);
  return 0;
}

std::string Rfcommunity::rfcomm_flagstostr(uint32_t flags)
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
    return std::string(str);
}

void Rfcommunity::f_print_dev_info(rfcomm_dev_info *di){
    char src[18], dst[18], addr[40];

    ba2str(&di->src, src); ba2str(&di->dst, dst);

    if (bacmp(&di->src, &m_local_bdaddr) == 0){
        sprintf(addr, "%s", dst);
    }
    else{
        sprintf(addr, "%s -> %s", src, dst);
    }
    printf("rfcomm%d: %s channel %d %s %s\n",
        di->id, addr, di->channel,
        rfcomm_state[di->state].c_str(),
        di->flags ? rfcomm_flagstostr(di->flags).c_str() : "");
}

bool Rfcommunity::f_print_dev_list(){
  struct rfcomm_dev_list_req *dl;
  struct rfcomm_dev_info *di;
  int i;

  dl = (rfcomm_dev_list_req *) malloc(sizeof(*dl) + RFCOMM_MAX_DEV * sizeof(*di));
  if(!dl){
    perror("Can't allocate memory");
    return false;
  }
  dl->dev_num = RFCOMM_MAX_DEV;
  di = dl->dev_info;

  if(ioctl(m_ctl, RFCOMMGETDEVLIST, (void *) dl) < 0 ){
    perror("Can't get device list");
    free(dl);
    return false;
  }
  for( i = 0; i < dl->dev_num; i++){
    f_print_dev_info(di + i);
  }
  free(dl);
  return true;
}

bool Rfcommunity::f_cmd_show(int16_t dev){
    struct rfcomm_dev_info di = { .id = dev };
    if(ioctl(m_ctl, RFCOMMGETDEVINFO, &di) < 0){
        perror("Get info failed");
        return false;
    }
    f_print_dev_info(&di);
    return true;
}

bool Rfcommunity::f_cmd_show(const char *dev_addr){
    struct rfcomm_dev_info di;
    bdaddr_t bdaddr;
    str2ba(dev_addr, &bdaddr);
    di.dst = bdaddr;
    if(ioctl(m_ctl, RFCOMMGETDEVINFO, &di) < 0){
        perror("Get info failed");
        return false;
    }
    f_print_dev_info(&di);
    return true;
}
