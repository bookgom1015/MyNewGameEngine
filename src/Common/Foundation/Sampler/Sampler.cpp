#include "Common/Foundation/Sampler/Sampler.hpp"

#include <algorithm>
#include <numeric>

using namespace Common::Foundation;
using namespace DirectX;

UnitSquareSample2D Sampler::GetSample2D() {
	return mSamples[GetSampleIndex()];
}

HemisphereSample3D Sampler::GetHemisphereSample3D() {
	return mHemisphereSamples[GetSampleIndex()];
}

// Resets the sampler with newly randomly generated samples
void Sampler::Reset(UINT numSamples, UINT numSampleSets, HemisphereDistribution::Type hemisphereDistribution) {
    mIndex = 0;
    mNumSamples = numSamples;
    mNumSampleSets = numSampleSets;
    mSamples.resize(mNumSamples * mNumSampleSets, UnitSquareSample2D(FLT_MAX, FLT_MAX));
    mShuffledIndices.resize(mNumSamples * mNumSampleSets);
    mHemisphereSamples.resize(mNumSamples * mNumSampleSets, HemisphereSample3D(FLT_MAX, FLT_MAX, FLT_MAX));

    // Reset generator and initialize distributions.
    {
        // Initialize to the same seed for determinism.
        mGeneratorURNG.seed(msSeed);

        std::uniform_int_distribution<UINT> jumpDistribution(0, mNumSamples - 1);
        std::uniform_int_distribution<UINT> jumpSetDistribution(0, mNumSampleSets - 1);

        std::uniform_real_distribution<float> unitSquareDistribution(0.f, 1.f);

        // Specify the next representable value for the end range, since
        // uniform_real_distribution constructs excluding the end value [being, end).
        std::uniform_real_distribution<float> unitSquareDistributionInclusive(0.f, nextafter(1.f, FLT_MAX));

        GetRandomJump = [&]() { return jumpDistribution(mGeneratorURNG); };
        GetRandomSetJump = [&]() { return jumpSetDistribution(mGeneratorURNG); };
        GetRandomFloat01 = [&]() { return unitSquareDistribution(mGeneratorURNG); };
        GetRandomFloat01inclusive = [&]() { return unitSquareDistributionInclusive(mGeneratorURNG); };
    }
    // Generate random samples.
    {
        GenerateSamples2D();

        switch (hemisphereDistribution) {
        case HemisphereDistribution::E_Uniform: InitializeHemisphereSamples(0.f); break;
        case HemisphereDistribution::E_Cosine: InitializeHemisphereSamples(1.f); break;
        }

        for (UINT i = 0; i < mNumSampleSets; i++) {
            auto first = std::begin(mShuffledIndices) + i * mNumSamples;
            auto last = first + mNumSamples;

            std::iota(first, last, 0u); // Fill with 0, 1, ..., m_numSamples - 1 
            std::shuffle(first, last, mGeneratorURNG);
        }
    }
}

UnitSquareSample2D Sampler::RandomFloat01_2D() {
    return XMFLOAT2(GetRandomFloat01(), GetRandomFloat01());
}

UINT Sampler::GetRandomNumber(UINT min, UINT max) {
    std::uniform_int_distribution<UINT> distribution(min, max);
    return distribution(mGeneratorURNG);
}

// Get a valid index from <0, m_numSampleSets * m_numSamples>.
// The index increases by 1 on each call, but on a first 
// access of a next sample set, the:
// - sample set is randomly picked
// - sample set is indexed from a random starting index within a set.
// In addition the order of indices is retrieved from shuffledIndices.
UINT Sampler::GetSampleIndex() {
    // Initialize sample and set jumps.
    if (mIndex % mNumSamples == 0) {
        // Pick a random index jump within a set.
        mJump = GetRandomJump();

        // Pick a random set index jump.
        mSetJump = GetRandomSetJump() * mNumSamples;
    }

    return mSetJump + mShuffledIndices[(mIndex++ + mJump) % mNumSamples];
}

