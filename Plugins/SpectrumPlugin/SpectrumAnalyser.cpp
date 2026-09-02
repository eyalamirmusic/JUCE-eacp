#include "SpectrumAnalyser.h"

#include <algorithm>
#include <cmath>

namespace SpectrumPlugin
{
namespace
{
// The range the picture spans. The bottom is below the lowest note anybody
// plays; the top is where a 48k session runs out of spectrum anyway, and is
// clamped down further on a slower rate so the last bins are never empty.
constexpr auto bottomHz = 28.0;
constexpr auto topHz = 18000.0;

// The window the display maps onto its height, in dB. -84 is far enough down
// that a quiet tail still moves the picture; 0 is full scale, so a peaking
// signal fills it.
constexpr auto floorDb = -84.f;
constexpr auto ceilingDb = 0.f;

// A Hann window's coherent gain is 0.5, so a real sine of amplitude A leaves
// A * fftSize / 4 in its bin. Dividing by that puts a full-scale sine at 0dB,
// which is what makes the ceiling above mean what it says.
constexpr auto magnitudeScale = 4.f / (float) SpectrumAnalyser::fftSize;
} // namespace

SpectrumAnalyser::SpectrumAnalyser()
{
    rebuildBands(sampleRate.load(std::memory_order_relaxed));
}

void SpectrumAnalyser::prepare(double newSampleRate)
{
    // Only the rate is published. The bands built from it are the reader's, and
    // are rebuilt on its own thread the next time it notices the number moved —
    // writing them here would be writing what the render thread is reading.
    //
    // Nothing flushes the fifo: the samples in it were captured at the old
    // rate, but they are one window's worth and gone within a frame or two,
    // and a reset here is exactly the kind of write the reader cannot survive.
    sampleRate.store(newSampleRate, std::memory_order_relaxed);
}

void SpectrumAnalyser::pushBlock(const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
        return;

    const auto scale = 1.f / (float) numChannels;
    const auto* const* channels = buffer.getArrayOfReadPointers();

    // Whatever room there is. A full fifo means the editor is closed or a frame
    // ran long, and the write is short by however many samples did not fit —
    // which costs a picture nobody is looking at a fraction of one window.
    const auto write = fifo.write(numSamples);

    auto sample = 0;

    write.forEach(
        [&](int index)
        {
            auto sum = 0.f;

            for (auto channel = 0; channel < numChannels; ++channel)
                sum += channels[channel][sample];

            fifoBuffer[(size_t) index] = sum * scale;
            ++sample;
        });
}

void SpectrumAnalyser::analyse(float deltaSeconds,
                               float sensitivityDb,
                               float falloffSeconds)
{
    const auto rate = sampleRate.load(std::memory_order_relaxed);

    if (!juce::approximatelyEqual(rate, bandRate))
        rebuildBands(rate);

    // How much of the gap to a lower value survives this frame. Expressed per
    // second and exponentiated by the frame's own delta, so the picture falls
    // at the rate the slider says on a 60Hz panel and on a 120Hz one.
    const auto fall =
        falloffSeconds > 0.f ? std::exp(-deltaSeconds / falloffSeconds) : 0.f;

    if (drainIntoWindow())
    {
        transform(sensitivityDb);
    }
    else
    {
        // Nothing arrived at all: the host suspended the plugin, or stopped
        // calling it. Let the measurement itself fall rather than leaving the
        // picture frozen on the last thing that was playing.
        for (auto& value: measured)
            value *= fall;
    }

    auto loudest = 0.f;

    for (auto bin = 0; bin < binCount; ++bin)
    {
        auto& value = bins[(size_t) bin];
        const auto target = measured[(size_t) bin];

        // Instant on the way up, slow on the way down — a spectrum analyser's
        // usual asymmetry, and the reason a transient reads as a spike that
        // leaves a trail rather than as one frame nobody sees.
        value = target > value ? target : target + (value - target) * fall;
        loudest = juce::jmax(loudest, value);
    }

    level = loudest;
}

void SpectrumAnalyser::rebuildBands(double rate)
{
    bandRate = rate;

    const auto nyquist = rate * 0.5;
    const auto highest = juce::jmin(topHz, nyquist * 0.94);
    const auto binsPerHz = (double) fftSize / rate;
    const auto span = highest / bottomHz;
    const auto lastBin = fftSize / 2;

    for (auto bin = 0; bin < binCount; ++bin)
    {
        // Geometric spacing: each display bin covers the same *ratio* of
        // frequency as the one before it, which is what puts every octave on
        // the same number of pixels.
        const auto low = bottomHz * std::pow(span, (double) bin / (double) binCount);
        const auto high =
            bottomHz * std::pow(span, (double) (bin + 1) / (double) binCount);

        auto& band = bands[(size_t) bin];

        band.first = juce::jlimit(1, lastBin, (int) std::floor(low * binsPerHz));
        band.last =
            juce::jlimit(band.first, lastBin, (int) std::floor(high * binsPerHz));
    }
}

bool SpectrumAnalyser::drainIntoWindow()
{
    const auto ready = fifo.getNumReady();

    if (ready <= 0)
        return false;

    // Everything, not one window's worth: the tail of what is read is the
    // newest audio, and anything older than that is overwritten on the way
    // past. That is what keeps a backlog — a slow frame, an editor just
    // reopened — from being played back into the picture frame by frame.
    const auto read = fifo.read(ready);

    read.forEach(
        [this](int index)
        {
            window[(size_t) windowWrite] = fifoBuffer[(size_t) index];
            windowWrite = (windowWrite + 1) & (fftSize - 1);
        });

    return true;
}

void SpectrumAnalyser::transform(float sensitivityDb)
{
    // Out of the ring oldest-first, so the window is contiguous in time rather
    // than split wherever the writes happened to land.
    for (auto i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = window[(size_t) ((windowWrite + i) & (fftSize - 1))];

    std::fill(fftData.begin() + fftSize, fftData.end(), 0.f);

    hann.multiplyWithWindowingTable(fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data(), true);

    for (auto bin = 0; bin < binCount; ++bin)
    {
        const auto& band = bands[(size_t) bin];

        // The loudest bin in the band, not their average: a narrow peak that
        // lands in a wide band at the top of the range should still be a peak
        // in the picture rather than a bump divided by however many bins it
        // was sharing.
        auto peak = 0.f;

        for (auto i = band.first; i <= band.last; ++i)
            peak = juce::jmax(peak, fftData[(size_t) i]);

        const auto db =
            juce::Decibels::gainToDecibels(peak * magnitudeScale, floorDb)
            + sensitivityDb;

        measured[(size_t) bin] =
            juce::jlimit(0.f, 1.f, juce::jmap(db, floorDb, ceilingDb, 0.f, 1.f));
    }
}

} // namespace SpectrumPlugin
