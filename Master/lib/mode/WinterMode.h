#ifndef ThermalWinterMode_h
#define ThermalWinterMode_h

#include "../Mode.h"
#include "../Pump.h"
#include "../Read.h"

#define WELL_DEFAULT_RUNTIME 12
#define WELL_SENSOR_FULL 20
#define WELL_SAFE_ZONE 65     // Над 65см (сензор) почваме да пълним сериозно
#define MAIN_DRAIN_LIMIT 85   // Авариен праг за горния съд

class ThermalWinterMode : public Mode {
public:
  ThermalWinterMode() {}

  const __FlashStringHelper *title() override { return F("Thermal"); }

  void exec() override {
    if (!read || !rule) return;

    float outsideTemp = read->getOutsideTemp();
    uint8_t wellLevel = read->getWellLevel();
    uint8_t mainLevel = read->getMainLevel();

    // 1. Вземаме реалното покачване от последните цикли (самообучение)
    uint8_t realRise = this->fetchRise(TARGET_RISE_CM);
    if (realRise == 0) realRise = 5; // По подразбиране 5см (~75л)

    // 2. Базови изчисления за капацитет
    uint8_t wellEmpty = (wellLevel <= WELL_SENSOR_FULL) ? 0 : (wellLevel - WELL_SENSOR_FULL);
    float driftCorrection = constrain(this->fetchWeightedCorrection(), 0.8, 1.2);
    
    // Стандартен интервал според нуждите
    uint16_t breakTimeInterval = (uint16_t)((workHours * 60) / (wellEmpty / (float)realRise + 1));

    uint8_t finalRuntime = WELL_DEFAULT_RUNTIME;

    // 3. КОМБИНИРАНА ТЕРМАЛНА ЛОГИКА
    
    // А) Ако някой от съдовете е под критичното ниво
    if (mainLevel > MAIN_DRAIN_LIMIT || wellLevel > WELL_SAFE_ZONE) {
      breakTimeInterval = MIN_BREAK_TIME;
      if (spanLg.active()) this->setWarn(F("Priority Filling..."));
    }
    // Б) Термално подгряване при ПЪЛЕН WELL (wellLevel <= WELL_SAFE_ZONE)
    else if (outsideTemp <= -3.0) {
      // Динамичен мапинг: Колкото по-студено е, толкова по-често въртим.
      // Колкото по-голям е realRise (повече топла вода), толкова по-дълго чакаме.
      
      int tempIn = constrain((int)outsideTemp, -15, -3);
      
      // Базов престой от 180 до 600 мин
      uint16_t basePulseBreak = map(tempIn, -15, -3, 120, 480);
      
      // Корекция спрямо количеството вкарана вода:
      // Ако realRise е 10см вместо 5см, удвояваме времето за чакане
      float volumeFactor = (float)realRise / 5.0; 
      breakTimeInterval = (uint16_t)(basePulseBreak * volumeFactor);
      
      finalRuntime = 8; // Кратък импулс
    }
    // В) Топло време и пълни съдове
    else if (wellLevel <= WELL_SENSOR_FULL) {
      breakTimeInterval = 1440; // 24ч престой
    }

    // 4. Финално изчисляване на Runtime (ако не е 8 мин импулс)
    if (finalRuntime == WELL_DEFAULT_RUNTIME) {
      uint16_t cappedBreak = constrain(breakTimeInterval, 45, 720);
      finalRuntime = (uint8_t)((WELL_DEFAULT_RUNTIME * driftCorrection) + map(cappedBreak, 45, 720, 0, 8));
    }

    finalRuntime = constrain(finalRuntime, 8, 22);

    if (spanLg.active()) {
      dbg(F("[THERMAL] T: ")); dbg(outsideTemp);
      dbg(F(" | Rise: ")); dbg(realRise);
      dbg(F("cm | Break: ")); dbg(breakTimeInterval);
      dbgLn(F("m"));
    }

    this->pumpWell(finalRuntime, (unsigned long)breakTimeInterval);
  }
};
#endif