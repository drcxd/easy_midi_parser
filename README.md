# Easy Midi Parser

This is a simple tool program which reads the MIDI format file and extract
the sound signal inside it. The programm consits of two parts.

# easy_midi_parser.c

This C program handles all the work with MIDI file. It extract the sound signals
and output them in certain format. Some data format in MIDI file is not recognized
now, but it is easy to modify the program to add more functionality to it.

# emp2zc.pl

This a Perl script which converts the output of the easy_midi_parser.c into some
specific form. It is not written for general purpose.
