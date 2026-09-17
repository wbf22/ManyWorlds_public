// Author: Claude
#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

using namespace std;

class PitchShifter {
private:
    vector<float> originalSample;
    static constexpr float A4_FREQUENCY = 440.0f;
    static constexpr int A4_MIDI_NOTE = 69;
    
    // Precomputed pitch-shifted samples for efficiency
    // Stores samples for 3 octaves around middle C (C3 to C6)
    // Middle C (C4) is MIDI note 60
    vector<vector<float>> pitchShiftCache;
    
    // MIDI note range: 36 (C2) to 96 (C7) - covers 3 octaves around middle C (60)
    inline static constexpr int MIN_MIDI_NOTE = 48;  // C3
    inline static constexpr int MAX_MIDI_NOTE = 84;  // C6
    inline static constexpr int CACHE_SIZE = MAX_MIDI_NOTE - MIN_MIDI_NOTE + 1;
    
public:
    /**
     * Constructor - initializes with a sample and preprocesses pitch shifts
     * @param sample Vector of floats representing the audio sample
     */
    PitchShifter(const vector<float>& sample) 
        : originalSample(sample) {
        preprocessPitchShifts();
    }
    
    /**
     * Get a pitch-shifted version of the sample for a given MIDI note
     * @param midiNote MIDI note number (48-84 for C3-C6)
     * @return Vector of floats representing the pitch-shifted sample
     */
    vector<float> getNote(int midiNote) const {
        // Clamp to valid range
        midiNote = max(MIN_MIDI_NOTE, min(MAX_MIDI_NOTE, midiNote));
        
        int cacheIndex = midiNote - MIN_MIDI_NOTE;
        if (cacheIndex >= 0 && cacheIndex < CACHE_SIZE) {
            return pitchShiftCache[cacheIndex];
        }
        
        // Fallback: return original sample if out of bounds
        return originalSample;
    }
    
    /**
     * Get the frequency in Hz for a given MIDI note
     * @param midiNote MIDI note number
     * @return Frequency in Hz
     */
    static float midiNoteToFrequency(int midiNote) {
        return A4_FREQUENCY * pow(2.0f, (midiNote - A4_MIDI_NOTE) / 12.0f);
    }
    
    /**
     * Calculate the pitch shift ratio needed to transpose from original to target note
     * Assumes the original sample is at C4 (MIDI note 60)
     * @param midiNote Target MIDI note number
     * @return Pitch shift ratio (playback speed multiplier)
     */
    static float calculatePitchRatio(int midiNote) {
        float targetFreq = midiNoteToFrequency(midiNote);
        float c4Freq = midiNoteToFrequency(60);  // C4 (middle C)
        return targetFreq / c4Freq;
    }

private:
    /**
     * Preprocesses all pitch shifts for the valid MIDI range
     * This is more efficient than real-time pitch shifting
     */
    void preprocessPitchShifts() {
        pitchShiftCache.resize(CACHE_SIZE);
        
        for (int midiNote = MIN_MIDI_NOTE; midiNote <= MAX_MIDI_NOTE; ++midiNote) {
            int cacheIndex = midiNote - MIN_MIDI_NOTE;
            float pitchRatio = calculatePitchRatio(midiNote);
            pitchShiftCache[cacheIndex] = pitchShiftSample(originalSample, pitchRatio);
        }
    }
    
    /**
     * Pitch shift a sample using linear interpolation
     * This is a simple but effective time-stretching algorithm
     * @param sample Input audio sample
     * @param ratio Pitch shift ratio (< 1.0 = lower pitch, > 1.0 = higher pitch)
     * @return Pitch-shifted sample
     */
    static vector<float> pitchShiftSample(const vector<float>& sample, float ratio) {
        if (sample.empty() || ratio <= 0.0f) {
            return sample;
        }
        
        // When pitching up, the output is shorter; when pitching down, it's longer
        size_t outputSize = static_cast<size_t>(sample.size() / ratio);
        vector<float> result(outputSize, 0.0f);
        
        for (size_t i = 0; i < outputSize; ++i) {
            // Map output index to input position
            float inputPos = i * ratio;
            int intPart = static_cast<int>(inputPos);
            float fracPart = inputPos - intPart;
            
            // Linear interpolation for smooth pitch shifting
            if (intPart < static_cast<int>(sample.size()) - 1) {
                result[i] = sample[intPart] * (1.0f - fracPart) + 
                           sample[intPart + 1] * fracPart;
            } else if (intPart < static_cast<int>(sample.size())) {
                result[i] = sample[intPart];
            }
        }
        
        return result;
    }
};

