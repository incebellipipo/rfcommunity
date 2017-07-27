#include <iostream>
#include <rfcommunity.hxx>

int main() {

  Rfcommunity rfcommunity;
  rfcommunity.setRemoteAddr("00:06:66:6E:B5:A0");

  rfcommunity.removeDuplicates();
  return 0;
}
