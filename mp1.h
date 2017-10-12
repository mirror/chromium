
#include <type_traits>

#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace threadsafe {
// Unique-ptr'd ownership.
// Doesn't have "promotable wp" semantics.
    /*
template<typename T> class Base {
  public:
  ThreadSafe<T> 

};
*/

//template<typename T1, typename T2> struct CheckType;
template<typename T1, typename T2 = void> struct CheckType {
       static constexpr bool type = true;
};

template<typename T1> struct CheckType<T1, typename T1::weak_ptr> {
       static constexpr bool type = false;
};

/*
template<typename First, typename... TheRest> struct GetMadIfPointer : GetMadIfPointer<TheRest...>{
};

template<> struct GetMadIfPointer<void> {
       static constexpr bool type = true;
};

// TODO: match only those arguments that are of a type that has an inner
// weak_ptr class.  then again, int* probably isn't so good either.
template<typename First, typename... TheRest> struct GetMadIfPointer<First*, TheRest...> {
        static constexpr bool type = false;
};
*/

// Non-templated base type, for easy is_convertible<> checking.
struct WeakPtrableBase{};

// If somebody refers to a WeakPtrable as a raw ptr, this will be used.
struct AmMadBecausePointer {
       static constexpr bool TheResult = false;
};

// Set |value| based on whether |Arg| is a pointer to a WeakPtrableBase.
// It would be nice to build this check into base::Bind, which already checks
// for scoped_refptr-as-rawptr (and that's where i copied this code from :) ).
// for illustration, it's separate.  that might be too strong of a restriction,
// though, if it's on the same thread.  The goal is to keep a non-wp from being
// sent to another thread, which is kind of hard.  keeping it out of callbacks
// in general might be a reasonable way to do that, though one could always
// wrap the raw ptr into a struct and pass that, and there's not much we could
// do to detect it.  of course, so might one do that with a scoped_refptr, and
// base::Bind still checks for the simple cases.
template<typename Arg> struct CheckIfWrongPtr {
    enum {
        value = std::is_pointer<Arg>::value && std::is_convertible<Arg, WeakPtrableBase*>::value
    };
};

// See if an arg list refers to a WeakPtrableBase*, since it shouldn't.
// The empty case only.  Indicates that everything is fine.
template<typename... TheRest> struct GetMadIfPointer {
       static constexpr bool TheResult = true;
};

// If |Arg| is a WeakPtrableBase* then get mad, else recurse.
template<typename Arg, typename... TheRest> struct GetMadIfPointer<Arg, TheRest...> :
    public std::conditional_t<CheckIfWrongPtr<Arg>::value, AmMadBecausePointer,
    GetMadIfPointer<TheRest...> > {};

template<typename MethodPointer> struct GetMadIfPointerMethod;

template<typename Class, typename... Args> struct GetMadIfPointerMethod<void (Class::*)(Args...)> : public GetMadIfPointer<Args...> {
};

// Inherit from this to get postable weakptrs.
template<typename T> class WeakPtrable : public WeakPtrableBase {
    public:
        // TODO(liberato): T isn't constructed yet, but this is probably okay.
        WeakPtrable() : task_runner_(base::SequencedTaskRunnerHandle::Get()),
        weak_factory_((T*)this) {}

        // maybe don't call this a weak ptr.  it's more like a "weak call poster".
        class weak_ptr {
            public:
                // TODO(liberato): is there any case where we'd need to do this on
                // some other thread?  i doubt it.  all of these won't work.
                weak_ptr(T& thiz) : task_runner_(thiz.task_runner_), wp_(thiz.weak_factory_.GetWeakPtr()) {}
                weak_ptr(T* thiz) : task_runner_(thiz->task_runner_), wp_(thiz->weak_factory_.GetWeakPtr()) {}
                weak_ptr(std::unique_ptr<T>& thiz) : task_runner_(thiz.get()->task_runner_), wp_(thiz.get()->weak_factory_.GetWeakPtr()) {}

                // TODO(liberato): is |wp_| copyable from a random thread?  this one
                // probably does need to work from anywhere.  i think that it does.
                weak_ptr(const weak_ptr& other) : wp_(other.wp_) {}

                // Calls to |this| on some other thread.
                // TODO: can we make ptrs become wp's via type matching with T?  or,
                // at least, not allow calls with T* ?  i suspect that we can prevent calls
                // with T* .
                template<typename Method, typename... Args> void Post(Method method, Args... args) {
                    // If |Method| has any pointers to things with ::weak_ptr,
                    // then assume that it's not allowed.  note that this is
                    // flaky at best.  ** will not match.  not sure if we could
                    // check any combination of * and &.  plus, one just has to
                    // wrap a raw ptr up in a struct and it's hopeless.  not
                    // sure that ptr checking makes any sense.
                    static_assert(GetMadIfPointerMethod<Method>::TheResult, "WeakPtrable types must not be sent as raw ptrs.");
                    task_runner_->PostTask(FROM_HERE, base::Bind(method, wp_, args...));
                }

                template<typename Method, typename... Args> void operator()(Method method, Args... args) {
                    Post(method, args...);
                }

            private:
                using RandomType = void;

            private:

                scoped_refptr<base::SequencedTaskRunner> task_runner_;
                base::WeakPtr<T> wp_;
        };


    private:
        scoped_refptr<base::SequencedTaskRunner> task_runner_;
        base::WeakPtrFactory<T> weak_factory_;
};

// Uses scoped_refptr.  still allows weak_ptr<>.  maybe weak_ptr should be
// optional?  as in, it can just as well post a scoped refptr.
/*
template<typename T> class Shared : public Base<T>, base::ThreadSafeRefCounted<T> {
};
*/

} // namespace threadsafe

class Foo : public threadsafe::WeakPtrable<Foo> {
    public:
        Foo() {}

        void method1(int x) {
        };

        void method2(int* x) {
        };

        // this is not an okay of sending Foo, since Foo is WeakPtrable.
        // Note that it would still not be okay if method3 were on some other
        // WeakPtrable object, instead of Foo.
        void method3(Foo* x) {
        };
};

void fn2(Foo::weak_ptr wp) {
    wp.Post(&Foo::method1, 123);
}

void fn() {
    std::unique_ptr<Foo> foo(new Foo);
    Foo::weak_ptr wp(foo);

    // Not sure which looks nicer
    wp.Post(&Foo::method1, 123);
    wp(&Foo::method1, 123);
    int x;
    wp(&Foo::method2, &x);  // ptr to int is okay, though not sure it should be.
    wp(&Foo::method3, foo.get());  // ptr to Foo is not okay.  use wp instead.

    wp.Post(&Foo::method1, 123);

    // Could post this to another thread.  note that |foo| is automatically
    // converted to a weak_ptr, since that's what fn wants.
    // actually, a Foo* is, a Foo and unique_ptr<Foo> aren't.  not sure why.
    base::Bind(&fn2, foo.get());
    base::Bind(&fn2, wp);

    // This works, since it'll auto-convert to weak_ptr.  not sure why bind
    // won't do that.
    fn2(*foo);
    fn2(foo);
    fn2(foo.get());
}

