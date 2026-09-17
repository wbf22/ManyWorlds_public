// Author: Claude
#pragma once

#include "PitchShifter.hpp"
#include <vector>
#include <cctype>

using namespace std;

class KeyboardMajor {
public:
    /**
     * C Major Scale Notes (three octaves)
     * C3 (MIDI 48) to F5 (MIDI 86)
     */
    enum class Note {
        // Lower octave
        C3 = 48,   // C3
        D3 = 50,   // D3
        E3 = 52,   // E3
        F3 = 53,   // F3
        G3 = 55,   // G3
        A3 = 57,   // A3
        B3 = 59,   // B3
        
        // Middle octave
        C4 = 60,   // C4
        D4 = 62,   // D4
        E4 = 64,   // E4
        F4 = 65,   // F4
        G4 = 67,   // G4
        A4 = 69,   // A4
        B4 = 71,   // B4
        C5 = 72,   // C5
        D5 = 74,   // D5
        
        // Upper octave
        E5 = 75,   // E5
        F5 = 76,   // F5
        G5 = 77,   // G5
        A6 = 78,   // A6
        B6 = 79,   // B6
        C6 = 80,   // C6
        D6 = 81,   // D6
        E6 = 82    // E6
    };

    KeyboardMajor(const vector<float>& sample) 
        : pitchShifter(sample) {}

    /**
     * Play a note from the C Major scale by keyboard key (piano-like mapping)
     * 
     * Upper row (QWERTY):    q w e r t y u i o p
     * Maps to piano notes:   C D E F G A B C D E (C5 to G5, upper octave)
     * 
     * Middle row (letters):  a s d f g h j k ;
     * Maps to piano notes:   C D E F G A B C D (C4 to D5)
     * 
     * Lower row (letters):   z x c v b n m
     * Maps to piano notes:   C D E F G A B (C3 to B3)
     * 
     * @param key Character representing the note from keyboard
     *            Upper:  'q'=C5, 'w'=D5, 'e'=E5, 'r'=F5, 't'=G5, 'y'=A5, 'u'=B5, 'i'=E5, 'o'=F5, 'p'=G5
     *            Middle: 'a'=C4, 's'=D4, 'd'=E4, 'f'=F4, 'g'=G4, 'h'=A4, 'j'=B4, 'k'=C5, ';'=D5
     *            Lower:  'z'=C3, 'x'=D3, 'c'=E3, 'v'=F3, 'b'=G3, 'n'=A3, 'm'=B3
     * @return Pitch-shifted sample for the requested note
     */
    vector<float> playNote(char key) const {
        key = tolower(key);
        
        switch (key) {
            // Upper row (QWERTY keys) - upper octave
            case 'q':
                return pitchShifter.getNote(static_cast<int>(Note::C5));
            case 'w':
                return pitchShifter.getNote(static_cast<int>(Note::D5));
            case 'e':
                return pitchShifter.getNote(static_cast<int>(Note::E5));
            case 'r':
                return pitchShifter.getNote(static_cast<int>(Note::F5));
            case 't':
                return pitchShifter.getNote(static_cast<int>(Note::G5));
            case 'y':
                return pitchShifter.getNote(static_cast<int>(Note::A6));
            case 'u':
                return pitchShifter.getNote(static_cast<int>(Note::B6));
            case 'i':
                return pitchShifter.getNote(static_cast<int>(Note::C6));
            case 'o':
                return pitchShifter.getNote(static_cast<int>(Note::D6));
            case 'p':
                return pitchShifter.getNote(static_cast<int>(Note::E6));
            
            // Middle row (letter keys) - middle octave
            case 'a':
                return pitchShifter.getNote(static_cast<int>(Note::C4));
            case 's':
                return pitchShifter.getNote(static_cast<int>(Note::D4));
            case 'd':
                return pitchShifter.getNote(static_cast<int>(Note::E4));
            case 'f':
                return pitchShifter.getNote(static_cast<int>(Note::F4));
            case 'g':
                return pitchShifter.getNote(static_cast<int>(Note::G4));
            case 'h':
                return pitchShifter.getNote(static_cast<int>(Note::A4));
            case 'j':
                return pitchShifter.getNote(static_cast<int>(Note::B4));
            case 'k':
                return pitchShifter.getNote(static_cast<int>(Note::C5));
            case ';':
                return pitchShifter.getNote(static_cast<int>(Note::D5));
            
            // Lower row (letter keys) - lower octave
            case 'z':
                return pitchShifter.getNote(static_cast<int>(Note::C3));
            case 'x':
                return pitchShifter.getNote(static_cast<int>(Note::D3));
            case 'c':
                return pitchShifter.getNote(static_cast<int>(Note::E3));
            case 'v':
                return pitchShifter.getNote(static_cast<int>(Note::F3));
            case 'b':
                return pitchShifter.getNote(static_cast<int>(Note::G3));
            case 'n':
                return pitchShifter.getNote(static_cast<int>(Note::A3));
            case 'm':
                return pitchShifter.getNote(static_cast<int>(Note::B3));
            
            default:
                // Default to C4 if invalid input
                return pitchShifter.getNote(static_cast<int>(Note::C4));
        }
    }

    /**
     * Play a note by Note enum
     * 
     * @param note The note to play
     * @return Pitch-shifted sample for the requested note
     */
    vector<float> playNote(Note note) const {
        return pitchShifter.getNote(static_cast<int>(note));
    }

    /**
     * Play a note by MIDI note number
     * Only valid MIDI notes in C major scale will produce correct results
     * Other notes will still be played but outside the intended scale
     * 
     * @param midiNote MIDI note number (0-127)
     * @return Pitch-shifted sample for the requested note
     */
    vector<float> playNoteByMidi(int midiNote) const {
        return pitchShifter.getNote(midiNote);
    }

private:
    PitchShifter pitchShifter;
};
