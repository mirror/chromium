
#include <map>
#include <type_traits>

#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace threadsafe {

// Non-templated base type, for easy is_convertible<> checking.
class RemotableObjectBase {};

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
        // Raw ptrs and references aren't allowed.  it's unclear that we need to
        // check for references, though, since we can't seem to bind them
        // anyway without making a copy.  WeakPtrables aren't copyable because
        // of the weak ptr factory.
        value = std::is_pointer<Arg>::value && (
                std::is_convertible<Arg, RemotableObjectBase*>::value ||
                std::is_convertible<Arg, RemotableObjectBase&>::value ||
                std::is_convertible<Arg, RemotableObjectBase&&>::value
                )
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

// Uses scoped_refptr.  still allows weak_ptr<>.  maybe weak_ptr should be
// optional?  as in, it can just as well post a scoped refptr.
/*
template<typename T> class Shared : public Base<T>, base::ThreadSafeRefCounted<T> {
};
*/

// i think that we really want a RemoteObject<T>, which specializes based on T.
// if T is a WeakPtrable, then it posts weak ptrs that are resolved on the
// correct thread, but no notification at post time is provided.
//
// if T is a ThreadSafeWeakPtrable, then it should post thread-safe promoting
// wps, and Post() should return true or false about whether the object will
// get the message.  the message will have the promoted refptr.
//
// actually, does T even need any of this?  WeakPtrableRemoteObject<T> can just
// make the weak ptr factory and vend it out for T.  T doesn't have to hold
// the factory.  the disadvantage is that we can't type-check T then; one might
// send T*'s when one means to send WeakPtrRemoteObject<T>::wp's.  of course,
// if one only has a wp, then one doesn't have that option, but the owner of
// the object presumably has the object itself.
//
// then again, maybe one would not ever create a raw T.  one would create a
// WeakPtrableRemoteObject<T>(...), which would own T and only vend out wp's.
// then, one can't get it wrong, since one doesn't have a T.  of course, it
// means that the owner can't call T without posting.  might be nice for the
// WeakPtrableRemoteObject<> to support CallImmediately().  also, note that
// WeakPTrableRemoteObject<> can be un-bindable, so that the owner can't post
// the object itself.

template<class T> class weak_ptr;

template<class T> class RemotableObject : public RemotableObjectBase {
    public:
        // Note that |this| isn't T-constructed yet.
    RemotableObject() :
        task_runner_(base::SequencedTaskRunnerHandle::Get()),
       weak_factory_(static_cast<T*>(this)) {
    }

    private:
    friend class weak_ptr<T>;

    scoped_refptr<base::SequencedTaskRunner> task_runner_;
    base::WeakPtrFactory<T> weak_factory_;
};

