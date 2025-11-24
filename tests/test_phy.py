import unittest
from src.mfx.decoder.phy import RailSignalDecoder, DecodedEvent, DecodedType

class TestRailSignalDecoder(unittest.TestCase):
    def setUp(self):
        self.decoder = RailSignalDecoder()

    def test_decode_0(self):
        # Initially Hunting, so we need Sync first
        # Sync pattern: 100, 50, 100, 100, 50, 100

        # Feed Sync
        sync_pattern = [100, 50, 100, 100, 50, 100]
        for t in sync_pattern:
            self.decoder.feed_edge(t)

        events = self.decoder.pop_events()
        self.assertEqual(events, [DecodedEvent(DecodedType.SYNC)])

        # Feed '0' -> 100
        self.decoder.feed_edge(100)
        # Should wait because 100 is ambiguous (could be start of Sync)
        self.assertEqual(self.decoder.pop_events(), [])

        # Feed another edge to flush previous 100
        self.decoder.feed_edge(100)
        events = self.decoder.pop_events()
        # First 100 should be decoded as BIT(0)
        self.assertEqual(events, [DecodedEvent(DecodedType.BIT, 0)])

    def test_decode_1(self):
        # Sync
        for t in [100, 50, 100, 100, 50, 100]:
            self.decoder.feed_edge(t)
        self.decoder.pop_events()

        # Feed '1' -> 50, 50
        self.decoder.feed_edge(50)
        self.assertEqual(self.decoder.pop_events(), []) # Incomplete
        self.decoder.feed_edge(50)
        self.assertEqual(self.decoder.pop_events(), [DecodedEvent(DecodedType.BIT, 1)])

    def test_sync_detection_in_stream(self):
        # Stream: Sync, 0, 1, Sync
        # Sync: 100, 50, 100, 100, 50, 100
        # 0: 100
        # 1: 50, 50
        # Sync: ...

        stream = [
            100, 50, 100, 100, 50, 100, # Sync
            100,                        # 0
            50, 50,                     # 1
            100, 50, 100, 100, 50, 100  # Sync
        ]

        for t in stream:
            self.decoder.feed_edge(t)

        events = self.decoder.pop_events()
        expected = [
            DecodedEvent(DecodedType.SYNC),
            DecodedEvent(DecodedType.BIT, 0),
            DecodedEvent(DecodedType.BIT, 1),
            DecodedEvent(DecodedType.SYNC)
        ]
        self.assertEqual(events, expected)

    def test_sync_hunting_offset(self):
        # Feed garbage then Sync
        # Garbage: 100, 100
        # Sync: ...

        stream = [100, 100] + [100, 50, 100, 100, 50, 100]
        for t in stream:
            self.decoder.feed_edge(t)

        events = self.decoder.pop_events()
        # The garbage is discarded during hunting
        self.assertEqual(events, [DecodedEvent(DecodedType.SYNC)])

    def test_partial_sync_handling(self):
        # Sync
        for t in [100, 50, 100, 100, 50, 100]:
            self.decoder.feed_edge(t)
        self.decoder.pop_events()

        # 0: 100
        # partial sync lookalike: 100, 50... but followed by 50 (so it's 0, 1)

        stream = [100, 50, 50]
        for t in stream:
            self.decoder.feed_edge(t)

        events = self.decoder.pop_events()
        self.assertEqual(events, [DecodedEvent(DecodedType.BIT, 0), DecodedEvent(DecodedType.BIT, 1)])

if __name__ == '__main__':
    unittest.main()
