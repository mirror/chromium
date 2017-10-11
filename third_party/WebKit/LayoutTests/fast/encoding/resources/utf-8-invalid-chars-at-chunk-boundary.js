// The offending bytes are a 3-byte char lead byte, a continuation byte, and an
// ASCII byte. To reproduce the bug, they need to be aligned so that the chunk
// (4096 bytes) contains the lead byte but not enough other bytes.

// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling filling filling filling filling filling filling.
// Filling filling filling fill

var string_with_invalid_chars = "Xà¥Y";
