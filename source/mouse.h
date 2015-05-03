#include <CoreGraphics/CoreGraphics.h>
#include <node.h>
#include <node_object_wrap.h>
#include <nan.h>

using namespace v8;

struct MouseEvent {
	CGFloat x;
	CGFloat y;
	CGEventType type;
};

class Mouse : public node::ObjectWrap {
	public:
		static void Initialize(Handle<Object> exports);
		static Persistent<Function> constructor;
		void Run();
		void Stop();
		void HandleEvent(CGEventType type, CGEventRef event);
		void HandleClose();
		void HandleSend();

	private:
		MouseEvent* event;
		NanCallback* event_callback;
		NanCallback* destroy_callback;
		uv_async_t* async;
		uv_mutex_t async_lock;
		uv_thread_t thread;
		uv_cond_t async_cond;
		uv_mutex_t async_cond_lock;
		CFRunLoopRef loop_ref;
		bool stopped;
		bool finished;

		explicit Mouse(NanCallback*);
		~Mouse();

		static NAN_METHOD(New);
		static NAN_METHOD(Destroy);
		static NAN_METHOD(AddRef);
		static NAN_METHOD(RemoveRef);
};
