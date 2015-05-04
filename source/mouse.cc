#include "mouse.h"

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
	Mouse* mouse = (Mouse*) async->data;
	mouse->HandleClose();
}

Persistent<Function> Mouse::constructor;

Mouse::Mouse(NanCallback* callback) {
	event = new MouseEvent();
	async = new uv_async_t;
	async->data = this;
	loop_ref = NULL;
	stopped = false;
	finished = false;
	event_callback = callback;
	destroy_callback = NULL;
	uv_async_init(uv_default_loop(), async, OnSend);
	uv_mutex_init(&async_lock);
	uv_cond_init(&async_cond);
	uv_mutex_init(&async_cond_lock);
	uv_thread_create(&thread, RunThread, this);
}

Mouse::~Mouse() {
	Stop();
	uv_thread_join(&thread);

	uv_mutex_destroy(&async_lock);
	uv_cond_destroy(&async_cond);
	uv_mutex_destroy(&async_cond_lock);

	delete async;
	delete event_callback;
	delete destroy_callback;
	delete event;
}

void Mouse::Initialize(Handle<Object> exports) {
	NanScope();

	Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(Mouse::New);
	tpl->SetClassName(NanNew<String>("Mouse"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	NODE_SET_PROTOTYPE_METHOD(tpl, "destroy", Mouse::Destroy);
	NODE_SET_PROTOTYPE_METHOD(tpl, "ref", Mouse::AddRef);
	NODE_SET_PROTOTYPE_METHOD(tpl, "unref", Mouse::RemoveRef);

	NanAssignPersistent(Mouse::constructor, tpl->GetFunction());
	exports->Set(NanNew<String>("Mouse"), tpl->GetFunction());
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

	uv_mutex_lock(&async_lock);
	finished = true;
	uv_mutex_unlock(&async_lock);
	uv_async_send(async);
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

void Mouse::HandleClose() {
	if(destroy_callback) destroy_callback->Call(0, NULL);
	Unref();
}

void Mouse::HandleSend() {
	NanScope();

	uv_mutex_lock(&async_lock);
	bool f = finished;
	MouseEvent e = {
		event->x,
		event->y,
		event->type
	};
	uv_mutex_unlock(&async_lock);

	if(f) {
		uv_close((uv_handle_t*) async, OnClose);
		return;
	}

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

	event_callback->Call(3, argv);
}

NAN_METHOD(Mouse::New) {
	NanScope();

	NanCallback* callback = new NanCallback(args[0].As<Function>());

	Mouse* obj = new Mouse(callback);
	obj->Wrap(args.This());
	obj->Ref();

	NanReturnValue(args.This());
}

NAN_METHOD(Mouse::Destroy) {
	NanScope();

	Mouse* mouse = ObjectWrap::Unwrap<Mouse>(args.Holder());

	if(args[0]->IsFunction()) {
		NanCallback* callback = new NanCallback(args[0].As<Function>());
		mouse->destroy_callback = callback;
	}

	mouse->Stop();

	NanReturnUndefined();
}

NAN_METHOD(Mouse::AddRef) {
	NanScope();

	Mouse* mouse = ObjectWrap::Unwrap<Mouse>(args.Holder());
	uv_ref((uv_handle_t*) mouse->async);

	NanReturnUndefined();
}

NAN_METHOD(Mouse::RemoveRef) {
	NanScope();

	Mouse* mouse = ObjectWrap::Unwrap<Mouse>(args.Holder());
	uv_unref((uv_handle_t*) mouse->async);

	NanReturnUndefined();
}