// weak ptr with no execution guarantees.  we could also have one which does
// return true / false from post, via a scoped_refptr.  however, it requires
// that RemotableObject is a bit more heavyweight.  maybe we need different
// base classes for RemotableObject for consumers that want that.  it's also
// somewhat tricky to get right -- RemotableObject has to have nothing except
// a scoped_refptr to the real (inner) that would be what we'd actually send
// around.  that would let the original, creating object be entirely independent
// of the ptrs to it.
//
// it's not too hard to do.  rename RemotableObject, and make everything look
// for that instead.  RemotableObject would have a class member, and
// StronglyRemotableObject would have a scoped_fefptr to a dynamically
// allocated one (or would just inherit from it).
template<class T> class weak_ptr {
    public:
        // TODO(liberato): is there any case where we'd need to do this on
        // some other thread?  i doubt it.  all of these won't work.
        weak_ptr(T& thiz) : task_runner_(static_cast<RemotableObject<T>*>(&thiz)->task_runner_), wp_(static_cast<RemotableObject<T>*>(&thiz)->weak_factory_.GetWeakPtr()) {}
        weak_ptr(T* thiz) : task_runner_(static_cast<RemotableObject<T>*>(thiz)->task_runner_), wp_(static_cast<RemotableObject<T>*>(thiz)->weak_factory_.GetWeakPtr()) {}
        //weak_ptr(std::unique_ptr<RemotableObject<T> >& thiz) : task_runner_(thiz.get()->task_runner_), wp_(thiz.get()->weak_factory_.GetWeakPtr()) {}

        // TODO(liberato): is |wp_| copyable from a random thread?  this one
        // probably does need to work from anywhere.  i think that it does.
        weak_ptr(const weak_ptr& other) : wp_(other.wp_) {}

        // Calls to |wp_| on some other thread.
        template<typename Method, typename... Args> void Post(Method method, Args... args) {
            // If |Method| has any pointers / refs / etc. to RemotableObjects,
            // then assume that it's not allowed.  note that this is
            // flaky at best.  ** will not match.  not sure if we could
            // check any combination of * and &.  plus, one just has to
            // wrap a raw ptr up in a struct and it's hopeless.  not
            // sure that ptr checking makes any sense.  then again, we do that
            // for scoped_refptr already.
            static_assert(GetMadIfPointerMethod<Method>::TheResult, "WeakPtrable types must not be sent as raw ptrs.");
            task_runner_->PostTask(FROM_HERE, base::Bind(method, wp_, args...));
        }

        template<typename Method, typename... Args> void operator()(Method method, Args... args) {
            Post(method, args...);
        }

    private:
        template<typename... UnboundArgs> auto BindOnceHelper(scoped_refptr<base::SequencedTaskRunner> task_runner, base::OnceCallback<void(UnboundArgs...)> cb) {
           return base::BindOnce([](scoped_refptr<base::SequencedTaskRunner> task_runner, base::OnceCallback<void(UnboundArgs...)> cb,
                       UnboundArgs... unbound_args) {
           task_runner->PostTask(FROM_HERE, base::BindOnce(std::move(cb), unbound_args...));
           }, task_runner, std::move(cb));
        }

    public:
        // Create a thread-safe callback.
        template<typename Method, typename... Args> auto BindOnce(Method method, Args... args) {
            static_assert(GetMadIfPointerMethod<Method>::TheResult, "WeakPtrable types must not be sent as raw ptrs.");

            // |closure| will take whatever arguments are still unbound.
            auto closure = base::BindOnce(method, wp_, args...);
            return BindOnceHelper(task_runner_, std::move(closure));
        }

    private:
        template<typename... UnboundArgs> auto BindRepeatingHelper(scoped_refptr<base::SequencedTaskRunner> task_runner, base::RepeatingCallback<void(UnboundArgs...)> cb) {
           return base::BindRepeating([](scoped_refptr<base::SequencedTaskRunner> task_runner, base::RepeatingCallback<void(UnboundArgs...)> cb,
                       UnboundArgs... unbound_args) {
           task_runner->PostTask(FROM_HERE, base::BindRepeating(std::move(cb), unbound_args...));
           }, task_runner, std::move(cb));
        }

    public:
        // Create a thread-safe callback.
        template<typename Method, typename... Args> auto BindRepeating(Method method, Args... args) {
            static_assert(GetMadIfPointerMethod<Method>::TheResult, "WeakPtrable types must not be sent as raw ptrs.");

            // |closure| will take whatever arguments are still unbound.
            auto closure = base::BindRepeating(method, wp_, args...);
            return BindRepeatingHelper(task_runner_, std::move(closure));
        }

    private:
        scoped_refptr<base::SequencedTaskRunner> task_runner_;
        base::WeakPtr<T> wp_;
};

// Wrapper type for existing classes.
template<typename T> class RemotableWrapper : public RemotableObject<RemotableWrapper<T>>, public T {
    public:
        // Construct |T|
        template<typename...Args> RemotableWrapper(Args... args) : T(args...) {}
};

} // namespace threadsafe

class Foo : public threadsafe::RemotableObject<Foo> {
    public:
        Foo() {
            // Since Foo derives from RemotableObject, it can hand off wp's to
            // itself, etc.
            threadsafe::weak_ptr<Foo> wp(this);
            wp.BindOnce(&Foo::method1).Run(123);
        }

        void method1(int x) {
        }

        void method2(int* x) {
        }

        // It's not okay to send RemotableObject as an argument; use wp.
        // Note that it would still not be okay if method3 were on some other
        // WeakPtrable object, instead of Foo.  The only easy way to call
        // method3 is directly through RemotableObject.  We don't want the
        // RemotableObject to be put into a callback accidentally.
        // (note: assume that base::Bind enforces that ptrs to RemotableObjects
        // aren't allowed, like it does for raw ptrs to scoped_refptr<>)
        void method3(threadsafe::RemotableObject<Foo>* x) {
        }

        void method4(Foo& x) {
        }

        void method5(Foo&& x) {
        }
};

void fn2(threadsafe::weak_ptr<Foo> wp) {
    wp.Post(&Foo::method1, 123);
}

// fn3 cannot be called via post.
void fn3(threadsafe::RemotableObject<Foo>* x) {
}

