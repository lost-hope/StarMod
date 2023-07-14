#include "SysModSystem.h"
#include "module.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModWeb.h"

SysModSystem::SysModSystem() :Module("System") {};

void SysModSystem::setup() {
  Module::setup();
  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  parentObject = ui->initGroup(parentObject, name);

  ui->initDisplay(parentObject, "upTime", nullptr, [](JsonObject object) { //uiFun
    web->addResponse(object, "comment", "Uptime of board");
  });
  ui->initDisplay(parentObject, "loops");
  ui->initDisplay(parentObject, "heap", nullptr, [](JsonObject object) { //uiFun
    web->addResponse(object, "comment", "Free / Total (largest free)");
  });
  ui->initDisplay(parentObject, "stack");

  ui->initButton(parentObject, "restart", "Restart", nullptr, [](JsonObject object) {  //chFun
    web->ws->closeAll(1012);
    ESP.restart();
  });

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void SysModSystem::loop() {
  // Module::loop();

  loopCounter++;
  if (millis() - secondMillis >= 1000 || !secondMillis) {
    secondMillis = millis();
    ui->setValueV("upTime", "%u s", millis()/1000);
    ui->setValueV("loops", "%lu /s", loopCounter);
    ui->setValueV("heap", "%d / %d (%d) B", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap());
    ui->setValueV("stack", "%d B", uxTaskGetStackHighWaterMark(NULL));

    loopCounter = 0;
    
  }
}