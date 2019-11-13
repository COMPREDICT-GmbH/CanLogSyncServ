# CanLogSyncServ
`CanLogSyncServ` is a small and lightweight tool for providing a easy accessable interface to DBC CAN signals by an user defined
signal ID over a IPC protocol based on ZeroMQ and protobuf. For this the tool is optimized for performance (not perfectly optimized yet).


The IPC protocol is using the ZeroMQ Publisher/Subscriber pattern in combination with protobuf:
![alt text](https://github.com/imatix/zguide/raw/master/images/fig4.png "Publisher/Subscriber pattern")

For this reason the ZeroMQ and protobuf libraries are needed to receive data from the IPC sockets.

## Usage
Say this DBC is given:

network.dbc:
```
VERSION ""
NS_ : 
	NS_DESC_
	CM_
	BA_DEF_
	BA_
	VAL_
	CAT_DEF_
	CAT_
	FILTER
	BA_DEF_DEF_
	EV_DATA_
	ENVVAR_DATA_
	SGTYPE_
	SGTYPE_VAL_
	BA_DEF_SGTYPE_
	BA_SGTYPE_
	SIG_TYPE_REF_
	VAL_TABLE_
	SIG_GROUP_
	SIG_VALTYPE_
	SIGTYPE_VALTYPE_
	BO_TX_BU_
	BA_DEF_REL_
	BA_REL_
	BA_DEF_DEF_REL_
	BU_SG_REL_
	BU_EV_REL_
	BU_BO_REL_
	SG_MUL_VAL_

BS_:

BU_:

BO_ 1 Msg_0: 4 
 SG_ Sig_0 : 7|32@0+ (1,0) [0|4294967295] ""  Receiver

BO_ 2 Msg_1: 3
 SG_ Sig_0 : 12|3@1+ (1,0) [0|7] ""  Receiver
 SG_ Sig_1 : 16|2@1+ (1,0) [0|3] ""  Receiver

BO_ 3 Msg_2: 3
 SG_ Sig_0 : 12|2@1+ (1,0) [0|0] ""  Receiver
 SG_ Sig_1 : 16|3@1+ (1,0) [0|0] ""  Receiver
 SG_ Sig_2 : 14|2@1+ (1,0) [0|0] ""  Receiver
```
And we want log Msg_0::Sig_0, Msg_1::Sig_1, Msg_2::Sig_0, Msg_2::Sig_2.

Therefor we have either can start the `CanLogSyncServ`-server by specifiying the signals in the command line:
```
CanLogSyncServ --iface=vcan0 --sample_rate=5000 --ipc_link=ipc:///tmp/network.ipc --ipc_link=tcp://*:5556 --dbc=network.dbc --signal="1;Sig_0;0" "2;Sig_1;1" "3;Sig_0;2" "3;Sig_2;3"
```
or by creating a config file that we can pass to the `CanLogSyncServ`-server:

config.cfg:
```
# Message ID;Signal Name;Signal ID
1;Sig_0;0
2;Sig_1;1
3;Sig_0;2
3;Sig_2;3
```
And run:
```
CanLogSyncServ --iface=vcan0 --sample_rate=5000 --ipc_link=ipc:///tmp/network.ipc --ipc_link=tcp://*:5556 --dbc=network.dbc --config=config.cfg
```
The `CanLogSyncServ` is now broadcasting the physical signal values over those two IPC sockets.

> The `Sync` in `CanLogSyncServer` stands for synchronizing, this means,
> the tool is atomic braodcasting all the actual frames which belong into one sampe_rate interval.
> So if you receive something about a IPC socket, you can be sure, that **exactly** `sample_rate` microseconds passed between the last received data.
> So there is absolutly no synchronizing mechanism needed to be implemented by the user of this tool.

Now we can subscribe to those IPC sockets like this:

C++
```
#include <zmq.hpp>
#include <iostream>
#include <sstream>
#include "Signal.pb.h"
struct Signal
{
    uint64_t id;
    double value;
};
int main (int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, ZMQ_SUB);
    // either
    subscriber.connect("ipc:///tmp/network.ipc");
    // or
    // subscriber.connect("tcp://localhost:5556");
    subscriber.setsockopt(ZMQ_SUBSCRIBE, nullptr, 0);
    while (1)
    {
        zmq::message_t update;
        subscriber.recv(&update);
        char* data = static_cast<char*>(update.data());
        CanLogSyncServ::Pb_Signals sigs;
        membuf mb{data, data + update.size() + 1};
        std::istream is{&mb};
        sigs.ParseFromIstream(&is);
        std::cout << "(" << sigs.timestamp() << ") Received data:" << std::endl;
        for (std::size_t i = 0; i < sigs.sigs_size(); i++)
        {
            const auto& sig = sigs.sigs(i);
            std::cout << "id=" << sig.id() << " value=" << sig.value() << std::endl;
        }
    }
    return 0;
}
```
or the python3 way:
```
import zmq
import Signal_pb2

context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.connect("ipc:///tmp/weather.ipc")
socket.setsockopt_string(zmq.SUBSCRIBE, "")
while True:
    data = socket.recv()
    sigs = Signal_pb2.Pb_Signals()
    sigs.ParseFromString(data)
    print("(" + str(sigs.timestamp) + ") Received data:")
    for sig in sigs.sigs:
        print("id=" + str(sig.id) + " value=" + str(sig.value))

```
There are also many other bindings for other languages to ZeroMQ and protobuf. For more informations look at the official site of [ZeroMQ](https://zeromq.org/get-started/) and [protobuf](https://github.com/protocolbuffers/protobuf).

## Build
### Dependencies
  * [libzmq](https://github.com/zeromq/libzmq)
  * [cppzmq](https://github.com/zeromq/cppzmq)
  * [Vector_DBC](https://bitbucket.org/tobylorenz/vector_dbc/src/master/)
  * [Boost program_options](https://www.boost.org/)
  * [protobuf](https://github.com/protocolbuffers/protobuf)
  
### Unix
```
git clone https://github.com/xR3b0rn/CanLogSyncServ.git
mkdir CanLogSyncServ/build
cd CanLogSyncServ/build
ccmake ..
make
make install
```

### Windows
Not supported yet.

## TODO
  * Consider byte order (maybe using protobuf)

