#include <LedControl.h>
#include <avr/pgmspace.h>

// Wiring:
// - DIN -> 11  ||  green
// - CS  -> 12   ||  blue
// - CLK -> 13  ||  orange
// - VCC -> 5V  ||  red
// - GND -> GND   ||  black
// - Power: in parallel
// - DIN & ClOCK - in parallel
// - CS: from out to In on each screen - the API figute out the sequence they are connected.

constexpr uint8_t FIELD_MAX = 24;
char fields[5][FIELD_MAX];
constexpr uint8_t PIN_DIN = 11;
constexpr uint8_t PIN_CLK = 13;
constexpr uint8_t PIN_CS = 12;
constexpr uint8_t NUM_DEVICES = 5;  // Five separate displays (stacked in a column)
constexpr uint8_t DIGITS_PER_DEVICE = 8;

LedControl lc = LedControl(PIN_DIN, PIN_CLK, PIN_CS, NUM_DEVICES);

// Valid range: 0..15  (0 = dimmest, 15 = brightest) - up to 8 if on Board.
int BRIGHTNESS = 8;

struct Mapped {
  bool isDigit;   // true => use setDigit; false => use setChar
  char ch;        // when isDigit==false, printable character for setChar
  uint8_t digit;  // when isDigit==true, 0..9
  bool valid;     // mapping produced something printable (space counts as printable)
};

char toUpperFast(char c) {
  if (c >= 'a' && c <= 'z') return c - 'a' + 'A';
  return c;
}

// Map a single character to something the 7-seg can show
Mapped mapChar(char c) {
  if (c >= '0' && c <= '9') {
    return { true, 0, static_cast<uint8_t>(c - '0'), true };
  }

  // Space
  if (c == ' ') return { false, ' ', 0, true };

  // Supported glyphs directly
  switch (c) {
    case 'A': return (Mapped){ false, 'A', 0, true };
    case 'B': return (Mapped){ false, 'b', 0, true };
    case 'C': return (Mapped){ false, 'C', 0, true };
    case 'D': return (Mapped){ false, 'd', 0, true };
    case 'E': return (Mapped){ false, 'E', 0, true };
    case 'F': return (Mapped){ false, 'F', 0, true };
    case 'H': return (Mapped){ false, 'H', 0, true };
    case 'L': return (Mapped){ false, 'L', 0, true };
    case 'P': return (Mapped){ false, 'P', 0, true };
  }

  // Smart look-alikes
  switch (c) {
    case 'G': return (Mapped){ true, 0, 9, true };     // “9” is closest on 7-seg
    case 'I': return (Mapped){ true, 0, 1, true };     // “1”
    case 'J': return (Mapped){ false, ' ', 0, true };  // No good J on 7-seg; blank
    case 'K': return (Mapped){ false, 'H', 0, true };  // Use “H” as nearest
    case 'M': return (Mapped){ false, 'H', 0, true };  // Use “H”
    case 'N': return (Mapped){ false, 'H', 0, true };  // Use “H”
    case 'O': return (Mapped){ false, 'o', 0, true };  // ledControl supports lowercase “o”
    case 'Q': return (Mapped){ true, 0, 0, true };     // show as “0”
    case 'R': return (Mapped){ false, 'A', 0, true };  // suggestive “A”
    case 'S': return (Mapped){ true, 0, 5, true };     // “5”
    case 'T': return (Mapped){ true, 0, 7, true };     // “7” evokes T
    case 'U': return (Mapped){ false, 'o', 0, true };  // rounded “o” as U-shape
    case 'V': return (Mapped){ false, 'L', 0, true };  // crude
    case 'W': return (Mapped){ false, 'P', 0, true };  // crude
    case 'X': return (Mapped){ false, 'H', 0, true };  // crude cross feel
    case 'Y': return (Mapped){ true, 0, 4, true };     // evoke “Y” like “4”
    case 'Z': return (Mapped){ true, 0, 2, true };     // “2”
  }

  // Anything else (symbols we don’t special-case): blank, still valid
  return (Mapped){ false, ' ', 0, true };
}

