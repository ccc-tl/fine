#include "progress.h"

#include "spdlog/spdlog.h"

Progress::Progress(std::vector<float> w) : previousPartsProgress(0), currentPartProgress(0), finished(false), weights(w) {
    std::reverse(weights.begin(), weights.end());
    currentWeight = weights.back();
    weights.pop_back();
    spdlog::trace("Set inital progress weight to: {}", currentWeight);
}

void Progress::updatePart(u32 current, u32 max) {
    if (finished) return;
    u32 partProgress = (current * 100.0 / max);
    u32 weightedPartProgress = partProgress * currentWeight;
    if (weightedPartProgress > currentPartProgress) {
        currentPartProgress = weightedPartProgress;
        spdlog::trace("Part progress: {}%", partProgress);
        spdlog::info("Total progress: {}%", previousPartsProgress + currentPartProgress);
    }
    if (current >= max) {
        nextPart();
    }
}

void Progress::nextPart() {
    if (finished) return;
    previousPartsProgress += 100 * currentWeight;
    if (weights.size() == 0) {
        finished = true;
    } else {
        currentPartProgress = 0;
        currentWeight = weights.back();
        weights.pop_back();
        spdlog::trace("Proceded to next progress part, set progress weight to: {}", currentWeight);
    }
}
