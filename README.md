# Rfcomm command line tool C++ binding

Hi to all. This is some silly approach to use rfcomm command line program in another purposes. 
Rfcomm methods are simplified like;
- `rfcommunity.Connect(host_bdaddr, remote_bdaddr, channel)`
- `rfcommunity.Bind(host_bdaddr, remote_bdaddr,channel)`
- `rfcommunity.Release()` and `rfcommunity.Release(dev_port)`

And obviously its currently under development. For now don't expect something great.