// DP markers: '.', ':', '/', '-' => set decimal point on the previous printed digit.
bool displayRow(uint8_t device, const String& raw) {
  if (device >= NUM_DEVICES) return false;

  // Uppercase and process
  String s;
  s.reserve(raw.length());
  for (size_t i = 0; i < raw.length(); i++) s += toUpperFast(raw[i]);

  // Prepare a compact list of printable “cells” with optional dp flags
  struct Cell {
    Mapped m;
    bool dp;
  };
  Cell cells[DIGITS_PER_DEVICE];
  uint8_t count = 0;

  auto setDpOnPrevious = [&](void) {
    if (count == 0) return;  // nothing to attach to
    cells[count - 1].dp = true;
  };

  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];

    // DP markers attach to previous printed digit/char
    if (c == '.' || c == ':' || c == '/' || c == '-') {
      setDpOnPrevious();
      continue;
    }

    Mapped m = mapChar(c);
    if (!m.valid) {
      // Unhandled fatal symbol => error state for the whole row
      showErrorRow(device);
      return false;
    }

    if (count >= DIGITS_PER_DEVICE) {
      // Overflow (more than 8 printable positions) => error state
      showErrorRow(device);
      return false;
    }

    cells[count++] = { m, false };
  }

  // Render left-aligned: leftmost is digit 7, rightmost is digit 0.
  // Example: cell[0] -> digit 7, cell[1] -> digit 6, ...
  lc.clearDisplay(device);
  for (uint8_t i = 0; i < count; i++) {
    int digitIndex = (DIGITS_PER_DEVICE - 1) - i;  // 7..0
    if (cells[i].m.isDigit) {
      lc.setDigit(device, digitIndex, cells[i].m.digit, cells[i].dp);
    } else {
      lc.setChar(device, digitIndex, cells[i].m.ch, cells[i].dp);
    }
  }
  // Fill remaining right-hand digits with blanks (no dp)
  for (int i = count; i < DIGITS_PER_DEVICE; i++) {
    int digitIndex = (DIGITS_PER_DEVICE - 1) - i;
    lc.setChar(device, digitIndex, ' ', false);
  }

  return true;
}

// error: empty line and just decimal points
void showErrorRow(uint8_t device) {
  if (device >= NUM_DEVICES) return;
  lc.clearDisplay(device);
  for (uint8_t i = 0; i < DIGITS_PER_DEVICE; i++) {
    lc.setChar(device, i, ' ', true);
  }
}

void setGlobalBrightness(uint8_t level) {
  if (level > 15) level = 15;
  for (uint8_t d = 0; d < NUM_DEVICES; d++) {
    lc.setIntensity(d, level);
  }
}

// Story in CSV format
const char csv_data[] PROGMEM =
  "MARY ANN,31-08-88,WHITECH,,AGE 43\n"
  "NICHOLS,ST JAMES,DAWES ST,WHITECH,VICTIM 1\n"
  "ANNIE,08/09/88,SPTLFLDS,,AGE 47\n"
  "CHAPMAN,29 HANBY,COURT,,VICTIM 2\n"
  "ELIZABTH,30/09/88,BERNER ST,,AGE 44\n"
  "STRIDE,SWEDISH,WORKED,,VICTIM 3\n"
  "CATHERIN,30/09/88,MITRE SQ,,AGE 46\n"
  "EDDOES,1.45AM,ALLEY,,VICTIM 4\n"
  "MARY JNE,09-11-88,MILLERS,,AGE 25\n"
  "KELLY,ROOM 13,SPTLFLDS,,VICTIM 5\n";

// Timing in ms
constexpr unsigned long RECORD_DELAY_MS = 8000;

// CSV reader state
size_t csv_index = 0;
bool csv_eof = false;

