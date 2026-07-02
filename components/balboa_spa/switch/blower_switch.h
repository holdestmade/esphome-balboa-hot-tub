#pragma once

#include "jet_switch_base.h"

namespace esphome
{
  namespace balboa_spa
  {

    // The blower is an on/off output, handled by the shared jet toggle logic
    // (retry with max attempts, discard-updates window after a command).
    class BlowerSwitch : public JetSwitchBase
    {
    public:
      BlowerSwitch() : JetSwitchBase("balboa_spa.blower_switch", "blower") {};

    protected:
      double get_jet_state(const SpaState *spaState) override { return spaState->blower; }
      void toggle_jet() override { spa->toggle_blower(); }
    };

  } // namespace balboa_spa
} // namespace esphome
