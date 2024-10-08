# Channel


## 相关链接

参考示例：
- {{ '[examples_py_protobuf_channel_publisher_app.py]({}/src/examples/py/protobuf_channel/examples_py_protobuf_channel_publisher_app.py)'.format(code_site_root_path_url) }}
- {{ '[examples_py_protobuf_channel_subscriber_app.py]({}/src/examples/py/protobuf_channel/examples_py_protobuf_channel_subscriber_app.py)'.format(code_site_root_path_url) }}


## protobuf 协议

协议用于确定通信各端的消息格式。一般来说，协议都是使用一种与具体的编程语言无关的 IDL ( Interface description language )描述，然后由某种工具转换为各个语言的代码。

[Protobuf](https://protobuf.dev/)是一种由 Google 开发的、用于序列化结构化数据的轻量级、高效的数据交换格式，是一种广泛使用的 IDL。

当前版本 AimRT Python 只支持 protobuf 协议。在使用 AimRT Python 发送/订阅消息之前，使用者需要先基于 protobuf 协议生成一些 python 的桩代码。


在使用时，开发者需要先定义一个`.proto`文件，在其中定义一个消息结构。例如`example.proto`：

```protobuf
syntax = "proto3";

message ExampleMsg {
  string msg = 1;
  int32 num = 2;
}
```

然后使用 Protobuf 官方提供的 protoc 工具进行转换，生成 Python 代码，例如：
```shell
protoc --python_out=. example.proto
```

这将生成`example_pb2.py`文件，包含了根据定义的消息类型生成的 Python 接口，我们的业务代码中需要 import 此文件。


## ChannelHandle

模块可以通过调用`CoreRef`句柄的`GetChannelHandle()`接口，获取`ChannelHandleRef`句柄，来使用 Channel 功能。其提供的核心接口如下：
- `GetPublisher(str)->PublisherRef`
- `GetSubscriber(str)->SubscriberRef`


开发者可以调用`ChannelHandleRef`中的`GetPublisher`方法和`GetSubscriber`方法，获取指定 Topic 名称的`PublisherRef`和`SubscriberRef`类型句柄，分别用于 Channel 发布和订阅。这两个方法使用注意如下：
  - 这两个接口是线程安全的。
  - 这两个接口可以在`Initialize`阶段和`Start`阶段使用。


## Publish

用户如果需要发布一个 Msg，牵涉的接口主要有以下两个：
- `aimrt_py.RegisterPublishType(publisher, msg_type)->bool` ： 用于注册此消息类型；
  - 第一个参数`publisher`是一个`PublisherRef`句柄，代表某个 Topic；
  - 第二个参数`msg_type`是一个`Protobuf`类型；
  - 返回值是一个 bool 值，表示注册是否成功；
- `aimrt_py.Publish(publisher, msg)` ： 用于发布消息；
  - 第一个参数`publisher`是一个`PublisherRef`句柄，代表某个 Topic；
  - 第二个参数`msg`是一个`Protobuf`类型实例，需要与注册时的消息类型对应；


用户需要两个步骤来实现逻辑层面的消息发布：
- **Step 1**：使用`aimrt_py.RegisterPublishType`方法注册协议类型；
  - 只能在`Initialize`阶段注册；
  - 不允许在一个`PublisherRef`中重复注册同一种类型；
  - 如果注册失败，会返回 false；
- **Step 2**：使用`aimrt_py.Publish`方法发布数据；
  - 只能在`Start`阶段之后发布数据；
  - 在调用`Publish`接口时，开发者应保证传入的 Msg 在`Publish`接口返回之前都不会发生变化，否则行为是未定义的；


用户`Publish`一个消息后，特定的 Channel 后端将处理具体的消息发布请求。此时根据不同后端的实现，有可能会阻塞一段时间，因此`Publish`方法耗费的时间是未定义的。但一般来说，Channel 后端都不会阻塞`Publish`方法太久，详细信息请参考对应后端的文档。


另外请注意，当前版本暂不支持 channel context 功能。


## Subscribe


用户如果需要订阅一个 Msg，需要使用以下接口：
- `aimrt_py.Subscribe(subscriber, msg_type, handle)->bool` ： 用于订阅一种消息;
  - 第一个参数`subscriber`是一个`SubscriberRef`句柄，代表某个 Topic；
  - 第二个参数`msg_type`是一个`Protobuf`类型；
  - 第三个参数`handle`是一个签名为`(msg)->void`的消息处理回调，`msg`类型是订阅时传入的`msg_type`类型；
  - 返回值是一个 bool 值，表示注册是否成功；

注意：
- 只能在`Initialize`调用订阅接口；
- 不允许在一个`SubscriberRef`中重复订阅同一种类型；
- 如果订阅失败，会返回 false；


此外还需要注意的是，由哪个执行器来执行订阅的回调，这和具体的 Channel 后端实现有关，在运行阶段通过配置才能确定，使用者在编写逻辑代码时不应有任何假设。详细信息请参考对应后端的文档。


最佳实践是：如果回调中的任务非常轻量，比如只是设置一个变量，那就可以直接在回调里处理；但如果回调中的任务比较重，那最好调度到其他专门执行任务的执行器里进行处理。

另外请注意，当前版本暂不支持 channel context 功能。

## 使用示例

以下是一个使用 AimRT Python 进行 Publish 的示例，通过 Create Module 方式拿到`CoreRef`句柄。如果是基于`Module`模式在`Initialize`方法中拿到`CoreRef`句柄，使用方式也类似：
```python
import aimrt_py
import threading

from google.protobuf.json_format import MessageToJson
import event_pb2

def main():
    aimrt_core = aimrt_py.Core()

    # Initialize
    core_options = aimrt_py.CoreOptions()
    core_options.cfg_file_path = "path/to/cfg/xxx_cfg.yaml"
    aimrt_core.Initialize(core_options)

    # Create Module
    module_handle = aimrt_core.CreateModule("NormalPublisherPyModule")

    # Register publish type
    topic_name = "test_topic"
    publisher = module_handle.GetChannelHandle().GetPublisher(topic_name)
    assert publisher, "Get publisher for topic '{}' failed.".format(topic_name)

    aimrt_py.RegisterPublishType(publisher, event_pb2.ExampleEventMsg)

    # Start
    thread = threading.Thread(target=aimrt_core.Start)
    thread.start()

    # Sleep for seconds
    time.sleep(1)

    # Publish event
    event_msg = event_pb2.ExampleEventMsg()
    event_msg.msg = "example msg"
    event_msg.num = 123456

    aimrt_py.info(module_handle.GetLogger(),
                  "Publish new pb event, data: {}".format(MessageToJson(event_msg)))

    aimrt_py.Publish(publisher, event_msg)

    # Sleep for seconds
    time.sleep(1)

    # Shutdown
    aimrt_core.Shutdown()

    thread.join()

if __name__ == '__main__':
    main()
```


以下是一个使用 AimRT Python 进行 Subscribe 的示例，通过 Create Module 方式拿到`CoreRef`句柄。如果是基于`Module`模式在`Initialize`方法中拿到`CoreRef`句柄，使用方式也类似：

```python
import aimrt_py
import threading

from google.protobuf.json_format import MessageToJson
import event_pb2

global_aimrt_core = None


def signal_handler(sig, frame):
    global global_aimrt_core

    if (global_aimrt_core and (sig == signal.SIGINT or sig == signal.SIGTERM)):
        global_aimrt_core.Shutdown()
        return

    sys.exit(0)


def main():
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    aimrt_core = aimrt_py.Core()

    global global_aimrt_core
    global_aimrt_core = aimrt_core

    # Initialize
    core_options = aimrt_py.CoreOptions()
    core_options.cfg_file_path = "path/to/cfg/xxx_cfg.yaml"
    aimrt_core.Initialize(core_options)

    # Create Module
    module_handle = aimrt_core.CreateModule("NormalSubscriberPyModule")

    # Subscribe
    topic_name = "test_topic"
    subscriber = module_handle.GetChannelHandle().GetSubscriber(topic_name)
    assert subscriber, "Get subscriber for topic '{}' failed.".format(topic_name)

    def EventHandle(msg):
        aimrt_py.info(module_handle.GetLogger(), "Get new pb event, data: {}".format(MessageToJson(msg)))

    aimrt_py.Subscribe(subscriber, event_pb2.ExampleEventMsg, EventHandle)

    # Start
    thread = threading.Thread(target=aimrt_core.Start)
    thread.start()

    while thread.is_alive():
        thread.join(1.0)


if __name__ == '__main__':
    main()
```

