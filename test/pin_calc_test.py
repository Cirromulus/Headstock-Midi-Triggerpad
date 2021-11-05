num_pins = 6
midi_max = 127

def cc_to_num(value):
    return int(int((num_pins-1)*value+(midi_max-1))/midi_max)

def num_to_cc(i):
    return int((i*midi_max)/(num_pins-1))

pins = range(6)

ccs = list(map(lambda x: num_to_cc(x), pins))

back_calc = list(map(lambda x: cc_to_num(x), ccs))

print(list(pins))
print(ccs)
print(back_calc)
