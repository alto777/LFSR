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

The output channels are weighted sums. Any column of the shift register that is ON adds the knob value to the row output.

The tap select buttons (blue at bottom) choose the feedbback bits. The gate is active if the gate select button is on (green) and the shift register bit is high in a given column.

The gate mode switch allows suppression of output changes if there is no gate generated. 

The mode switch selects from two kinds of LFSR feedback methods. Again, wikipedia and google are your friends. While both Fibo(nacci) anf Gal(ois) configurations produce the same bit stream falling off the end, the state of the shift register whilst doing is different.

Disclaimer: I found this quote "LFSRs aren't so musically interesting that we're writing them up, but they are still cool." I'm slowly coming to agree with this, but… :-)

I made this by shamelessly hacking up SEQ3 from the Fundamental group of plugins. I changed the sequencing logic, which essentially shifts a '1' bit along N stages, to instead make the calculation of an LFSR. I changed the knobs to "snap" type to lock in a chromatic/diatonic switching effect - you could leave that off for wildness, I suppose. My ear isn't good enough/I don't have the patience to dial in exact frequencies.

Very much a work in progress. Lemme know if you make this go and how you'd like it improved. It isn't that interesting, but a trio made with three VCOs can get a bit entertaining.

# Psychtone

This is a re-creation of Don Lancaster's 1971 kit "Music Composer-Synthesizer", or the note selection part of it anyway. I'm still ironing out some details but it is usable at this stage and a bit of play can end up with some nice results. Be sure to turn up the "weight" mini-pots that are concentric with the "tune select" knobs. The shift register bits can be set/cleared by clicking on the LEDs like the FG8.

# COMING SOON, or later:

These are really just warm-up exercises. I will expand the FG8 to the obvious FG24, folding in ideas and improvements that I hope aren't all mine. I also have a crude but functional emulator for the Triadex MUSE. Google it.

