/* 这是一个不参与 OLED 工程编译的示例，展示两个 APP 如何通过话题通信。 */
#include "message_center.h"

typedef struct {
    float vx;
    float vy;
} ChassisCommand_t;

static MessagePublisher_t cmd_publisher;
static MessageSubscriberHandle_t chassis_subscriber;

void MessageCenter_Demo_Init(void)
{
    MessageCenter_Init();
    MessageCenter_Publisher_Register(&cmd_publisher, "chassis_cmd",
                                     sizeof(ChassisCommand_t));
    MessageCenter_Subscriber_Register(&chassis_subscriber, "chassis_cmd",
                                      sizeof(ChassisCommand_t));
}

void RobotCmd_App_Run(void)
{
    ChassisCommand_t command = {.vx = 1.0f, .vy = 0.0f};
    MessageCenter_Publish(&cmd_publisher, &command);
}

void Chassis_App_Run(void)
{
    ChassisCommand_t command;
    if (MessageCenter_Subscribe_Get(&chassis_subscriber, &command)) {
        /* 根据 command.vx / command.vy 计算并设置底盘目标。 */
    }
}