// It would be nice if this worked, so that Goo could hand out wps to itself.
// Not sure if it's better to wrap existing types as RemotableObject versions,
// or make classes do this.  probably this, since we can always do the latter
// by creating class X : public threadsafe::RemotableObject<Y>{}
// If RO<> owns the weak factory, though, it's a bit of a problem since it
// won't tear down the wp<T> until after T's destructor runs.  then again, i
// guess that's normal.  as long as RO<T>'s destructor doesn't use the wp, it
// should be fine.  it's the only thing that runs between ~T and the destruction
// of RO<T>::weak_factory_.
/*
class Goo : public threadsafe::RemotableObject<Goo> {
};
*/

void fn() {
    // Create a RemotableObject out of Foo.
    // What doesn't work as well is if Foo wants to do this sort of thing;
    // it doesn't know that it's a remotable object.  i guess one could define
    // an abstract base class, and make an Impl : public RemotableObject<Base>.
    // It would be nicer if Impl : public RemotableObject<Impl>.
    Foo foo;
    // Since we have |foo|, we may call methods on it directly, inline.
    foo.method1(123);
    foo.method3(&foo);

    // Pretend that base::Bind shouldn't allow ptrs to RemotableObjects, like it
    // gets mad about things that are pointers to types that derive from
    // RefCountedThreadSafe.  So, one cannot post a callback that has a
    // RemotableObject directly.  Instead, create a thread-safe wp:
    threadsafe::weak_ptr<Foo> wp(foo);

    // wp's are copyable, assignable, and movable.
    threadsafe::weak_ptr<Foo> wp2(wp);
    threadsafe::weak_ptr<Foo> wp3 = wp;
    threadsafe::weak_ptr<Foo> wp4 = std::move(wp3);

    // On any thread, we may post a call to |wp|.  It might or might not run,
    // depending on whether the wp is valid.  If it does run, then it will run
    // asynchronously on the correct thread.  It would be nice to have it return
    // false if it won't run, true if it is (and embed a scoped_refptr into the
    // underlying callback to guarantee that |foo| exists).
    // Not sure which looks nicer.
    wp.Post(&Foo::method1, 123);
    wp(&Foo::method1, 123);
    
    // Some examples of ptr checking.  We don't want |foo| being put into a
    // callback, since that breaks everything.  Send wp<Foo> only.
    int x;
    wp(&Foo::method2, &x);  // ptr to int is okay.
    // Foo::method3 is not callable via wp post.
    //wp(&Foo::method3, &foo);  // ptr to Foo is not okay.  use wp instead.
    // These fail because they're not copyable.  not sure why it copies them
    // when sending to method4, which has a ref arg, though.  i guess that
    // bind sends a temporary.  Either way, it works since we doin't want to
    // be able to post references or pointers to |foo| around.
    //wp(&Foo::method4, foo);  // ref to Foo is not okay.  use wp instead.
    //wp(&Foo::method5, *foo.get());  // rvalue-ref to Foo is not okay.
    // TODO(liberato): this should fail.  base::Bind should check.
    base::Bind(&fn3, &foo);  // ptr to Foo is not okay.  use wp instead.

    // We can also create callbacks that take as arguments weak_ptr<Foo>.
    // Could post this to another thread.  note that |foo| is automatically
    // converted to a weak_ptr, since that's what fn2 wants.
    // actually, a Foo* is, a Foo and unique_ptr<Foo> aren't.  not sure why.
    base::Bind(&fn2, &foo);
    base::Bind(&fn2, wp);

    // Thread-safe callbacks.  Would be nice if Bind(&T::Method, wp<T>) did this
    // automatically, but i couldn't figure out how to do that.
    // Any of these may be run on any thread, and they'll thread-hop back.
    base::OnceCallback<void()> cb1 = wp.BindOnce(&Foo::method1, 123);
    base::OnceCallback<void(int)> cb2 = wp.BindOnce(&Foo::method1);
    base::RepeatingCallback<void()> cb3 = wp.BindRepeating(&Foo::method1, 123);
    base::RepeatingCallback<void(int)> cb4 = wp.BindRepeating(&Foo::method1);
    // It would be nice if we also had these on RemotableObject directly, so
    // that the object owner didn't have to create wp just to create callbacks.
    // Of course, wp's should also continue to have this ability; even
    // non-owners should e able to create thread-safe callbacks.

    // We can wrap existing types too.
    threadsafe::RemotableWrapper<std::map<int, int>> rmap;
    threadsafe::weak_ptr<decltype(rmap)> wp_rmap(rmap);
    base::OnceCallback<void(void)> set_map_cb = wp_rmap.BindOnce(&std::map<int,int>::clear);
}

