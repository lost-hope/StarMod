#pragma once //as also included in ModModel
#include "Module.h"
// #include "SysModWeb.h"

typedef void(*USFun)(JsonObject);
typedef void(*LoopFun)(JsonObject, uint8_t*);

struct UserLoop {
  JsonObject object;
  uint32_t interval = 1000; //1 second default
  unsigned long lastMillis = 0;
  unsigned long counter = 10;
  size_t bufSize = 0;
  LoopFun fun;
};


class SysModUI:public Module {

public:
  static std::vector<USFun> uiFunctions;
  static std::vector<UserLoop> loopFunctions;

  SysModUI() :Module("UI") {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    success &= web->addURL("/", "/index.htm", "text/html");
    // success &= web->addURL("/index.js", "/index.js", "text/javascript");
    // success &= web->addURL("/index.css", "/index.css", "text/css");

    success &= web->setupJsonHandlers("/json", processJson);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //serve index.htm
  void setup() {
    Module::setup();

    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = initGroup(parentObject, name);
    initDisplay(parentObject, "uloops", nullptr, [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "User loops");
    });

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();
    for (auto it = begin (loopFunctions); it != end (loopFunctions); ++it) {
      if (millis() - it->lastMillis >= it->interval) {
        it->lastMillis = millis();

        //send object to notify client data coming is for object (client then knows it is canvas and expects data for it)
        // print->printObject(object); print->print("\n");
        responseDoc.clear(); //needed for deserializeJson?
        const char * id = it->object["id"];
        if (responseDoc[id].isNull()) responseDoc.createNestedObject(id);
        responseDoc[id]["value"] = true;
        // web->addResponse(object, "value", object["value"]);
        web->sendDataWs(responseDoc.as<JsonVariant>());

        // print->print("bufSize %d", it->bufSize);

        //send leds info in binary data format
        ws.cleanupClients();
        AsyncWebSocketMessageBuffer * wsBuf = ws.makeBuffer(it->bufSize*3 + 3);
        if (wsBuf) {//out of memory
          uint8_t* buffer = wsBuf->get();

          it->fun(it->object, buffer); //call the function and fill the buffer

          ws.binaryAll(wsBuf);
          wsBuf->unlock();
          ws._cleanBuffers();
        }

        it->counter++;
        // print->print("%s %u %u %d %d\n", it->object["id"].as<const char *>(), it->lastMillis, millis(), it->interval, it->counter);
      }
    }

    if (millis() - secondMillis >= 1000 || !secondMillis) {
      secondMillis = millis();
      setValueV("uloops", "%lu /s", loopFunctions[loopFunctions.size()-1].counter);
      loopFunctions[loopFunctions.size()-1].counter = 0;
    }
}

