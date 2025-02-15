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

#include "ravennakit/ravenna/ravenna_node.hpp"

class ApplicationContext
{
public:
    rav::ravenna_node& getRavennaNode()
    {
        return ravennaNode_;
    }

private:
    rav::ravenna_node ravennaNode_;
};
