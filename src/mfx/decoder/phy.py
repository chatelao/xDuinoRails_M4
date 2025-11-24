import enum

class TransportState(enum.Enum):
    HUNTING = 0
    SYNCED = 1

class DecodedType(enum.Enum):
    BIT = 0
    SYNC = 1
    ERROR = 2

class DecodedEvent:
    def __init__(self, event_type, value=None):
        self.event_type = event_type
        self.value = value

    def __eq__(self, other):
        if isinstance(other, DecodedEvent):
            return self.event_type == other.event_type and self.value == other.value
        return False

    def __repr__(self):
        val_str = f", {self.value}" if self.value is not None else ""
        return f"DecodedEvent({self.event_type}{val_str})"

class RailSignalDecoder:
    def __init__(self):
        self.state = TransportState.HUNTING
        self.interval_buffer = []
        self.last_timestamp = None
        self.decoded_bits = [] # Output buffer

        # Tolerances
        self.short_min = 35
        self.short_max = 65
        self.long_min = 85
        self.long_max = 115

    def feed_samples(self, samples, sample_rate_hz):
        """
        Feed voltage samples.
        Assumes samples is a list of voltage values.
        Detects zero crossings.
        """
        dt_us = 1_000_000 / sample_rate_hz
        current_time = 0

        last_sample = None

        for sample in samples:
            if last_sample is not None:
                # Detect zero crossing
                if (last_sample < 0 and sample >= 0) or (last_sample >= 0 and sample < 0):
                    fraction = -last_sample / (sample - last_sample)
                    crossing_time = (current_time - dt_us) + fraction * dt_us

                    if self.last_timestamp is not None:
                        delta = crossing_time - self.last_timestamp
                        self.feed_edge(delta)

                    self.last_timestamp = crossing_time

            last_sample = sample
            current_time += dt_us

    def feed_edge(self, delta_us):
        """
        Process an edge with time delta from previous edge in microseconds.
        """
        self.interval_buffer.append(delta_us)
        self._process_buffer()

    def _process_buffer(self):
        while True:
            # 1. Search for Sync Pattern Violation in buffer
            # Pattern: [..., 100 (pre), 50, 100, 100, 50, 100 (post), ...]
            # Violation core: 50, 100, 100, 50 (indices i, i+1, i+2, i+3)

            sync_violation_idx = -1
            for i in range(len(self.interval_buffer) - 3):
                if (self._is_short(self.interval_buffer[i]) and
                    self._is_long(self.interval_buffer[i+1]) and
                    self._is_long(self.interval_buffer[i+2]) and
                    self._is_short(self.interval_buffer[i+3])):
                    sync_violation_idx = i
                    break

            if sync_violation_idx != -1:
                # Check for post-amble 100 at sync_violation_idx + 4
                if sync_violation_idx + 4 >= len(self.interval_buffer):
                    return # Wait for more data

                if sync_violation_idx > 1:
                    if self.state == TransportState.HUNTING:
                         start = sync_violation_idx - 1
                         self.interval_buffer = self.interval_buffer[start:]
                         continue
                    else:
                        pass
                else:
                    end_consume = sync_violation_idx + 5
                    self.decoded_bits.append(DecodedEvent(DecodedType.SYNC))
                    self.state = TransportState.SYNCED
                    self.interval_buffer = self.interval_buffer[end_consume:]
                    continue

            # Standard decoding
            if not self.interval_buffer:
                return

            if self.state == TransportState.HUNTING:
                if len(self.interval_buffer) > 10:
                    self.interval_buffer = self.interval_buffer[-6:]
                return

            # SYNCED decoding lookahead checks
            if sync_violation_idx == -1:
                if self._is_long(self.interval_buffer[0]):
                    if len(self.interval_buffer) >= 2 and self._is_short(self.interval_buffer[1]):
                         # Potential Sync start: 100, 50
                         if len(self.interval_buffer) < 3:
                             return

                         if self._is_long(self.interval_buffer[2]):
                             # 100, 50, 100... matches Sync prefix
                             if len(self.interval_buffer) < 5:
                                 return
                         else:
                             # 100, 50, 50... Not Sync.
                             pass

                elif self._is_short(self.interval_buffer[0]):
                     if len(self.interval_buffer) >= 2 and self._is_long(self.interval_buffer[1]):
                         # Potential Violation start: 50, 100
                         if len(self.interval_buffer) < 3:
                             return

                         if self._is_long(self.interval_buffer[2]):
                              # 50, 100, 100... matches Violation prefix
                              if len(self.interval_buffer) < 4:
                                  return
                         else:
                              # 50, 100, 50... Not Violation.
                              pass

            # Decode buffer[0]
            if self._is_long(self.interval_buffer[0]):
                 if len(self.interval_buffer) < 2:
                     return # Wait for next edge

                 self.decoded_bits.append(DecodedEvent(DecodedType.BIT, 0))
                 self.interval_buffer.pop(0)
            elif self._is_short(self.interval_buffer[0]):
                 if len(self.interval_buffer) >= 2 and self._is_short(self.interval_buffer[1]):
                     self.decoded_bits.append(DecodedEvent(DecodedType.BIT, 1))
                     self.interval_buffer.pop(0)
                     self.interval_buffer.pop(0)
                 elif len(self.interval_buffer) >= 2:
                     self.decoded_bits.append(DecodedEvent(DecodedType.ERROR, f"Invalid sequence at {self.interval_buffer[:2]}"))
                     self.interval_buffer.pop(0)
                     self.state = TransportState.HUNTING
                 else:
                     return # Wait for second half
            else:
                 self.decoded_bits.append(DecodedEvent(DecodedType.ERROR, f"Invalid interval: {self.interval_buffer[0]}"))
                 self.interval_buffer.pop(0)
                 self.state = TransportState.HUNTING

    def _is_short(self, val):
        return self.short_min <= val <= self.short_max

    def _is_long(self, val):
        return self.long_min <= val <= self.long_max

    def pop_events(self):
        events = self.decoded_bits
        self.decoded_bits = []
        return events
