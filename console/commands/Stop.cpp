#ifdef CONSOLE

#include "Stop.h"

Stop::Stop() : mouse(ConsoleMouse::inst()) {}

void Stop::initialize(){
}

void Stop::execute(){
}

bool Stop::isFinished(){
  return true;
}

void Stop::end(){
}
#endif