#ifndef RFCOMMUNITY_HXX_
#define RFCOMMUNITY_HXX_

#include <iostream>

namespace rfcommunity {
    class Rfcommunity {
    public:
        Rfcommunity(void);

        ~Rfcommunity(void);

        int Connect(const char *dev_name, const char *remote_addr, int channel);

        int Bind(const char *dev_name, const char *remote_addr, int channel);

        int Release(const char *dev_name);

        int Show(const char *dev_name);

        int Listen(const char *dev_name, int channel);

        int Watch(const char *dev_name, int channel);

        int Disconnect(const char *remote_addr);

    private:


    };
}
#endif
