// PriUint64.h provides a PriUint64 class template that makes uint64_t printable.

#include <PriUint64.h>

void
demo(uint64_t x)
{
  Serial.println(PriUint64<BIN>(x));
  Serial.println(PriUint64<OCT>(x));
  Serial.println(PriUint64<DEC>(x));
  Serial.println(PriUint64<HEX>(x));
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
}

void
loop()
{
}
