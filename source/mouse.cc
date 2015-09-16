#include "mouse.h"

const char* LEFT_DOWN = "left-down";
const char* LEFT_UP = "left-up";
const char* RIGHT_DOWN = "right-down";
const char* RIGHT_UP = "right-up";
const char* MOVE = "move";
const char* LEFT_DRAG = "left-drag";
const char* RIGHT_DRAG = "right-drag";

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

CGEventRef OnMouseEvent(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* context) {
	Mouse* mouse = (Mouse*) context;
	mouse->HandleEvent(type, event);

	return NULL;
}

NAUV_WORK_CB(OnSend) {
	Mouse* mouse = (Mouse*) async->data;
	mouse->HandleSend();
}

void OnClose(uv_handle_t* handle) {
	uv_async_t* async = (uv_async_t*) handle;
	delete async;
}

Nan::Persistent<Function> Mouse::constructor;

Mouse::Mouse(Nan::Callback* callback) {
	event = new MouseEvent();
	async = new uv_async_t;
	async->data = this;
	loop_ref = NULL;
	stopped = false;
	event_callback = callback;
	uv_async_init(uv_default_loop(), async, OnSend);
	uv_mutex_init(&async_lock);
	uv_cond_init(&async_cond);
	uv_mutex_init(&async_cond_lock);
	uv_thread_create(&thread, RunThread, this);
}

Mouse::~Mouse() {
	Stop();

	uv_mutex_destroy(&async_lock);
	uv_cond_destroy(&async_cond);
	uv_mutex_destroy(&async_cond_lock);

	delete event_callback;
	delete event;
}

void Mouse::Initialize(Handle<Object> exports) {
	Nan::HandleScope scope;

	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(Mouse::New);
	tpl->SetClassName(Nan::New<String>("Mouse").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	Nan::SetPrototypeMethod(tpl, "destroy", Mouse::Destroy);
	Nan::SetPrototypeMethod(tpl, "ref", Mouse::AddRef);
	Nan::SetPrototypeMethod(tpl, "unref", Mouse::RemoveRef);

	Mouse::constructor.Reset(tpl->GetFunction());
	exports->Set(Nan::New<String>("Mouse").ToLocalChecked(), tpl->GetFunction());
}

void Mouse::Run() {
	CFRunLoopRef ref = CFRunLoopGetCurrent();
	CGEventMask mask = CGEventMaskBit(kCGEventLeftMouseDown) |
		CGEventMaskBit(kCGEventLeftMouseUp) |
		CGEventMaskBit(kCGEventRightMouseDown) |
		CGEventMaskBit(kCGEventRightMouseUp) |
		CGEventMaskBit(kCGEventMouseMoved) |
		CGEventMaskBit(kCGEventLeftMouseDragged) |
		CGEventMaskBit(kCGEventRightMouseDragged);

	CFMachPortRef tap = CGEventTapCreate(
		kCGHIDEventTap,
		kCGHeadInsertEventTap,
		kCGEventTapOptionListenOnly,
		mask,
		OnMouseEvent,
		this);

	CFRunLoopSourceRef source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
	CFRunLoopAddSource(ref, source, kCFRunLoopCommonModes);
	CGEventTapEnable(tap, true);

	uv_mutex_lock(&async_cond_lock);
	loop_ref = ref;
	uv_cond_signal(&async_cond);
	uv_mutex_unlock(&async_cond_lock);

	CFRunLoopRun();

	CGEventTapEnable(tap, false);
	CFRunLoopRemoveSource(ref, source, kCFRunLoopCommonModes);
	CFRelease(source);
	CFRelease(tap);
}

void Mouse::Stop() {
	if(stopped) return;
	stopped = true;

	uv_mutex_lock(&async_cond_lock);
	while(loop_ref == NULL) uv_cond_wait(&async_cond, &async_cond_lock);
	CFRunLoopRef ref = loop_ref;
	uv_mutex_unlock(&async_cond_lock);

	CFRunLoopPerformBlock(ref, kCFRunLoopCommonModes, ^{
		CFRunLoopRef current = CFRunLoopGetCurrent();
		CFRunLoopStop(current);
	});

	CFRunLoopWakeUp(ref);
	uv_close((uv_handle_t*) async, OnClose);
	uv_thread_join(&thread);
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

void Mouse::HandleSend() {
	if(stopped) return;

	Nan::HandleScope scope;

	uv_mutex_lock(&async_lock);
	MouseEvent e = {
		event->x,
		event->y,
		event->type
	};
	uv_mutex_unlock(&async_lock);

	const char* name;

	if(e.type == kCGEventLeftMouseDown) name = LEFT_DOWN;
	if(e.type == kCGEventLeftMouseUp) name = LEFT_UP;
	if(e.type == kCGEventRightMouseDown) name = RIGHT_DOWN;
	if(e.type == kCGEventRightMouseUp) name = RIGHT_UP;
	if(e.type == kCGEventMouseMoved) name = MOVE;
	if(e.type == kCGEventLeftMouseDragged) name = LEFT_DRAG;
	if(e.type == kCGEventRightMouseDragged) name = RIGHT_DRAG;

	Local<Value> argv[] = {
		Nan::New<String>(name).ToLocalChecked(),
		Nan::New<Number>(e.x),
		Nan::New<Number>(e.y)
	};

	event_callback->Call(3, argv);
}

NAN_METHOD(Mouse::New) {
	Nan::Callback* callback = new Nan::Callback(info[0].As<Function>());

	Mouse* obj = new Mouse(callback);
	obj->Wrap(info.This());
	obj->Ref();

	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Mouse::Destroy) {
	Mouse* mouse = Nan::ObjectWrap::Unwrap<Mouse>(info.Holder());
	mouse->Stop();
	mouse->Unref();

	info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Mouse::AddRef) {
	Mouse* mouse = Nan::ObjectWrap::Unwrap<Mouse>(info.Holder());
	uv_ref((uv_handle_t*) mouse->async);

	info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Mouse::RemoveRef) {
	Mouse* mouse = Nan::ObjectWrap::Unwrap<Mouse>(info.Holder());
	uv_unref((uv_handle_t*) mouse->async);

	info.GetReturnValue().SetUndefined();
}