// Initialize samples on a 3D hemisphere from 2D unit square samples
// cosDensityPower - cosine density power {0, 1, ...}. 0:uniform, 1:cosine,...
void Sampler::InitializeHemisphereSamples(float cosDensityPower) {
    for (UINT i = 0, end = static_cast<UINT>(mSamples.size()); i < end; i++) {
        // Compute azimuth (phi) and polar angle (theta)
        /*
        float phi = XM_2PI * m_samples[i].x;
        float theta = acos(powf((1.f - mSamples[i].y), 1.f / (cosDensityPower + 1)));

        // Convert the polar angles to a 3D point in local orthornomal
        // basis with orthogonal unit vectors along x, y, z.
        mHemisphereSamples[i].x = sinf(theta) * cosf(phi);
        mHemisphereSamples[i].y = sinf(theta) * sinf(phi);
        mHemisphereSamples[i].z = cosf(theta);
        */
        // Optimized version using trigonometry equations.
        float cosTheta = std::powf((1.f - mSamples[i].y), 1.f / (cosDensityPower + 1));
        float sinTheta = std::sqrtf(1.f - cosTheta * cosTheta);
        mHemisphereSamples[i].x = sinTheta * cosf(XM_2PI * mSamples[i].x);
        mHemisphereSamples[i].y = sinTheta * sinf(XM_2PI * mSamples[i].x);
        mHemisphereSamples[i].z = cosTheta;

    }
}

// Generate multi-jittered sample patterns on a unit square [0,1].
// Ref: Section 5.3.4 in Ray Tracing from the Ground Up.
// The distribution has good random sampling distributions
// with somewhat uniform distributions in both:
// - 2D
// - 1D projections of each axes.
// Multi-jittered is a combination of two sample distributions:
// - Jittered: samples are distributed on a NxN grid, 
//             with each sample being random within its cell.
// - N-rooks/Linear hypercube sampling: samples have uniform
//             distribution in 1D projections of each axes.
void MultiJittered::GenerateSamples2D() {
    for (UINT s = 0; s < NumSampleSets(); s++) {
        // Generate samples on 2 level grid, with one sample per each (x,y)
        const UINT SampleSetStartID = s * NumSamples();

        const UINT T = NumSamples();
        const UINT N = static_cast<UINT>(sqrt(T));

        #define SAMPLE(i) mSamples[SampleSetStartID + i]

        // Generate random samples
        for (UINT col = 0, i = 0; col < N; col++) {
            for (UINT row = 0; row < N; row++, i++) {
                const XMFLOAT2 Stratum(static_cast<float>(row), static_cast<float>(col));
                const XMFLOAT2 Cell(static_cast<float>(col), static_cast<float>(row));
                const UnitSquareSample2D RandomValue01 = RandomFloat01_2D();

                SAMPLE(i).x = (RandomValue01.x + Cell.x) / T + Stratum.x / N;
                SAMPLE(i).y = (RandomValue01.y + Cell.y) / T + Stratum.y / N;
            }
        }

        // Shuffle sample axes such that there's a sample in each stratum 
        // and n-rooks is maintained.

        // Shuffle x coordinate across rows within a column
        for (UINT row = 0; row < N - 1; row++) {
            for (UINT col = 0; col < N; col++) {
                const UINT k = GetRandomNumber(row + 1, N - 1);
                std::swap(SAMPLE(row * N + col).x, SAMPLE(k * N + col).x);
            }
        }
            

        // Shuffle y coordinate across columns within a row
        for (UINT row = 0; row < N; row++) {
            for (UINT col = 0; col < N - 1; col++) {
                const UINT k = GetRandomNumber(col + 1, N - 1);
                std::swap(SAMPLE(row * N + col).y, SAMPLE(row * N + k).y);
            }
        }
    }
}

// Generate random sample patterns on unit square.
void Random::GenerateSamples2D() {
    for (auto& sample : mSamples) 
        sample = RandomFloat01_2D();
}