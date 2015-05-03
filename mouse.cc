#include "mouse.h"

#include <stdio.h>

CGEventRef HandleEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* context) {
	Mouse* mouse = (Mouse*) context;
	mouse->HandleEvent(type, event);

	return NULL;
}

bool IsMouseEvent(CGEventType type) {
	return type == kCGEventLeftMouseDown ||
			type == kCGEventLeftMouseUp ||
			type == kCGEventRightMouseDown ||
			type == kCGEventRightMouseUp ||
			type == kCGEventMouseMoved ||
			type == kCGEventLeftMouseDragged ||
			type == kCGEventRightMouseDragged;
}

void RunThread(void* arg) {
	Mouse* mouse = (Mouse*) arg;
	mouse->Run();
}

void OnEvent(uv_async_t* async, int) {
	Mouse* mouse = (Mouse*) async->data;
	mouse->ExecuteCallback();
}

void OnClose(uv_handle_t* handle) {

}

Persistent<Function> Mouse::constructor;

Mouse::Mouse(NanCallback* callback) : callback(callback) {
	event = new MouseEvent();
	async = new uv_async_t;
	async->data = this;
	uv_async_init(uv_default_loop(), async, OnEvent);
	uv_mutex_init(&async_lock);
	uv_thread_create(&thread, RunThread, this);
}

Mouse::~Mouse() {
	printf("%s\n", "==============================");
	uv_mutex_destroy(&async_lock);
	if(callback) delete callback;
	if(event) delete event;
}

void Mouse::Initialize() {
	Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(Mouse::New);
	tpl->SetClassName(NanNew<String>("Mouse"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	NODE_SET_PROTOTYPE_METHOD(tpl, "destroy", Mouse::Destroy);
	NanAssignPersistent(Mouse::constructor, tpl->GetFunction());
}

void Mouse::Run() {
	CGEventMask mask = CGEventMaskBit(kCGEventLeftMouseDown) |
						CGEventMaskBit(kCGEventLeftMouseUp) |
						CGEventMaskBit(kCGEventRightMouseDown) |
						CGEventMaskBit(kCGEventRightMouseUp) |
						CGEventMaskBit(kCGEventMouseMoved) |
						CGEventMaskBit(kCGEventLeftMouseDragged) |
						CGEventMaskBit(kCGEventRightMouseDragged);

	CFMachPortRef tap = CGEventTapCreate(kCGHIDEventTap,
										 kCGHeadInsertEventTap,
										 kCGEventTapOptionListenOnly,
										 mask,
										 HandleEventCallback,
										 this);

	CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
	CGEventTapEnable(tap, true);
	CFRunLoopRun();
}

void Mouse::HandleEvent(CGEventType type, CGEventRef e) {
	if(!IsMouseEvent(type)) return;
	CGPoint location = CGEventGetLocation(e);
	uv_mutex_lock(&async_lock);
	event->x = location.x;
	event->y = location.y;
	event->type = type;
	uv_mutex_unlock(&async_lock);
	uv_async_send(async);
}

void Mouse::ExecuteCallback() {
	NanScope();

	uv_mutex_lock(&async_lock);
	MouseEvent e = {
		event->x,
		event->y,
		event->type
	};
	uv_mutex_unlock(&async_lock);

	Local<String> name;

	if(e.type == kCGEventLeftMouseDown) name = NanNew<String>("left-down");
	if(e.type == kCGEventLeftMouseUp) name = NanNew<String>("left-up");
	if(e.type == kCGEventRightMouseDown) name = NanNew<String>("right-down");
	if(e.type == kCGEventRightMouseUp) name = NanNew<String>("right-up");
	if(e.type == kCGEventMouseMoved) name = NanNew<String>("move");
	if(e.type == kCGEventLeftMouseDragged) name = NanNew<String>("left-drag");
	if(e.type == kCGEventRightMouseDragged) name = NanNew<String>("right-drag");

	Local<Value> argv[] = {
		name,
		NanNew<Number>(e.x),
		NanNew<Number>(e.y)
	};

	callback->Call(3, argv);
}

NAN_METHOD(Mouse::New) {
	NanScope();

	NanCallback* callback = new NanCallback(args[0].As<Function>());

	Mouse* obj = new Mouse(callback);
	obj->Wrap(args.This());

	NanReturnValue(args.This());
}

NAN_METHOD(Mouse::Destroy) {
	NanScope();
	NanReturnValue(NanNew<String>("mouse hello"));
}
