#include <iostream>
#include <vector>
#include <cassert>
#include "../../src/MfxDecoder.h"
#include "../../src/MfxDecoder.cpp"

// Helper to print event
std::ostream& operator<<(std::ostream& os, const MfxDecoder::DecodedEvent& e) {
    os << "Event(";
    switch(e.type) {
        case MfxDecoder::EVENT_NONE: os << "NONE"; break;
        case MfxDecoder::EVENT_BIT: os << "BIT," << (int)e.value; break;
        case MfxDecoder::EVENT_SYNC: os << "SYNC"; break;
        case MfxDecoder::EVENT_ERROR: os << "ERROR"; break;
    }
    os << ")";
    return os;
}

void test_decode_0() {
    MfxDecoder decoder;
    // Feed Sync
    std::vector<uint16_t> sync = {100, 50, 100, 100, 50, 100};
    for(auto t : sync) decoder.feedEdge(t);

    assert(decoder.available());
    MfxDecoder::DecodedEvent e = decoder.read();
    assert(e.type == MfxDecoder::EVENT_SYNC);

    // Feed '0' -> 100
    decoder.feedEdge(100);
    // Should wait because 100 is ambiguous (could be start of Sync)
    assert(!decoder.available());

    // Feed flush
    decoder.feedEdge(100);
    assert(decoder.available());
    e = decoder.read();
    assert(e.type == MfxDecoder::EVENT_BIT);
    assert(e.value == 0);

    std::cout << "test_decode_0 PASSED" << std::endl;
}

void test_decode_1() {
    MfxDecoder decoder;
    std::vector<uint16_t> sync = {100, 50, 100, 100, 50, 100};
    for(auto t : sync) decoder.feedEdge(t);
    decoder.read(); // Consume SYNC

    decoder.feedEdge(50);
    assert(!decoder.available());
    decoder.feedEdge(50);
    assert(decoder.available());
    MfxDecoder::DecodedEvent e = decoder.read();
    assert(e.type == MfxDecoder::EVENT_BIT);
    assert(e.value == 1);

    std::cout << "test_decode_1 PASSED" << std::endl;
}

void test_sync_stream() {
    MfxDecoder decoder;
    std::vector<uint16_t> stream = {
        100, 50, 100, 100, 50, 100, // Sync
        100,                        // 0
        50, 50,                     // 1
        100, 50, 100, 100, 50, 100  // Sync
    };

    for(auto t : stream) decoder.feedEdge(t);

    // Read events
    // SYNC
    assert(decoder.available());
    MfxDecoder::DecodedEvent e1 = decoder.read();
    assert(e1.type == MfxDecoder::EVENT_SYNC);

    // BIT 0
    assert(decoder.available());
    MfxDecoder::DecodedEvent e2 = decoder.read();
    assert(e2.type == MfxDecoder::EVENT_BIT);
    assert(e2.value == 0);

    // BIT 1
    assert(decoder.available());
    MfxDecoder::DecodedEvent e3 = decoder.read();
    assert(e3.type == MfxDecoder::EVENT_BIT);
    assert(e3.value == 1);

    // SYNC
    assert(decoder.available());
    MfxDecoder::DecodedEvent e4 = decoder.read();
    assert(e4.type == MfxDecoder::EVENT_SYNC);

    std::cout << "test_sync_stream PASSED" << std::endl;
}

int main() {
    test_decode_0();
    test_decode_1();
    test_sync_stream();
    return 0;
}
