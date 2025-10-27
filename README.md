# The Five — LED Memorial
This project is inspired by “The Five: The Untold Lives of the Women Killed by Jack the Ripper” by [Hallie Rubenhold](https://hallierubenhold.com).
Rubenhold’s book reclaims the humanity of **Mary Ann Nichols, Annie Chapman, Elizabeth Stride, Catherine Eddowes, and Mary Jane Kelly** — women long reduced to the role of “victims” in history. Instead of focusing on the murderer, it tells the stories of their lives, circumstances, and voices erased by Victorian moral prejudice.

This code pays homage to them through light — five LED displays illuminating their names and fragments of their lives in an ongoing, cyclic rhythm, reflecting remembrance through repetition.

The work emerged from a creative prompt centred on the idea of “gathering” — an act of bringing together, of convergence and care. Within this context, The Five are symbolically gathered again, their separate lives reunited through light and code. Each display becomes a fragment of testimony, and together they form a collective presence: a memorial circuit where individual stories pulse side by side, meeting in rhythm and time.

## Overview
This Arduino sketch drives five MAX7219-based 7-segment LED displays, arranged vertically, each representing one of The Five. It reads data from a small in-memory CSV table stored in flash memory (PROGMEM), displaying one record at a time across the five displays.
Each record shows fragments of biographical information (e.g. name, date, location, and age). After a few seconds, the display updates to the next entry, looping indefinitely.
This project explores how electronic media can embody memory — turning text into light, sequence, and quiet persistence.

## Hardware Setup
Required components:
- Arduino-compatible board (e.g. Uno, Nano)
- 5 × MAX7219-based 7-segment display modules
- Power supply (5V)
- Jumper wires

### Wiring

| Signal | Arduino Pin | Colour | Notes |
| ------- | :---: | ------- | ------ |
| DIN | 11 | Green | Data input |
| CS | 12 | Blue | Chip select (cascade from OUT → IN between modules) |
| CLK | 13 | Orange | Clock |
| VCC | 5V | Red | Power |
| GND | GND | Black | Ground |

### Parallel connections:
- DIN and CLK are shared (parallel) across all displays
- CS is daisy-chained (from OUT to IN on each screen)

## Code Summary
- Library dependencies:
    - LedControl.h — to manage MAX7219 LED displays
    - <avr/pgmspace.h> — to store text in flash memory
- Display management:
    - Each display shows up to 8 characters. The sketch maps letters and numbers into the limited glyphs available on a 7-segment display using the mapChar() function, including fallbacks for unsupported characters.
- CSV logic:
    -   The data is defined as a single constant (csv_data) stored in program memory. Each line corresponds to five text fields.
    - The sketch reads one record at a time, parses it, and sends each field to a corresponding display.
- Timing:
    - The content changes every 8 seconds (RECORD_DELAY_MS = 8000), looping indefinitely.
- Error handling:
    - If a line contains invalid characters or exceeds length limits, an error pattern (decimal dots across all digits) is displayed.

## How to Run
1. Install the LedControl library via the Arduino IDE Library Manager.
2. Connect the displays according to the wiring diagram.
3. Upload the sketch to your Arduino board.
4. Power on the system — the displays will sequentially test, then begin showing the CSV data loop.

## Future Development
This is an ongoing project exploring memory, history, and technology. Planned features include:
- Improving the texts
- Refinement of typographic mapping to expand supported characters
- Movement or blinking effects, so the observer will know the device is working

# License
© Luiz Amorim, 2025 — Creative project under development.

This project is an artistic homage to “The Five: The Untold Lives of the Women Killed by Jack the Ripper” by [Hallie Rubenhold](https://hallierubenhold.com).
All literary rights, characters, and historical interpretations belong to the original author and publisher.
This work offers a creative reinterpretation through light and code, intended purely for artistic and educational purposes, with no commercial intent or claim over the original text.