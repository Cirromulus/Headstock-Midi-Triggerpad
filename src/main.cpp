#include <Arduino.h>
#include <USBComposite.h>
#include <limits>   // Push it to the limits!

static constexpr int8_t base_note = 60;   //C3 or C4, depending on implementation
static constexpr int8_t base_cc = 30;
static constexpr int8_t midi_max = 127;

enum InputChannelMapping : uint8_t {
    standard = 0,
    modulo,
    hold
};

enum OutputChannelMapping : uint8_t {
    note = 0,
    cc_value
};

constexpr bool invert_input = false;
constexpr uint8_t INPUT_PINS[] = {
    PA0,
    PA1,
    PA2,
    PA3,
    PA4,
    PA5,
};

// At 3.3v, my button LEDs need around 25 Ohm to be driven with 5mA
constexpr bool invert_output = true;
constexpr uint8_t OUTPUT_PINS[] = {
    PB8,
    PB7,
    PB6,
    PB5,
    //PB4-PA15 do not have sink capability, PA12-11 are USB
    PA10,
    PA9,
};
uint8_t input_state[sizeof(INPUT_PINS)] = {0};

class myMidi : public USBMIDI {
  void actuateNote(unsigned int channel, unsigned int note, bool on) {
    int8_t offs = -1;
    static_assert(sizeof(OUTPUT_PINS) < std::numeric_limits<typeof(offs)>::max());
    switch (channel) {
      case InputChannelMapping::modulo:
        // "solo" modulo mode
        offs = note % sizeof(OUTPUT_PINS);
        break;
      case InputChannelMapping::hold:
        // "memory" mode that keeps on lighting the last note
        // Intended for showing current playing instrument slot
        if(on) {
          if(note >= base_note && (note - base_note) < sizeof(OUTPUT_PINS)) {
            for(const auto pin : OUTPUT_PINS) {
              digitalWrite(pin, 0 ^ invert_output);
            }
            offs = note - base_note;
          }
        }
        break;
      case InputChannelMapping::standard:
      default:
        if(note >= base_note && (note - base_note) < sizeof(OUTPUT_PINS)) {
          offs = note - base_note;
        }
      }
      if(offs >= 0) {
        digitalWrite(OUTPUT_PINS[offs], on ^ invert_output);
      }
      digitalWrite(PC13, on ^ true); //debug
  };

  virtual void handleNoteOff(unsigned int channel, unsigned int note, unsigned int velocity) {
    actuateNote(channel, note, false);
  }
  virtual void handleNoteOn(unsigned int channel, unsigned int note, unsigned int velocity) {
    actuateNote(channel, note, true);
  }
  virtual void handleControlChange(unsigned int channel, unsigned int controller, unsigned int value) {
    // TODO: Funny Patterns
  }

};

myMidi midi;
USBCompositeSerial CompositeSerial;

void setup() {
    // debug onboard PIN
    pinMode(PC13, OUTPUT);
    digitalWrite(PC13, 1);

    for(const auto pin : OUTPUT_PINS) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, 0 ^ invert_output);
    }
    for(const auto pin : INPUT_PINS) {
        if(invert_input)
            pinMode(pin, INPUT_PULLUP);
        else
            pinMode(pin, INPUT_PULLDOWN);
    }
    USBComposite.setProductId(0x0031);
    USBComposite.setProductString("Headstock_Trigger");
    USBComposite.setManufacturerString("PascalPieper.de");
    midi.registerComponent();
    CompositeSerial.registerComponent();
    USBComposite.begin();
}

void loop() {
    midi.poll();
    for(uint8_t i = 0; i < sizeof(INPUT_PINS); i++) {
        uint8_t meas = digitalRead(INPUT_PINS[i]) ^ invert_input;
        if(input_state[i] != meas) {
            if(meas) {
                midi.sendNoteOn(OutputChannelMapping::note, base_note + i, midi_max);
                midi.sendControlChange(OutputChannelMapping::cc_value, base_cc, i*(sizeof(INPUT_PINS)-1)/midi_max);
                digitalWrite(PC13, 0); //debug
            } else {
                midi.sendNoteOff(OutputChannelMapping::note, base_note + i, midi_max);
                digitalWrite(PC13, 1); //debug
            }
            input_state[i] = meas;
        }
    }
}
