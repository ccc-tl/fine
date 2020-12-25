#pragma once

#include "platform.h"

class Progress {
  public:
    Progress(std::vector<float> weights);
    void updatePart(u32 current, u32 max);

  private:
    void nextPart();
    u32 previousPartsProgress;
    u32 currentPartProgress;
    float currentWeight;
    bool finished;
    std::vector<float> weights;
};
