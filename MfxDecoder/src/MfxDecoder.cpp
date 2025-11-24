#include "MfxDecoder.h"

MfxDecoder::MfxDecoder() {
    reset();
}

void MfxDecoder::reset() {
    _state = STATE_HUNTING;
    _bufferSize = 0;
    _queueHead = 0;
    _queueTail = 0;
    _queueCount = 0;
}

void MfxDecoder::feedEdge(uint16_t deltaUs) {
    if (_bufferSize < BUFFER_CAPACITY) {
        _intervals[_bufferSize++] = deltaUs;
    } else {
        // Buffer full
        if (_state == STATE_HUNTING) {
             shiftBuffer(1);
             _intervals[_bufferSize++] = deltaUs;
        } else {
             // In SYNCED, overflowing means we lost sync or something.
             shiftBuffer(1);
             _intervals[_bufferSize++] = deltaUs;
             pushEvent(EVENT_ERROR, 0);
             _state = STATE_HUNTING;
        }
    }
    processBuffer();
}

bool MfxDecoder::available() const {
    return _queueCount > 0;
}

MfxDecoder::DecodedEvent MfxDecoder::read() {
    if (_queueCount == 0) {
        return {EVENT_NONE, 0};
    }
    DecodedEvent e = _queue[_queueHead];
    _queueHead = (_queueHead + 1) % QUEUE_CAPACITY;
    _queueCount--;
    return e;
}

void MfxDecoder::pushEvent(EventType type, uint8_t value) {
    if (_queueCount < QUEUE_CAPACITY) {
        _queue[_queueTail] = {type, value};
        _queueTail = (_queueTail + 1) % QUEUE_CAPACITY;
        _queueCount++;
    }
}

void MfxDecoder::shiftBuffer(uint8_t count) {
    if (count >= _bufferSize) {
        _bufferSize = 0;
        return;
    }
    for (uint8_t i = 0; i < _bufferSize - count; i++) {
        _intervals[i] = _intervals[i + count];
    }
    _bufferSize -= count;
}

bool MfxDecoder::isShort(uint16_t val) const {
    return val >= SHORT_MIN && val <= SHORT_MAX;
}

bool MfxDecoder::isLong(uint16_t val) const {
    return val >= LONG_MIN && val <= LONG_MAX;
}

void MfxDecoder::processBuffer() {
    while (true) {
        int16_t sync_idx = -1;

        // Search for Sync Pattern Violation: S L L S (50, 100, 100, 50)
        // Need at least 4 items.
        if (_bufferSize >= 4) {
            for (uint8_t i = 0; i <= _bufferSize - 4; i++) {
                if (isShort(_intervals[i]) &&
                    isLong(_intervals[i+1]) &&
                    isLong(_intervals[i+2]) &&
                    isShort(_intervals[i+3])) {
                    sync_idx = i;
                    break;
                }
            }
        }

        if (sync_idx != -1) {
            // Found violation

            // Check for post-amble at sync_idx + 4
            if (sync_idx + 4 >= _bufferSize) {
                return; // Wait for more data
            }

            if (sync_idx > 1) {
                if (_state == STATE_HUNTING) {
                    // Align to pre-amble
                    shiftBuffer(sync_idx - 1);
                    continue;
                } else {
                    // Fall through to decode data before Sync
                    // sync_idx > 1, so we assume valid data precedes it
                }
            } else {
                // sync_idx <= 1
                // Consume Sync
                pushEvent(EVENT_SYNC);
                _state = STATE_SYNCED;
                shiftBuffer(sync_idx + 5);
                continue;
            }
        }

        if (_bufferSize == 0) return;

        if (_state == STATE_HUNTING) {
            if (_bufferSize > 10) {
                 // Keep last 6
                 shiftBuffer(_bufferSize - 6);
            }
            return;
        }

        // SYNCED

        if (sync_idx == -1) {
            // Lookahead checks
            if (isLong(_intervals[0])) {
                if (_bufferSize >= 2 && isShort(_intervals[1])) {
                    // Potential Sync start: 100, 50
                    if (_bufferSize < 3) return;
                    if (isLong(_intervals[2])) {
                        // 100, 50, 100... Sync prefix
                        if (_bufferSize < 5) return;
                    }
                    // Else: 100, 50, 50... Not Sync.
                }
            } else if (isShort(_intervals[0])) {
                if (_bufferSize >= 2 && isLong(_intervals[1])) {
                    // Potential Violation start: 50, 100
                    if (_bufferSize < 3) return;
                    if (isLong(_intervals[2])) {
                         // 50, 100, 100... Violation prefix
                         if (_bufferSize < 4) return;
                    }
                }
            }
        }

        // Decode buffer[0]
        if (isLong(_intervals[0])) {
            if (_bufferSize < 2) return;

            pushEvent(EVENT_BIT, 0);
            shiftBuffer(1);
        } else if (isShort(_intervals[0])) {
            if (_bufferSize >= 2) {
                if (isShort(_intervals[1])) {
                    pushEvent(EVENT_BIT, 1);
                    shiftBuffer(2);
                } else {
                     // 50 followed by something else. Error.
                     pushEvent(EVENT_ERROR, 0); // Invalid sequence
                     shiftBuffer(1); // Consume one to progress
                     _state = STATE_HUNTING;
                }
            } else {
                return; // Wait
            }
        } else {
            // Invalid interval
            pushEvent(EVENT_ERROR, 0);
            shiftBuffer(1);
            _state = STATE_HUNTING;
        }
    }
}
