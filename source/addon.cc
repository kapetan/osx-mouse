#include <node.h>
#include "mouse.h"

void Initialize(Handle<Object> exports) {
	Mouse::Initialize(exports);
}

NODE_MODULE(addon, Initialize)
