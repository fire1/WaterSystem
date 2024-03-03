
#ifndef Loop_h
#define Loop_h
class Span {

private:
  bool active = false;
  uint16_t length;
  unsigned long index;


public:

  Span(uint16_t len)
    : length(len) {}

  bool isActive() {
    return active;
  }

  void tick() {
    if (index >= length) {
      index = 0;
      active = true;
      return;
    } else
      index++;
    active = false;
  }
};



#endif