# grids

Port of [Mutable Instruments Grids](https://pichenettes.github.io/mutable-instruments-documentation/modules/grids/) to SnazzyFX Ardcore.

ArdCore sequencing is based on [20 Objects GateSequence sketch](https://github.com/darwingrosse/ArdCore-Code/blob/master/20%20Objects/AC14_GateSequence/AC14_GateSequence.ino).

Some parts of the code like the pattens and morphing between different drum patterns, and random number generator are taken directly from [Grids' source code](https://github.com/pichenettes/eurorack/tree/master/grids) and Emilie Gillet's [avrlib](https://github.com/pichenettes/avril/). This is possible since Grids uses the same chip as Ardcore, and Arduino is essentially a wrapper around C++ code for programming the AVR chip.

Drum patterns themselves are stored in exactly the same format as Grids, so you can use the [same methods as for modifying Grids patterns](https://www.youtube.com/watch?v=Eex-iLuUdiw).


## Set up

Check out the repository:
```
git clone https://github.com/krastins/grids.git
```

Or [download the zip archive](https://github.com/krastins/grids/archive/refs/heads/main.zip) if you're unfamiliar with `git` and extract it to your Arduino sketches folder.

Open the sketch in Arduino IDE, connect your Ardcore and upload.

## Dependencies

Select Mutable Insruments Grids dependencies (`U8Mix`, `U8U8MulShift8`, `random.h`) from [avrlib](https://github.com/pichenettes/avril/) (but not the full avrlib) are bundled together with this sketch for convenience.

## Limitations

Since Ardcore has fewer inputs and outputs than Grids, I had to make some compromises for controls and outputs:

* X and Y coordinates are controlled by Ardcore's A0 and A1 knobs and have no CV control.
* Fill amount is controlled by a single knob. It controls all 3 drums by default, but you can easily change it in the code to affect only one or two of the drums if you want.
* Since Ardcore has only 2 digital outs (and I don't have the Ardcore expander), I had to use the DAC out for the third drum (HH).
* There's no way to reset the sequencer becasue of the limited inputs.
* This doesn't have the Grids' Euclidean mode. If you're looking for an Euclidean sequencer, [there's another Ardcore sketch for that](https://github.com/darwingrosse/ArdCore-Code/blob/master/20%20Objects/AC30_DualEuclidean/AC30_DualEuclidean.ino).

## Further reading

[Mutable Instruments Grids manual](https://pichenettes.github.io/mutable-instruments-documentation/modules/grids/manual/)

[Mutable Instruments Grids Drum Fill visualisation](https://devdsp.com/grids.html)