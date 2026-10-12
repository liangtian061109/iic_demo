# Message Center

消息中心用于 APP 与 APP 之间的发布/订阅通信。它不是 OLED、I2C 等设备驱动的必需层。

```text
robot_cmd APP  --发布 chassis_cmd-->  message_center  --队列--> chassis APP
```

发布者和订阅者只需要约定话题名和数据结构，不需要互相包含头文件：

```c
typedef struct { float vx; float vy; } ChassisCommand_t;

MessagePublisher_t cmd_pub;
MessageSubscriberHandle_t chassis_sub;
MessageCenter_Init();
MessageCenter_Publisher_Register(&cmd_pub, "chassis_cmd", sizeof(ChassisCommand_t));
MessageCenter_Subscriber_Register(&chassis_sub, "chassis_cmd", sizeof(ChassisCommand_t));
```

发布者把一份数据复制到每个订阅者自己的队列；一个订阅者取走数据不会影响其他订阅者。
队列满时丢弃最旧的一份消息。这个示例使用固定大小静态池：最多 8 个话题、16 个订阅者、
每个订阅者 4 条消息、每条消息最多 64 字节，不使用 `malloc`。

`Examples/message_center_demo.c` 是两个 APP 的最小用法，未加入 OLED 的 CMake 构建。
示例实现适合单核、单生产者/单消费者的教学用途；真正接入 FreeRTOS 时，发布和取消息
需要在任务之间增加临界区、队列锁或直接使用 FreeRTOS queue。
