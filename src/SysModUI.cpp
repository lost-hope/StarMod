#include "SysModUI.h"
#include "SysModPrint.h"
#include "SysModWeb.h"
#include "SysModModel.h"

//init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
std::vector<void(*)(JsonObject object)> SysModUI::uiFunctions;
std::vector<UserLoop> SysModUI::loopFunctions;

bool SysModUI::userLoopsChanged = false;;
  SysModUI::SysModUI() :Module("UI") {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    success &= web->addURL("/", "/index.htm", "text/html");
    // success &= web->addURL("/index.js", "/index.js", "text/javascript");
    // success &= web->addURL("/index.css", "/index.css", "text/css");

    success &= web->setupJsonHandlers("/json", processJson);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //serve index.htm
  void SysModUI::setup() {
    Module::setup();

    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = initGroup(parentObject, name);

    JsonObject userLoopsObject = initMany(parentObject, "uloops", nullptr, [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "User loops");
      JsonArray rows = web->addResponseArray(object, "many");

      for (auto userLoop = begin (loopFunctions); userLoop != end (loopFunctions); ++userLoop) {
        JsonArray row = rows.createNestedArray();
        row.add(userLoop->object["id"]);
        row.add(userLoop->counter);
        userLoopsChanged = userLoop->counter != userLoop->prevCounter;
        userLoop->prevCounter = userLoop->counter;
        userLoop->counter = 0;
      }
    });
    initDisplay(userLoopsObject, "ulObject", nullptr, [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "Name");
    });
    initDisplay(userLoopsObject, "ulLoopps", nullptr, [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "Loops/s");
    });

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void SysModUI::loop() {
    // Module::loop();

    for (auto userLoop = begin (loopFunctions); userLoop != end (loopFunctions); ++userLoop) {
      if (millis() - userLoop->lastMillis >= userLoop->interval) {
        userLoop->lastMillis = millis();

        web->ws->cleanupClients();
        if (web->ws->count()) {

          //send object to notify client data coming is for object (client then knows it is canvas and expects data for it)
          setChFunAndWs(userLoop->object, "new");

          //send leds info in binary data format
          AsyncWebSocketMessageBuffer * wsBuf = web->ws->makeBuffer(userLoop->bufSize * 3 + 4);
          if (wsBuf) {//out of memory
            wsBuf->lock();
            uint8_t* buffer = wsBuf->get();

            //to loop over old size
            buffer[0] = userLoop->bufSize / 256;
            buffer[1] = userLoop->bufSize % 256;
            // print->print("interval1 %u %d %d %d %d %d %d\n", millis(), userLoop->interval, userLoop->bufSize, buffer[0], buffer[1]);

            userLoop->loopFun(userLoop->object, buffer); //call the function and fill the buffer

            userLoop->bufSize = buffer[0] * buffer[1] * buffer[2];
            userLoop->interval = buffer[3]*10; //from cs to ms
            // print->print("interval2 %u %d %d %d %d %d %d\n", millis(), userLoop->interval, userLoop->bufSize, buffer[0], buffer[1], buffer[2], buffer[3]);

            for (auto client:web->ws->getClients()) {
              if (!client->queueIsFull() && client->status() == WS_CONNECTED) 
                client->binary(wsBuf);
              else {
                // web->clientsChanged = true; tbd: changed also if full status changes
                web->printClient("loopFun client full or not connected", client);
              }
            }
            wsBuf->unlock();
            web->ws->_cleanBuffers();
          }
          else {
            print->print("loopFun WS buffer allocation failed\n");
            web->ws->closeAll(1013); //code 1013 = temporary overload, try again later
            web->ws->cleanupClients(0); //disconnect all clients to release memory
            web->ws->_cleanBuffers();
          }
        }

        userLoop->counter++;
        // print->print("%s %u %u %d %d\n", userLoop->object["id"].as<const char *>(), userLoop->lastMillis, millis(), userLoop->interval, userLoop->counter);
      }
    }

    if (millis() - secondMillis >= 1000 || !secondMillis) {
      secondMillis = millis();

      //replace uloops table
      JsonVariant responseVariant = (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?web->responseDoc0:web->responseDoc1)->as<JsonVariant>();
      (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?web->responseDoc0:web->responseDoc1)->clear(); //needed for deserializeJson?

      responseVariant["uiFun"] = "uloops";
      processJson(responseVariant); //this calls uiFun command, which might change userLoopsChanged

      //if something changed in uloops
      if (userLoopsChanged) {
        userLoopsChanged = false;
        // print->printJson("uloops change response", responseVariant);
        web->sendDataWs(responseVariant);
      }
    }
  }

  JsonObject SysModUI::initObject(JsonObject parent, const char * id, const char * type, USFun uiFun, USFun chFun, LoopFun loopFun) {
    JsonObject object = findObject(id);

    //create new object
    if (object.isNull()) {
      print->print("initObject create new %s: %s\n", type, id);
      // print->printJson("test2", *mdl->model);
      if (parent.isNull()) {
        JsonArray objects = mdl->model->as<JsonArray>();
        object = objects.createNestedObject();
      } else {
        if (parent["n"].isNull()) parent.createNestedArray("n"); //if parent exist and no "n" array, create it
        object = parent["n"].createNestedObject();
        // serializeJson(model, Serial);Serial.println();
      }
      object["id"] = id;
    }
    else
      print->print("Object %s already defined\n", id);

    if (!object.isNull()) {
      object["s"] = true; //deal with obsolete objects, check object exists in code
      object["type"] = type;
      if (uiFun) {
        //if fun already in uiFunctions then reuse, otherwise add new fun in uiFunctions
        std::vector<void(*)(JsonObject object)>::iterator itr = find(uiFunctions.begin(), uiFunctions.end(), uiFun);
        if (itr!=uiFunctions.end()) //found
          object["uiFun"] = distance(uiFunctions.begin(), itr); //assign found function
        else { //not found
          uiFunctions.push_back(uiFun); //add new function
          object["uiFun"] = uiFunctions.size()-1;
        }
      }
      if (chFun) {
        //if fun already in uiFunctions then reuse, otherwise add new fun in uiFunctions
        std::vector<void(*)(JsonObject object)>::iterator itr = find(uiFunctions.begin(), uiFunctions.end(), chFun);
        if (itr!=uiFunctions.end()) //found
          object["chFun"] = distance(uiFunctions.begin(), itr); //assign found function
        else { //not found
          uiFunctions.push_back(chFun); //add new function
          object["chFun"] = uiFunctions.size()-1;
        }
      }
      if (loopFun) {
        UserLoop loop;
        loop.loopFun = loopFun;
        loop.object = object;

        loopFunctions.push_back(loop);
        object["loopFun"] = loopFunctions.size()-1;
        userLoopsChanged = true;
        // print->print("iObject loopFun %s %u %u %d %d\n", object["id"].as<const char *>());
      }
    }
    else
      print->print("initObject could not find or create object %s with %s\n", id, type);

    return object;
  }

  //setValue char
  JsonObject SysModUI::setValue(const char * id, const char * value) {
    JsonObject object = findObject(id);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value);
        if (object["type"] == "display") { // do not update object["value"]
          setChFunAndWs(object, value); //value: bypass object["value"]
        } else {
          object["value"] = (char *)value; //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/) (otherwise crash!!)
          setChFunAndWs(object);
        }
      }
    }
    else
      print->print("setValue Object %s not found\n", id);
    return object;
  }

  //setValue int
  JsonObject SysModUI::setValue(const char * id, int value) {
    JsonObject object = findObject(id);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value);
        object["value"] = value;
        setChFunAndWs(object);
      }
    }
    else
      print->print("setValue Object %s not found\n", id);

    return object;
  }

  //setValue bool
  JsonObject SysModUI::setValue(const char * id, bool value) {
    JsonObject object = findObject(id);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value?"true":"false");
        object["value"] = value;
        setChFunAndWs(object);
      }
    }
    else
      print->print("setValue Object %s not found\n", id);
    return object;
  }

  JsonObject SysModUI::setValueP(const char * id, const char * format, ...) {
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[100];
    vsnprintf(value, sizeof(value), format, args);
    print->print("%s\n", value);

    va_end(args);

    return setValue(id, value);
  }

  JsonVariant SysModUI::getValue(const char * id) {
    JsonObject object = findObject(id);
    if (!object.isNull())
      return object["value"];
    else {
      print->print("Value of %s does not exist!!\n", id);
      return JsonVariant();
    }
  }

  //run the change function and send response to all? websocket clients
  void SysModUI::setChFunAndWs(JsonObject object, const char * value) { //value: bypass object["value"]

    if (!object["chFun"].isNull()) {//isnull needed here!
      size_t funNr = object["chFun"];
      uiFunctions[funNr](object);
    }

    JsonVariant responseVariant = (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?web->responseDoc0:web->responseDoc1)->as<JsonVariant>();
    (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?web->responseDoc0:web->responseDoc1)->clear();

    if (value)
      web->addResponse(object, "value", value);
    else {
      if (object["value"].is<int>())
        web->addResponseInt(object, "value", object["value"].as<int>());
      else if (object["value"].is<bool>())
        web->addResponseBool(object, "value", object["value"].as<bool>());
      else if (object["value"].is<const char *>())
        web->addResponse(object, "value", object["value"].as<const char *>());
      else {
        print->print("unknown type for %s\n", object["value"].as<String>().c_str());
        web->addResponse(object, "value", object["value"]);
      }
      // if (object["id"] == "pview" || object["id"] == "fx") {
      //   print->printJson("setChFunAndWs response", responseDoc);
      // }
    }

    web->sendDataWs(responseVariant);
  }

  JsonObject SysModUI::findObject(const char * id, JsonArray parent) { //static for processJson
    JsonArray root;
    // print ->print("findObject %s %s\n", id, parent.isNull()?"root":"n");
    if (parent.isNull()) {
      root = mdl->model->as<JsonArray>();
    }
    else {
      root = parent;
    }
    JsonObject foundObject;
    for(JsonObject object : root) {
      if (foundObject.isNull()) {
        if (object["id"] == id)
          foundObject = object;
        else if (!object["n"].isNull())
          foundObject = findObject(id, object["n"]);
      }
    }
    return foundObject;
  }

  const char * SysModUI::processJson(JsonVariant &json) { //static for setupJsonHandlers
    if (json.is<JsonObject>()) //should be
    {
      for (JsonPair pair : json.as<JsonObject>()) { //iterate json elements
        const char * key = pair.key().c_str();
        JsonVariant value = pair.value();

        // commands
        if (pair.key() == "uiFun") { //JsonString can do ==
          //find the dropdown object and collect it's options...
          JsonObject object = findObject(value); //value is the id
          if (!object.isNull()) {
            //call ui function...
            if (!object["uiFun"].isNull()) {//isnull needed here!
              size_t funNr = object["uiFun"];
              uiFunctions[funNr](object);
              if (object["type"] == "dropdown")
                web->addResponseInt(object, "value", object["value"]); //temp assume int only

              // print->printJson("PJ Command", responseDoc);
            }
          }
          else
            print->print("PJ Command %s object %s not found\n", key, value.as<String>().c_str());
        } 
        else { //normal change
          if (!value.is<JsonObject>()) { //no objects (inserted by uiFun responses)
            JsonObject object = findObject(key);
            if (!object.isNull())
            {
              if (object["value"] != value) { // if changed
                // print->print("processJson %s %s->%s\n", key, object["value"].as<String>().c_str(), value.as<String>().c_str());

                //set new value
                if (value.is<const char *>())
                  setValue(key, value.as<const char *>());
                else if (value.is<bool>())
                  setValue(key, value.as<bool>());
                else if (value.is<int>())
                  setValue(key, value.as<int>());
                else {
                  print->print("processJson %s %s->%s not a supported type yet\n", key, object["value"].as<String>().c_str(), value.as<String>().c_str());
                }
              }
              else if (object["type"] == "button") //button always
                setChFunAndWs(object); //setValue without assignment
            }
            else
              print->print("Object %s not found\n", key);
          }
        }
      } //for json pairs
    }
    else
      print->print("Json not object???\n");
    return nullptr;
  }