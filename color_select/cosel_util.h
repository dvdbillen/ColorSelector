// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// This module is a respository of misc. helpers

#ifndef cosel_util_h
#define cosel_util_h


#include <chrono>
#include <thread>


namespace cosel {

// Just return milliseconds without typing all that
inline uint64_t millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>
           (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

// Just delay for some milliseconds without typing all that
inline void delay(unsigned millis) {
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}


}

#endif // cosel_util_h
