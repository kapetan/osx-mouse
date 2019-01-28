#ifndef _MOUSE_H
#define _MOUSE_H

#include <ApplicationServices/ApplicationServices.h>
#include <node.h>
#include <nan.h>

using namespace v8;

struct MouseEvent {
	CGFloat x;
	CGFloat y;
	CGEventType type;
};

const unsigned int BUFFER_SIZE = 10;

class Mouse : public Nan::ObjectWrap {
	public:
		static void Initialize(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE exports);
		static Nan::Persistent<Function> constructor;
		void Run();
		void Stop();
		void HandleEvent(CGEventType type, CGEventRef event);
		void HandleClose();
		void HandleSend();

	private:
		Nan::Callback* event_callback;
		Nan::AsyncResource* async_resource;
		uv_async_t* async;
		uv_mutex_t async_lock;
		uv_thread_t thread;
		uv_cond_t async_cond;
		uv_mutex_t async_cond_lock;
		CFRunLoopRef loop_ref;
		bool stopped;
		MouseEvent* eventBuffer[BUFFER_SIZE];
		unsigned int readIndex;
		unsigned int writeIndex;

		explicit Mouse(Nan::Callback*);
		~Mouse();

		static NAN_METHOD(New);
		static NAN_METHOD(Destroy);
		static NAN_METHOD(AddRef);
		static NAN_METHOD(RemoveRef);
};

#endif
