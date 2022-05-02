#include <Arduino.h>
#include <USBComposite.h>
#include <limits>   // Push it to the limits!

// base configuration
static constexpr int8_t base_note = 60;   //C3 or C4, depending on implementation
static constexpr int8_t base_cc = 0;      //Bank select
static constexpr int8_t midi_max = 127;
typedef unsigned long Time;
static constexpr Time debounce_time_ms = 200; // time difference, may be smaller in type
static constexpr bool midi_cc_has_priority = true;

enum ChannelMapping : uint8_t {
    standard = 0,
    modulo,
    hold,
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
typedef uint8_t PinOffs;

// basic sanity checks
static_assert(sizeof(INPUT_PINS) < std::numeric_limits<PinOffs>::max());
static constexpr PinOffs num_input_pins = sizeof(INPUT_PINS);
static_assert(sizeof(OUTPUT_PINS) < std::numeric_limits<PinOffs>::max());
static constexpr PinOffs num_output_pins = sizeof(OUTPUT_PINS);

// global variables
uint8_t input_state[num_input_pins] = {0};      // reducing messages to "only changed"
Time last_input_change[num_input_pins] = {0};   // input debounce
int8_t last_mici_cc_value = -1;

uint8_t num_to_cc(PinOffs pin_number){
  static_assert(num_input_pins > 1);
  return (pin_number*midi_max)/(num_input_pins-1);
}
PinOffs cc_to_num(uint8_t cc_value){
  return ((num_output_pins-1)*cc_value+((midi_max/(num_input_pins-1))*2))/midi_max;
  // The following reacts to "start" of range, sometimes not good with certain DAWs
  //return ((num_output_pins-1)*cc_value+(midi_max-1))/midi_max;
}
static_assert(num_input_pins == num_output_pins,
  "Warning: if number of input and output pins differ, "
  "midi CCs are not mapped correctly"
);

class myMidi : public USBMIDI {
  void actuateNote(unsigned int channel, unsigned int note, bool on) {
    int8_t offs = -1;
    static_assert(num_output_pins < std::numeric_limits<typeof(offs)>::max());
    switch (channel) {
      case ChannelMapping::modulo:
        // "solo" modulo mode
        offs = note % num_output_pins;
        break;
      case ChannelMapping::hold:
        // "memory" mode that keeps on lighting the last note
        // Intended for showing current playing instrument slot
        if(on) {
          if(note >= base_note && (note - base_note) < num_output_pins) {
            for(const auto pin : OUTPUT_PINS) {
              digitalWrite(pin, 0 ^ invert_output);
            }
            offs = note - base_note;
          }
        }
        break;
      case ChannelMapping::standard:
      default:
        if(note >= base_note && (note - base_note) < num_output_pins) {
          offs = note - base_note;
        }
      }
      if(offs >= 0 && offs < num_output_pins &&
      		!(midi_cc_has_priority &&
                          (last_mici_cc_value >= 0 &&               // Default state: -1
                            offs == cc_to_num(last_mici_cc_value))  // would override cc indicator
                       ) ) 
      {
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
    switch(channel) {
      case ChannelMapping::cc_value:
      default:
        if(controller == base_cc) {
          // all pins off
          for(const auto pin : OUTPUT_PINS) {
            digitalWrite(pin, 0 ^ invert_output);
          }
          //except for the active one
          value %= midi_max+1; // just to be safe!
          digitalWrite(OUTPUT_PINS[cc_to_num(value)], 1 ^ invert_output);
          last_mici_cc_value = value;
        }
    }
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
    Time now = millis();
    for(uint8_t i = 0; i < num_input_pins; i++) {
        uint8_t meas = digitalRead(INPUT_PINS[i]) ^ invert_input;
        if(input_state[i] != meas && now - last_input_change[i] > debounce_time_ms) {
            if(meas) {
                midi.sendNoteOn(ChannelMapping::standard, base_note + i, midi_max);
                midi.sendControlChange(ChannelMapping::cc_value, base_cc, num_to_cc(i));
                digitalWrite(PC13, 0); //debug
            } else {
                midi.sendNoteOff(ChannelMapping::standard, base_note + i, midi_max);
                digitalWrite(PC13, 1); //debug
            }
            input_state[i] = meas;
            last_input_change[i] = now;
        }
    }
}

void testloop() {
	bool a_button_pressed = false;
    for(uint8_t i = 0; i < num_input_pins; i++) {
        a_button_pressed |= digitalRead(INPUT_PINS[i]) ^ invert_input;
    }
}
