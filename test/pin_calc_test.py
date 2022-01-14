num_pins = 6
num_input_pins = num_pins
num_output_pins = num_pins
midi_max = 127

def cc_to_num(value):
    return int(int((num_input_pins-1)*value+(midi_max-1))/midi_max)

def mid_cc_to_num(cc_value):
    return int(int((num_output_pins-1)*cc_value+((midi_max/(num_output_pins-1))*2))/midi_max)

def num_to_cc(i):
    return int((i*midi_max)/(num_pins-1))

def num_to_mid_cc(i):
    return int((i*midi_max)/(num_pins) + (num_pins)/2)

def get_steps(beep):
    last_val = beep[0]
    ranges = [0]
    for i,val in enumerate(beep):
        if(val != last_val):
            ranges.append(i-1)
            ranges.append(i)
            #print("val: " + str(val) + ", last_val: " + str(last_val))
            last_val = val
    ranges.append(len(beep)-1)
    return [(ranges[i], ranges[i + 1])
        for i in range(0, len(ranges), 2)]


pins = range(num_pins)
cc = range(midi_max+1)

ccs = list(map(lambda x: num_to_cc(x), pins))
ccs_mid = list(map(lambda x: num_to_mid_cc(x), pins))

back_calc = list(map(lambda x: mid_cc_to_num(x), ccs))
back_calc_mid = list(map(lambda x: mid_cc_to_num(x), ccs))


print(list(pins))
print("         Midi value from input button: " + str(ccs))
print("Centered Midi value from input button: " + str(ccs_mid))
print("direct translation: " + str(back_calc))
print("center translation: " + str(back_calc_mid))

print("direct translation ranges: " + str(get_steps(list(map(lambda x: cc_to_num(x), cc)))))
print("center translation ranges: " + str(get_steps(list(map(lambda x: mid_cc_to_num(x), cc)))))
