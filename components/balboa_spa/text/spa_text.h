#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text/text.h"
#include "../balboaspa.h"
#include <string>

namespace esphome
{
  namespace balboa_spa
  {

    class SpaTimeText : public text::Text
    {
    public:
      SpaTimeText() = default;
      void set_parent(BalboaSpa *parent);
      void update_from_spa_state(SpaState *state);

    protected:
      void control(const std::string &value) override;
      
    private:
      BalboaSpa *parent_;
      bool updating_from_spa_ = false;
    };

    class SpaFilter1StartTimeText : public text::Text
    {
    public:
      SpaFilter1StartTimeText() = default;
      void set_parent(BalboaSpa *parent);
      void update_from_filter_settings(SpaFilterSettings *settings);

    protected:
      void control(const std::string &value) override;
      
    private:
      BalboaSpa *parent_;
    };

    class SpaFilter1DurationText : public text::Text
    {
    public:
      SpaFilter1DurationText() = default;
      void set_parent(BalboaSpa *parent);
      void update_from_filter_settings(SpaFilterSettings *settings);

    protected:
      void control(const std::string &value) override;
      
    private:
      BalboaSpa *parent_;
    };

    class SpaFilter2StartTimeText : public text::Text
    {
    public:
      SpaFilter2StartTimeText() = default;
      void set_parent(BalboaSpa *parent);
      void update_from_filter_settings(SpaFilterSettings *settings);

    protected:
      void control(const std::string &value) override;
      
    private:
      BalboaSpa *parent_;
    };

    class SpaFilter2DurationText : public text::Text
    {
    public:
      SpaFilter2DurationText() = default;
      void set_parent(BalboaSpa *parent);
      void update_from_filter_settings(SpaFilterSettings *settings);

    protected:
      void control(const std::string &value) override;
      
    private:
      BalboaSpa *parent_;
    };

  } // namespace balboa_spa
} // namespace esphome