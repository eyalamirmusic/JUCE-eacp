#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>

namespace SpectrumPlugin
{

// The FFT, and the two threads it sits between.
//
// A visualizer needs a whole spectrum where the gain example needed one peak,
// and a spectrum is too big to publish in an atomic. So the channel is a
// lock-free SPSC fifo of mono samples: processBlock writes into it and the
// analysis reads out of it, and the two never share anything else.
//
// The transform itself runs on the *render* thread — once per displayed frame,
// from the view's update() — rather than on the audio thread. That is the whole
// reason the fifo carries samples rather than magnitudes:
//
//   - the audio thread is left doing a sum and a copy, which is the least a
//     tap can cost, and nothing it does depends on the size of the transform;
//   - the picture is refreshed as often as it is *drawn*, not as often as a
//     block arrives, so the analysis rate does not swing with the host's
//     buffer size;
//   - a 2048-point FFT is a few tens of microseconds, comfortably inside a
//     display refresh, and the display link asks for exactly that: a light
//     callback that advances state and returns.
//
// If the editor is closed nobody drains the fifo, it fills, and the audio
// thread's writes are dropped — which is correct, since there is no picture to
// be wrong. On reopening, the first frame drains whatever backlog is there in
// one go and keeps only the newest window, so the display starts current
// instead of catching up through stale audio.
class SpectrumAnalyser
{
public:
    // 4096 samples: ~85ms at 48k, and a 12Hz bin. The bin width is what decides
    // this, not the cost — the bottom of the display range is 28Hz, and at
    // half this size the lowest two octaves would share so few bins that they
    // draw as a visible staircase however smoothly the shader interpolates.
    // The window still holds a transient for only a handful of frames, which is
    // the other end of the trade.
    static constexpr auto fftOrder = 12;
    static constexpr auto fftSize = 1 << fftOrder;

    // The display bins the shader reads — a log-frequency regrouping of the
    // FFT's linear ones, so an octave takes the same width at the bottom of
    // the picture as at the top. Also the width of the GPU buffer.
    static constexpr auto binCount = 96;

    SpectrumAnalyser();

    // Called from prepareToPlay, which is not the audio thread but may run
    // while the editor renders — hence the atomic, and the rebuild happening
    // on the reader's side of it.
    void prepare(double newSampleRate);

    // Audio thread. Sums to mono and pushes; no lock, no allocation, and no
    // dependence on the transform size. Drops rather than blocks when the
    // fifo is full.
    void pushBlock(const juce::AudioBuffer<float>& buffer) noexcept;

    // Render thread, once per displayed frame. Drains the fifo, transforms
    // what it found, and folds the result into the bins the picture reads.
    void analyse(float deltaSeconds, float sensitivityDb, float falloffSeconds);

    // The picture's input: one value per display bin, 0 to 1.
    const std::array<float, binCount>& getBins() const noexcept { return bins; }

    // The loudest bin, for the parts of the picture that answer to the whole
    // signal rather than to one frequency.
    float getLevel() const noexcept { return level; }

private:
    // A display bin's slice of the FFT's linear bins. Both ends inclusive, and
    // both at least 1: bin 0 is DC, which is not a frequency anybody wants to
    // see.
    struct Band
    {
        int first = 1;
        int last = 1;
    };

    void rebuildBands(double rate);
    bool drainIntoWindow();
    void transform(float sensitivityDb);

    // Written by the audio thread, read by the render thread, and nothing
    // either of them owns is shared besides these two.
    static constexpr auto fifoSize = fftSize * 2;

    juce::AbstractFifo fifo {fifoSize};
    std::array<float, fifoSize> fifoBuffer {};

    // Consumer-owned from here down.
    std::array<float, fftSize> window {};
    int windowWrite = 0;

    juce::dsp::FFT fft {fftOrder};
    juce::dsp::WindowingFunction<float> hann {
        (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false};

    // Twice the transform size: JUCE's real-only transforms use the upper half
    // as scratch, whatever engine is underneath.
    std::array<float, fftSize * 2> fftData {};

    std::array<Band, binCount> bands {};
    std::array<float, binCount> measured {};
    std::array<float, binCount> bins {};
    float level = 0.f;

    std::atomic<double> sampleRate {44100.0};
    double bandRate = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyser)
};

} // namespace SpectrumPlugin