int pgmGet() {
  if (csv_eof) return -1;
  char c = pgm_read_byte_near(csv_data + csv_index);
  if (c == '\0') {
    csv_eof = true;
    return -1;
  }
  csv_index++;
  return (int)(unsigned char)c;
}

void csvRewind() {
  csv_index = 0;
  csv_eof = false;
}

// Read one CSV line into out[5][FIELD_MAX]
bool readCsvRecord(char out[5][FIELD_MAX]) {
  if (csv_eof) return false;

  // Clear outputs
  for (uint8_t f = 0; f < 5; ++f) out[f][0] = '\0';

  uint8_t field = 0;
  uint8_t pos = 0;

  while (true) {
    int ch = pgmGet();
    if (ch < 0) {
      if (field == 0 && pos == 0) return false;
      out[field][pos] = '\0';
      break;
    }

    if (ch == '\n') {
      out[field][pos] = '\0';
      break;
    }

    if (ch == '\r') continue;

    if (ch == ',') {
      out[field][pos] = '\0';
      field++;
      pos = 0;
      if (field >= 5) {
        while (true) {
          int ch2 = pgmGet();
          if (ch2 < 0 || ch2 == '\n') break;
          if (ch2 == '\r') continue;
        }
        break;
      }
      continue;
    }

    if (pos < FIELD_MAX - 1) {
      out[field][pos++] = (char)ch;
    }
  }

  // Trim spaces
  for (uint8_t f = 0; f < 5; ++f) {
    // left trim
    char* s = out[f];
    char* p = s;
    while (*p == ' ' || *p == '\t') ++p;
    if (p != s) {
      // shift left
      uint8_t i = 0;
      while (p[i]) {
        s[i] = p[i];
        ++i;
      }
      s[i] = '\0';
    }
    // right trim
    size_t len = 0;
    while (s[len]) ++len;
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t')) { s[--len] = '\0'; }
  }

  return true;
}

void showRecordOnDisplays(char f0[FIELD_MAX], char f1[FIELD_MAX], char f2[FIELD_MAX], char f3[FIELD_MAX], char f4[FIELD_MAX]) {
  displayRow(0, String(f0));
  displayRow(1, String(f1));
  displayRow(2, String(f2));
  displayRow(3, String(f3));
  displayRow(4, String(f4));
}

unsigned long lastChange = 0;

void setup() {
  for (uint8_t d = 0; d < NUM_DEVICES; d++) {
    lc.shutdown(d, false);
    lc.setScanLimit(d, 7);  // enable all 8 digits
    lc.setIntensity(d, BRIGHTNESS);
    lc.clearDisplay(d);
  }

  // test: shows the display number in the cells
  for (uint8_t d = 0; d < NUM_DEVICES; d++) {
    lc.clearDisplay(d);
    for (uint8_t i = 0; i < DIGITS_PER_DEVICE; i++) lc.setDigit(d, i, d, false);  // show 0..4
    delay(400);
    lc.clearDisplay(d);
  }

  // quick check if they are in sequence
  for (uint8_t d = 0; d < NUM_DEVICES; d++) lc.clearDisplay(d);
  for (uint8_t d = 0; d < NUM_DEVICES; d++) {
    for (uint8_t i = 0; i < DIGITS_PER_DEVICE; i++) lc.setChar(d, i, ' ', true);
    delay(200);
    for (uint8_t i = 0; i < DIGITS_PER_DEVICE; i++) lc.setChar(d, i, ' ', false);
  }

  // start at first CSV record
  csvRewind();
  if (readCsvRecord(fields)) {
    showRecordOnDisplays(fields[0], fields[1], fields[2], fields[3], fields[4]);
  }
  lastChange = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastChange >= RECORD_DELAY_MS) {
    if (!readCsvRecord(fields)) {
      // loop back to start
      csvRewind();
      if (!readCsvRecord(fields)) return;  // empty CSV
    }
    showRecordOnDisplays(fields[0], fields[1], fields[2], fields[3], fields[4]);
    lastChange = now;
  }
}
