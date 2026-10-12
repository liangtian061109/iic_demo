#include "message_center.h"
#include <stddef.h>
#include <string.h>

struct MessageSubscriber_s {
    MessageTopic_t *topic;
    uint8_t queue[MESSAGE_CENTER_QUEUE_DEPTH][MESSAGE_CENTER_MAX_DATA_SIZE];
    uint8_t front;
    uint8_t back;
    uint8_t count;
};

struct MessageTopic_s {
    bool used;
    char name[MESSAGE_CENTER_TOPIC_NAME_SIZE];
    uint16_t data_size;
    MessageSubscriber_t *subscribers[MESSAGE_CENTER_MAX_SUBSCRIBERS];
    uint8_t subscriber_count;
};

static MessageTopic_t topics[MESSAGE_CENTER_MAX_TOPICS];
static MessageSubscriber_t subscribers[MESSAGE_CENTER_MAX_SUBSCRIBERS];
static uint8_t subscriber_count;

bool MessageCenter_Init(void)
{
    memset(topics, 0, sizeof(topics));
    memset(subscribers, 0, sizeof(subscribers));
    subscriber_count = 0;
    return true;
}

static MessageTopic_t *FindTopic(const char *name)
{
    for (uint8_t i = 0; i < MESSAGE_CENTER_MAX_TOPICS; ++i) {
        if (topics[i].used && strcmp(topics[i].name, name) == 0) {
            return &topics[i];
        }
    }
    return NULL;
}

static MessageTopic_t *GetOrCreateTopic(const char *name, uint16_t data_size)
{
    if (name == NULL || name[0] == '\0' || data_size == 0 ||
        data_size > MESSAGE_CENTER_MAX_DATA_SIZE ||
        strlen(name) >= MESSAGE_CENTER_TOPIC_NAME_SIZE) {
        return NULL;
    }

    MessageTopic_t *topic = FindTopic(name);
    if (topic != NULL) {
        return topic->data_size == data_size ? topic : NULL;
    }

    for (uint8_t i = 0; i < MESSAGE_CENTER_MAX_TOPICS; ++i) {
        if (!topics[i].used) {
            topic = &topics[i];
            memset(topic, 0, sizeof(*topic));
            topic->used = true;
            topic->data_size = data_size;
            memcpy(topic->name, name, strlen(name) + 1);
            return topic;
        }
    }
    return NULL;
}
bool MessageCenter_Publisher_Register(MessagePublisher_t *publisher,
                                      const char *name, uint16_t data_size)
{
    if (publisher == NULL || publisher->topic != NULL) {
        return false;
    }
    MessageTopic_t *topic = GetOrCreateTopic(name, data_size);
    if (topic == NULL) {
        return false;
    }
    publisher->topic = topic;
    return true;
}

bool MessageCenter_Subscriber_Register(MessageSubscriberHandle_t *subscriber,
                                       const char *name, uint16_t data_size)
{
    if (subscriber == NULL || subscriber->subscriber != NULL ||
        subscriber_count >= MESSAGE_CENTER_MAX_SUBSCRIBERS) {
        return false;
    }
    MessageTopic_t *topic = GetOrCreateTopic(name, data_size);
    if (topic == NULL || topic->subscriber_count >= MESSAGE_CENTER_MAX_SUBSCRIBERS) {
        return false;
    }
    /* 先完成实例和话题槽位的写入，最后再提交两个计数。 */
    const uint8_t subscriber_index = subscriber_count;
    const uint8_t topic_index = topic->subscriber_count;
    MessageSubscriber_t *instance = &subscribers[subscriber_index];
    memset(instance, 0, sizeof(*instance));
    instance->topic = topic;
    topic->subscribers[topic_index] = instance;
    subscriber->subscriber = instance;
    topic->subscriber_count = (uint8_t)(topic_index + 1);
    subscriber_count = (uint8_t)(subscriber_index + 1);
    return true;
}

bool MessageCenter_Publish(const MessagePublisher_t *publisher, const void *data)
{
    if (publisher == NULL || publisher->topic == NULL || data == NULL) {
        return false;
    }
    MessageTopic_t *topic = publisher->topic;
    for (uint8_t i = 0; i < topic->subscriber_count; ++i) {
        MessageSubscriber_t *subscriber = topic->subscribers[i];
        if (subscriber->count == MESSAGE_CENTER_QUEUE_DEPTH) {
            subscriber->front = (uint8_t)((subscriber->front + 1) %
                                          MESSAGE_CENTER_QUEUE_DEPTH);
            subscriber->count--;
        }
        memcpy(subscriber->queue[subscriber->back], data, topic->data_size);
        subscriber->back = (uint8_t)((subscriber->back + 1) %
                                     MESSAGE_CENTER_QUEUE_DEPTH);
        subscriber->count++;
    }
    return true;
}

bool MessageCenter_Subscribe_Get(MessageSubscriberHandle_t *subscriber, void *data)
{
    if (subscriber == NULL || subscriber->subscriber == NULL || data == NULL) {
        return false;
    }
    MessageSubscriber_t *instance = subscriber->subscriber;
    if (instance->count == 0) {
        return false;
    }
    memcpy(data, instance->queue[instance->front], instance->topic->data_size);
    instance->front = (uint8_t)((instance->front + 1) % MESSAGE_CENTER_QUEUE_DEPTH);
    instance->count--;
    return true;
}

