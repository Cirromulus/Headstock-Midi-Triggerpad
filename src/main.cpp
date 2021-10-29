#include <Arduino.h>
#include <USBComposite.h>

static constexpr int8_t base_note = 60;
constexpr uint8_t INPUT_PINS[] = {
    PA0,
    PA1,
    PA2,
    PA3,
    PA4,
    PA5,
};

constexpr bool invert_output = true;
constexpr uint8_t OUTPUT_PINS[] = {
    PB8,
    PB7,
    PB6,
    PB5,
    PB4,
    PB3,
    PC13
};
uint8_t input_state[sizeof(INPUT_PINS)] = {0};


class myMidi : public USBMIDI {
 virtual void handleNoteOff(unsigned int channel, unsigned int note, unsigned int velocity) {
     if(note >= base_note && (note - base_note) < sizeof(OUTPUT_PINS)) {
         digitalWrite(OUTPUT_PINS[note - base_note], 0 ^ invert_output);
     }
 }
 virtual void handleNoteOn(unsigned int channel, unsigned int note, unsigned int velocity) {
     if(note >= base_note && (note - base_note) < sizeof(OUTPUT_PINS)) {
         digitalWrite(OUTPUT_PINS[note - base_note], 1 ^ invert_output);
     }
  }

};

myMidi midi;
USBCompositeSerial CompositeSerial;

void setup() {
    for(const auto pin : OUTPUT_PINS) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, 1);
    }
    for(const auto pin : INPUT_PINS) {
        pinMode(pin, INPUT);
        digitalWrite(pin, 0);   //pull-down
    }
    USBComposite.setProductId(0x0031);
    midi.registerComponent();
    CompositeSerial.registerComponent();
    USBComposite.begin();
}

void loop() {
    midi.poll();
    for(uint8_t i = 0; i < sizeof(INPUT_PINS); i++) {
        uint8_t meas = digitalRead(INPUT_PINS[i]);
        if(input_state[i] != meas) {
            if(meas)
                midi.sendNoteOn(0, base_note + i, 127);
            else
                midi.sendNoteOff(0, base_note + i, 127);
            input_state[i] = meas;
        }
    }
}