  JsonObject initGroup(JsonObject parent, const char *id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr, size_t bufSize = 100, uint32_t interval = 1000) {
    JsonObject object = initObject(parent, id, "group", uiFun, chFun, loopFun, bufSize, interval);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initMany(JsonObject parent, const char *id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr, size_t bufSize = 100, uint32_t interval = 1000) {
    JsonObject object = initObject(parent, id, "many", uiFun, chFun, loopFun, bufSize, interval);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initInput(JsonObject parent, const char *id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr, size_t bufSize = 100, uint32_t interval = 1000) {
    JsonObject object = initObject(parent, id, "input", uiFun, chFun, loopFun, bufSize, interval);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initPassword(JsonObject parent, const char *id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr, size_t bufSize = 100, uint32_t interval = 1000) {
    JsonObject object = initObject(parent, id, "password", uiFun, chFun, loopFun, bufSize, interval);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initNumber(JsonObject parent, const char *id, int value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr, size_t bufSize = 100, uint32_t interval = 1000) {
    JsonObject object = initObject(parent, id, "number", uiFun, chFun, loopFun, bufSize, interval);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initSlider(JsonObject parent, const char *id, int value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr, size_t bufSize = 100, uint32_t interval = 1000) {
    JsonObject object = initObject(parent, id, "range", uiFun, chFun, loopFun, bufSize, interval);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initCanvas(JsonObject parent, const char *id, int value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr, size_t bufSize = 100, uint32_t interval = 1000) {
    JsonObject object = initObject(parent, id, "canvas", uiFun, chFun, loopFun, bufSize, interval);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initDisplay(JsonObject parent, const char *id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr, size_t bufSize = 100, uint32_t interval = 1000) {
    JsonObject object = initObject(parent, id, "display", uiFun, chFun, loopFun, bufSize, interval);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initCheckBox(JsonObject parent, const char *id, bool value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr, size_t bufSize = 100, uint32_t interval = 1000) {
    JsonObject object = initObject(parent, id, "checkbox", uiFun, chFun, loopFun, bufSize, interval);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initButton(JsonObject parent, const char *id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr, size_t bufSize = 100, uint32_t interval = 1000) {
    JsonObject object = initObject(parent, id, "button", uiFun, chFun, loopFun, bufSize, interval);
    if (object["value"].isNull()) object["value"] = value;
    //no call of fun for buttons!!! 
    // if (chFun) chFun(object);
    return object;
  }

  JsonObject initDropdown(JsonObject parent, const char *id, uint8_t value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr, size_t bufSize = 100, uint32_t interval = 1000) {
    JsonObject object = initObject(parent, id, "dropdown", uiFun, chFun, loopFun, bufSize, interval);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initObject(JsonObject parent, const char *id, const char *type, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr, size_t bufSize = 100, uint32_t interval = 1000) {
    JsonObject object = findObject(id);

    //create new object
    if (object.isNull()) {
      print->print("initObject create new %s: %s\n", type, id);
      if (parent.isNull()) {
        JsonArray root = model.as<JsonArray>();
        object = root.createNestedObject();
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
        loop.fun = loopFun;
        loop.interval = interval;
        loop.bufSize = bufSize;
        loop.object = object;

        loopFunctions.push_back(loop);
        object["loopFun"] = loopFunctions.size()-1;
      }
    }
    else
      print->print("initObject could not find or create object %s with %s\n", id, type);

    return object;
  }

  //setValue char
  static JsonObject setValue(const char *id, const char * value) {
    JsonObject object = findObject(id);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value);
        if (object["type"] == "display") { // do not update object["value"]
          if (!object["chFun"].isNull()) {//isnull needed here!
            size_t funNr = object["chFun"];
            uiFunctions[funNr](object);
          }

          responseDoc.clear(); //needed for deserializeJson?
          web->addResponse(object, "value", value);
          web->sendDataWs(responseDoc.as<JsonVariant>());
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
  static JsonObject setValue(const char *id, int value) {
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
  static JsonObject setValue(const char *id, bool value) {
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

  //run the change function and send response to all? websocket clients
  static void setChFunAndWs(JsonObject object) {

    if (!object["chFun"].isNull()) {//isnull needed here!
      size_t funNr = object["chFun"];
      uiFunctions[funNr](object);
    }

    responseDoc.clear(); //needed for deserializeJson?
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
    //   char resStr[200]; 
    //   serializeJson(responseDoc, resStr, 200);
    //   print->print("setChFunAndWs response %s\n", resStr);
    // }

    web->sendDataWs(responseDoc.as<JsonVariant>());
  }

  //Set value with argument list
  static JsonObject setValueV(const char *id, const char * format, ...) { //static to use in *Fun
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[100];
    vsnprintf(value, sizeof(value), format, args);

    va_end(args);

    return setValue(id, value);
  }

  //Set value with argument list and print
  JsonObject setValueP(const char *id, const char * format, ...) {
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[100];
    vsnprintf(value, sizeof(value), format, args);
    print->print("%s\n", value);

    va_end(args);

    return setValue(id, value);
  }

  JsonVariant getValue(const char *id) {
    JsonObject object = findObject(id);
    if (!object.isNull())
      return object["value"];
    else {
      print->print("Value of %s does not exist!!\n", id);
      return JsonVariant();
    }
  }

  static JsonObject findObject(const char *id, JsonArray parent = JsonArray()) { //static for processJson
    JsonArray root;
    // print ->print("findObject %s %s\n", id, parent.isNull()?"root":"n");
    if (parent.isNull()) {
      root = model.as<JsonArray>();
    }
    else {
      root = parent;
    }
    JsonObject foundObject;
    for(JsonObject object : root) {
      if (foundObject.isNull()) {
        if (strcmp(object["id"], id) == 0)
          foundObject = object;
        else if (!object["n"].isNull())
          foundObject = findObject(id, object["n"]);
      }
    }
    return foundObject;
  }

  static const char * processJson(JsonVariant &json) { //static for setupJsonHandlers
    if (json.is<JsonObject>()) //should be
    {
      for (JsonPair pair : json.as<JsonObject>()) { //iterate json elements
        const char * key = pair.key().c_str();
        JsonVariant value = pair.value();

        // commands
        if (strcmp(key, "uiFun")==0) {
          //find the dropdown object and collect it's options...
          JsonObject object = findObject(value); //value is the id
          if (!object.isNull()) {
            //call ui function...
            if (!object["uiFun"].isNull()) {//isnull needed here!
              size_t funNr = object["uiFun"];
              uiFunctions[funNr](object);
              if (object["type"] == "dropdown")
                web->addResponseInt(object, "value", object["value"]); //temp assume int only

              char resStr[200]; 
              serializeJson(responseDoc, resStr, 200);
              print->print("command %s %s: %s\n", key, object["id"].as<const char *>(), resStr);
            }
          }
          else
            print->print("command %s object %s not found\n", key, value.as<String>().c_str());
        } 
        else { //normal change
          if (!value.is<JsonObject>()) { //no objects (inserted by uiFun responses)
            JsonObject object = findObject(key);
            if (!object.isNull())
            {
              if (object["value"] != value) { // if changed
                print->print("processJson %s %s->%s\n", key, object["value"].as<String>().c_str(), value.as<String>().c_str());

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
              else if (strcmp(object["type"], "button") == 0)
                setChFunAndWs(object);
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

};

static SysModUI *ui;

//init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
std::vector<void(*)(JsonObject object)> SysModUI::uiFunctions;
std::vector<UserLoop> SysModUI::loopFunctions;
