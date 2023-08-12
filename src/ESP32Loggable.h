#ifndef ESP32_LOGGABLE_H
#define ESP32_LOGGABLE_H

#include <logging.hpp>

using namespace esp32m;

class ESP32Loggable : public Loggable
{
  public:
    ESP32Loggable(const char *name) : _name(name) {
    }

  protected:
    virtual const char *logName() const { return _name; };


  private:
    const char *_name;
};


#endif