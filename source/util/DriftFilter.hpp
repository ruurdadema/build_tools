/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#pragma once

#include "ravennakit/core/assert.hpp"

/**
 * Experimental filter to reduce spikes in drift correction ratios.
 * It's easy to come up with something better, but for now this should do it.
 */
struct DriftFilter
{
    static constexpr double k_min_confidence = 0.000000001;
    double target { 1.0 };
    double confidence { 0.0001 };
    double confidence_step_factor { 1.00001 };

    double update (double value)
    {
        RAV_ASSERT (confidence_step_factor > 0.0, "Confidence step factor must be greater than zero");

        if (value > target + confidence)
        {
            value = target + confidence;
            confidence *= confidence_step_factor;
        }
        else if (value < target - confidence)
        {
            value = target - confidence;
            confidence *= confidence_step_factor;
        }
        else
        {
            confidence = std::max (confidence / confidence_step_factor, k_min_confidence);
        }

        return value;
    }
};
