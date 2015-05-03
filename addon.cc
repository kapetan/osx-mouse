#include <node.h>
#include "mouse.h"

void Initialize(Handle<Object> exports, Handle<Object> module) {
	Mouse::Initialize();
	module->Set(NanNew<String>("exports"), Mouse::constructor);
}

NODE_MODULE(addon, Initialize)
