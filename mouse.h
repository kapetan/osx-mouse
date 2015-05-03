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
		static void Initialize();
		static Persistent<Function> constructor;
		void Run();
		void HandleEvent(CGEventType type, CGEventRef event);
		void ExecuteCallback();

	private:
		MouseEvent* event;
		NanCallback* callback;
		uv_async_t* async;
		uv_mutex_t async_lock;
		uv_thread_t thread;

		explicit Mouse(NanCallback*);
		~Mouse();

		static NAN_METHOD(New);
		static NAN_METHOD(Destroy);
};
