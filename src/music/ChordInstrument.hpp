// Author: Claude
#pragma once

#include "PitchShifter.hpp"
#include <vector>

using namespace std;

class ChordInstrument {
public:
    enum class ChordType {
        C_MAJOR,      // C, E, G
        G_MAJOR,      // G, B, D
        A_MINOR,      // A, C, E
        F_MAJOR,      // F, A, C
        E_MINOR,      // E, G, B
        D_MINOR,      // D, F, A
        C_SEVENTH,    // C, E, G, Bb
        G_SEVENTH,    // G, B, D, F
        A_MINOR_SEVENTH,  // A, C, E, G
        F_MAJOR_SEVENTH   // F, A, C, E
    };

    ChordInstrument(const vector<float>& sample) 
        : pitchShifter(sample) {}

    /**
     * Generate a chord by returning pitch-shifted samples for all notes in the chord
     * @param chordType The chord to generate
     * @return Vector of vectors, each inner vector is a pitch-shifted sample for one note
     */
    vector<vector<float>> playChord(ChordType chordType) const {
        vector<int> midiNotes = getMidiNotesForChord(chordType);
        vector<vector<float>> result;
        
        for (int midiNote : midiNotes) {
            result.push_back(pitchShifter.getNote(midiNote));
        }
        
        return result;
    }

private:
    PitchShifter pitchShifter;

    /**
     * Convert a chord type to its constituent MIDI notes
     * All chords are in the key of C, using notes in the range C3-C6
     * @param chordType The chord type to convert
     * @return Vector of MIDI note numbers for the chord
     */
    static vector<int> getMidiNotesForChord(ChordType chordType) {
        // Base notes in C major scale (C4 = MIDI 60):
        // C4 = 60, D4 = 62, E4 = 64, F4 = 65, G4 = 67, A4 = 69, B4 = 71
        // C5 = 72 (octave), Bb4 = 70 (flat)
        
        switch (chordType) {
            case ChordType::C_MAJOR:
                // C, E, G
                return {60, 64, 67};
                
            case ChordType::G_MAJOR:
                // G, B, D (in octave above)
                return {67, 71, 74};
                
            case ChordType::A_MINOR:
                // A, C, E
                return {69, 72, 76};
                
            case ChordType::F_MAJOR:
                // F, A, C
                return {65, 69, 72};
                
            case ChordType::E_MINOR:
                // E, G, B
                return {64, 67, 71};
                
            case ChordType::D_MINOR:
                // D, F, A
                return {62, 65, 69};
                
            case ChordType::C_SEVENTH:
                // C, E, G, Bb (dominant 7th)
                return {60, 64, 67, 70};
                
            case ChordType::G_SEVENTH:
                // G, B, D, F
                return {67, 71, 74, 77};
                
            case ChordType::A_MINOR_SEVENTH:
                // A, C, E, G
                return {69, 72, 76, 79};
                
            case ChordType::F_MAJOR_SEVENTH:
                // F, A, C, E
                return {65, 69, 72, 76};
                
            default:
                return {60};  // Default to C4
        }
    }
};
