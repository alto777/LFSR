# LFSR

Linear Feedback Shift Register synth sequencer(s)

For VCV Rack V 0.6.0

Now playing:
FG8 - 8 bit LFSR-based sequencer

Planned:

FG16 - 16 bit LFSR seuencer

Psychtone Mk II that funky 1970s "Music Composer-Synthesier"

Triadex MUSE emulator


# FG8
This is an 8 stage linear feedback shift register sequencer.

See

https://en.wikipedia.org/wiki/Linear-feedback_shift_register

for the math.

Google "Psychtone" and "Triadex MUSE' to get a peek at my inspiration for this and a few more plugin modules I will write.

The output channels are weighted sums. Any column of the shift register that is ON adds the knob value to the row output.

The tap select buttons (blue at bottom) choose the feedbback bits. The gate is active if the gate select button is on (green) and the shift register bit is high in a given column.

The gate mode switch allows suppression of output changes if there is no gate generated. 

The mode switch selects from two kinds of LFSR feedback methods. Again, wikipedia and google are your friends. While both Fibo(nacci) anf Gal(ois) configurations produce the same bit stream falling off the end, the state of the shift register whilst doing is different.

Disclaimer: I found this quote "LFSRs aren't so musically interesting that we're writing them up, but they are still cool." I'm slowly coming to agree with this, but… :-)

I made this by shamelessly hacking up SEQ3 from the Fundamental group of plugins. I changed the sequencing logic, which essentially shifts a '1' bit along N stages, to instead make the calculation of an LFSR. I changed the knobs to "snap" type to lock in a chromatic switching effect - you could leave that off for wildness, I suppose. My ear isn't good enough/I don't have the patience to dial in exact frequencies.

Very much a work in progress. I would like to do Don Lancaster's Psychtone tone generator (6 bits with some XOR taps) and ultimately the Triadex MUSE which has a 31 stage shift register and combines that with a 96 state counter for bits that are used for tone selection.

