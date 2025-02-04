#pragma once
#include "SysModPrint.h"

#include <vector>

class Modules {
  private:
    std::vector<Module *> modules;

  public:

  void setup() {
    for (Module *module:modules) {
      print->print("MODULES SETUP %s\n", module->name);
      module->setup();
    }
  }

  void loop() {
    for (Module *module:modules) {
      if (module->enabled && module->success) {
        module->loop();
        // module->testManager();
        // module->performanceManager();
        // module->dataSizeManager();
        // module->codeSizeManager();
      }
    }
  }

  void add(Module* module) {
    print->print("MODULES ADD %s\n", module->name);
    modules.push_back(module);
  }

  void connected() {
    print->print("MODULES connected \n");
    for (Module *module:modules) { //this causes crash!!! why???
      print->print("MODULES connected to %s\n", module->name);
      module->connected();
    }
  }

};

static Modules *mdls;