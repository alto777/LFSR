# LFSR

LFSR linear feedback shift register synth sequencer

For VCV Rack V 0.5.0 I think.

Sorry, speaking too soon: just in case you haven't noticed I didn't learn enough to start messing with git. I'll try to make this usable.

This is my noob attempt at a linear feedback shift register sequencer.

See

https://en.wikipedia.org/wiki/Linear-feedback_shift_register

for the math.

The output channels are weighted sums. Any column of the shift register that is ON adds the knob value to the row output.

The tap select buttons (blue at bottom) choose the feedbback bits. The gate is active if the gate select button is on (green) and the shift register bit is high in a given column.

The mode switch selects from two kinds of LFSR feedback methods. While both produce the same bit stream falling off the end, the state of the shift register whilst doing is different.

Disclaimer: I found this quote "LFSRs aren't so musically interesting that we're writing them up, but they are still cool." I'm slowly coming to agree with this, but… :-)

I made this by shamelessly hacking up SEQ3 from the Fundamental group of plugins. I changed the sequencing logic, which essentially shifts a '1' bit along N stages, to instead make the calculation of an LFSR. I changed the knobs to "snap" type to lock in a chromatic switching effect - you could leave that off for wildness, I suppose. My ear isn't good enough/I don't have the patience to dial in exact frequencies.

Very much a work in progress. I would like to do Don Lancaster's Psychtone tone generator (6 bits with some XOR taps) and ultimately the Triadex MUSE which has a 31 stage shift register and combines that with a 96 state counter for bits that are used for tone selection.

Google "Psychtone" and "Triadex MUSE'.