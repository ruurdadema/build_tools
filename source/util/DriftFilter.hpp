/*
 * Project: RAVENNAKIT demo application
 * Copyright (c) 2024-2025 Sound on Digital
 *
 * This file is part of the RAVENNAKIT demo application.
 *
 * The RAVENNAKIT demo application is licensed on a proprietary,
 * source-available basis. You are allowed to view, modify, and compile
 * this code for personal or internal evaluation only.
 *
 * You may NOT redistribute this file, any modified versions, or any
 * compiled binaries to third parties, and you may NOT use it for any
 * commercial purpose, except under a separate written agreement with
 * Sound on Digital.
 *
 * For the full license terms, see:
 *   LICENSE.md
 * or visit:
 *   https://ravennakit.com
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
