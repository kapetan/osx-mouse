#include "mouse.h"

NAN_MODULE_INIT(Initialize) {
	Mouse::Initialize(target);
}

NODE_MODULE(addon, Initialize)
