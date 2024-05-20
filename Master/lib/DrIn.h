
#ifndef DrIn
#define DrIn


class DrawInterface {
public:

  // Pure virtual functions - These functions must be implemented by derived classes
  virtual uint8_t getCursor() = 0;

  virtual void edit(Data *d) = 0;

  virtual void pump(Pump *p, Pump *s) = 0;

  virtual void resetCursor();

  virtual bool isEditing() = 0;

  virtual void noEdit() = 0;

  virtual bool isDisplayOn() = 0;

  virtual void warn(uint8_t i, bool buzz = true) = 0;

  virtual void warn(uint8_t i, String msg) = 0;

  virtual String getWarnMsg() = 0;
};


#endif
