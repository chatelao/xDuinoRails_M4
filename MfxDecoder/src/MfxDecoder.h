#ifndef MfxDecoder_h
#define MfxDecoder_h

#include <stdint.h>
#include <stddef.h>

class MfxDecoder {
public:
    enum EventType {
        EVENT_NONE = 0,
        EVENT_BIT,
        EVENT_SYNC,
        EVENT_ERROR
    };

    struct DecodedEvent {
        EventType type;
        uint8_t value;

        bool operator==(const DecodedEvent& other) const {
            return type == other.type && value == other.value;
        }
    };

    MfxDecoder();
    void reset();
    void feedEdge(uint16_t deltaUs);

    bool available() const;
    DecodedEvent read();

private:
    enum State {
        STATE_HUNTING,
        STATE_SYNCED
    };

    State _state;

    static const uint8_t BUFFER_CAPACITY = 16;
    uint16_t _intervals[BUFFER_CAPACITY];
    uint8_t _bufferSize;

    static const uint8_t QUEUE_CAPACITY = 16;
    DecodedEvent _queue[QUEUE_CAPACITY];
    uint8_t _queueHead;
    uint8_t _queueTail;
    uint8_t _queueCount;

    static const uint16_t SHORT_MIN = 35;
    static const uint16_t SHORT_MAX = 65;
    static const uint16_t LONG_MIN = 85;
    static const uint16_t LONG_MAX = 115;

    void processBuffer();
    void pushEvent(EventType type, uint8_t value = 0);
    void shiftBuffer(uint8_t count);

    bool isShort(uint16_t val) const;
    bool isLong(uint16_t val) const;
};

#endif
