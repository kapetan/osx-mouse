#include "mouse.h"

NODE_MODULE_INIT() {
	Mouse::Initialize(exports, module, context);
}
