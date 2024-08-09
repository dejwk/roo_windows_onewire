#include <Arduino.h>
#include <SPI.h>

#include "roo_display.h"
#include "roo_display/driver/ili9341.h"
#include "roo_display/driver/touch_xpt2046.h"
#include "roo_onewire.h"
#include "roo_windows.h"
#include "roo_windows/composites/menu/basic_navigation_item.h"
#include "roo_windows/composites/menu/menu.h"
#include "roo_windows/containers/aligned_layout.h"
#include "roo_windows_onewire.h"

using namespace roo_display;
using namespace roo_onewire;
using namespace roo_windows;

// Set your configuration for the driver.
static constexpr int kCsPin = 5;
static constexpr int kDcPin = 17;
static constexpr int kRstPin = 27;
static constexpr int kBlPin = 16;

static constexpr int kTouchCsPin = 2;

static constexpr int kOneWirePin = 25;

Ili9341spi<kCsPin, kDcPin, kRstPin> screen(Orientation().rotateLeft());
TouchXpt2046<kTouchCsPin> touch;

Display display(screen, touch,
                TouchCalibration(269, 249, 3829, 3684,
                                 Orientation::LeftDown()));

roo_scheduler::Scheduler scheduler;
Environment env(scheduler);

roo_windows::Application app(&env, display);

roo_onewire::OneWire onewire(kOneWirePin, scheduler);

enum Role { KITCHEN, BEDROOM };
std::vector<ThermometerRoles::Spec> roles = {{KITCHEN, "Kitchen"},
                                             {BEDROOM, "Bedroom"}};

ThermometerRoles thermometers(onewire, roles);
roo_windows_onewire::Configurator onewire_setup(env, thermometers);

class SettingsMenu : public menu::Menu {
 public:
  SettingsMenu(const Environment& env)
      : menu::Menu(env, "Settings"),
        onewire_item_(env, SCALED_ROO_ICON(filled, content_link),
                      "Thermometers", onewire_setup.main()) {
    add(onewire_item_);
  }

 private:
  menu::BasicNavigationItem onewire_item_;
};

SettingsMenu settings_menu(env);

class MainPane : public AlignedLayout {
 public:
  MainPane(const Environment& env)
      : AlignedLayout(env), button_(env, "Settings") {
    add(button_, kCenter | kMiddle);
    button_.setOnInteractiveChange(
        [this]() { getTask()->enterActivity(&settings_menu); });
  }

 private:
  SimpleButton button_;
};

MainPane pane(env);
SingletonActivity activity(app, pane);

void setup() {
  SPI.begin();

  display.init();
}

void loop() {
  app.tick();
  scheduler.executeEligibleTasks();
}
