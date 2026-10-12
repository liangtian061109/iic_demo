#ifndef MESSAGE_CENTER_H
#define MESSAGE_CENTER_H

#include <stdbool.h>
#include <stdint.h>

#define MESSAGE_CENTER_MAX_TOPICS 8
#define MESSAGE_CENTER_MAX_SUBSCRIBERS 16
#define MESSAGE_CENTER_QUEUE_DEPTH 4
#define MESSAGE_CENTER_TOPIC_NAME_SIZE 24
#define MESSAGE_CENTER_MAX_DATA_SIZE 64

typedef struct MessageTopic_s MessageTopic_t;
typedef struct MessageSubscriber_s MessageSubscriber_t;

/* APP 发布者只保存一个话题句柄；话题的存储由消息中心管理。 */
typedef struct {
    MessageTopic_t *topic;
} MessagePublisher_t;

/* 每个订阅者有独立队列，互不抢消息。 */
typedef struct {
    MessageSubscriber_t *subscriber;
} MessageSubscriberHandle_t;

bool MessageCenter_Init(void);
bool MessageCenter_Publisher_Register(MessagePublisher_t *publisher,
                                      const char *name, uint16_t data_size);
bool MessageCenter_Subscriber_Register(MessageSubscriberHandle_t *subscriber,
                                       const char *name, uint16_t data_size);
bool MessageCenter_Publish(const MessagePublisher_t *publisher, const void *data);
bool MessageCenter_Subscribe_Get(MessageSubscriberHandle_t *subscriber, void *data);

#endif
