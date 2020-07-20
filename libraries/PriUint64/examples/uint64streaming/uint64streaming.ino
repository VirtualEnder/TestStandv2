// PriUint64.h can integrate with Streaming 5 library.
// To enable integration, include Streaming.h before PriUint64.h.
//
// The baseline integration only overloads operator<< for uint64_t. On platforms where
// <type_traits> header is available, PriUint64.h also rewrites _HEX _DEC _OCT _BIN macros
// so that they can work with uint64_t. `#define PRIUINT64_OVERRIDE_STREAMING_BASED` forces
// this rewriting, but it would cause compile error if <type_traits> is unavailable.

#include <Streaming.h> // http://arduiniana.org/libraries/streaming/
#include <PriUint64.h>

void
demo(uint64_t x)
{
  Serial << x << endl;
#ifdef PRIUINT64_OVERRIDE_STREAMING_BASED
  Serial << _HEX(x) << endl;
  Serial << _DEC(x) << endl;
  Serial << _OCT(x) << endl;
  Serial << _BIN(x) << endl;
#endif // PRIUINT64_OVERRIDE_STREAMING_BASED
  Serial.println();
}

void
setup()
{
  Serial.begin(115200);
  Serial.println();

  demo(0x0);
  demo(0x10000);
  demo(0x9aa567b200);
  demo(0xffffffffffffffff);

  // Streaming of non-uint64 types still works.
  Serial << _HEX(31) << endl;
  Serial << _DEC(31) << endl;
  Serial << _OCT(31) << endl;
  Serial << _BIN(31) << endl;
  Serial << endl;
}

void
loop()
{
}
