#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <unistd.h>

#include <glm/glm.hpp>
#include <hpx/hpx.hpp>

// Converts number of bytes to a readable string representation
inline std::string byteString(size_t n, bool addSuffix = true)
{
    std::stringstream s;

    double bytes = (double) n;

    std::string suffix[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB" };

    for (int i = 0; i < 7; ++i) {
        if (bytes < 1024.0) {
            s << bytes;

            if (addSuffix)
                s << " " << suffix[i];

            return s.str();
        }

        bytes /= 1024.0;
    }

    return std::string("wow such many bytes");
}

// Returns the hostname of this machine
inline std::string hostname()
{
    char h[HOST_NAME_MAX];
    gethostname(h, HOST_NAME_MAX);

    return std::string(h);
}