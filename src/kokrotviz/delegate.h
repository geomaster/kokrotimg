#ifndef __KOKROTVIZ_DELEGATE_H__
#define __KOKROTVIZ_DELEGATE_H__

namespace kokrotviz {
    template<typename Signature>
    class Delegate;

    template<typename R, typename... Args>
    class Delegate<R(Args...)> {
    public:
        Delegate() : mObject(nullptr), mStub(nullptr), mDestructor(nullptr), mCloner(nullptr)
        {

        }

        Delegate(const Delegate& Other) 
        {
            if (Other.mDestructor != nullptr) {
                mObject = Other.mCloner(Other.mObject);
            } else {
                mObject = Other.mObject;
            }

            mStub = Other.mStub;
            mDestructor = Other.mDestructor;
            mCloner = Other.mCloner;
        }

        Delegate(Delegate&& Other) 
        {
            mObject = Other.mObject;
            mDestructor = Other.mDestructor;
            mCloner = Other.mCloner;
            mStub = Other.mStub;

            Other.mDestructor = nullptr;
            Other.mObject = nullptr;
            Other.mCloner = nullptr;
            Other.mStub = nullptr;
        }

        Delegate& operator=(const Delegate& Other) {
            if (mDestructor != nullptr) {
                mDestructor(mObject);
            }

            if (Other.mDestructor != nullptr) {
                mObject = Other.mCloner(Other.mObject);
            } else {
                mObject = Other.mObject;
            }

            mStub = Other.mStub;
            mDestructor = Other.mDestructor;
            mCloner = Other.mCloner;
        }

        Delegate& operator=(Delegate&& Other) {
            if (mDestructor != nullptr) {
                mDestructor(mObject);
            }

            mObject = Other.mObject;
            mDestructor = Other.mDestructor;
            mCloner = Other.mCloner;
            mStub = Other.mStub;

            Other.mDestructor = nullptr;
            Other.mObject = nullptr;
            Other.mCloner = nullptr;
            Other.mStub = nullptr;
        }


        R operator()(Args... Arguments)
        {
            return (*mStub)(mObject, Arguments...);
        }

        template<class T, R (T::*Method)(Args...)>
        static Delegate fromMethod(T* Object)
        {
            Delegate d;
            d.mObject = Object;
            d.mStub = &methodStub<T, Method>;

            return d;
        }

        template<R (*Function)(Args...)>
        static Delegate fromFunction()
        {
            Delegate d;
            d.mObject = nullptr;
            d.mStub = &globalFunctionStub<Function>;

            return d;
        }

        template<class T>
        static Delegate fromCallable(T Callable)
        {
            Delegate d;
            d.mObject = new T(Callable);
            d.mStub = &callableStub<T>;
            d.mDestructor = &callableDestructor<T>;
            d.mCloner = &callableCloner<T>;

            return d;
        }


        ~Delegate()
        {
            if (mDestructor)
                mDestructor(mObject);
        }

    protected:
        typedef R (*Stub)(void* Object, Args...);
        typedef void (*DestructionFunction) (void* Object);
        typedef void* (*CloneFunction) (void* Object);

        void* mObject;
        DestructionFunction mDestructor;
        CloneFunction mCloner;
        Stub mStub;

        template<class T, R (T::*Method)(Args...)>
        static R methodStub(void* Object, Args... Arguments)
        {
            return (static_cast<T*>(Object)->*Method)(Arguments...);
        }

        template<R (*Function)(Args...)>
        static R globalFunctionStub(void *Object, Args... Arguments)
        {
            return Function(Arguments...);
        }

        template<class T>
        static R callableStub(void *Object, Args... Arguments)
        {
            return (*static_cast<T*>(Object))(Arguments...);
        }

        template<class T>
        static void callableDestructor(void* Object)
        {
            delete static_cast<T*>(Object);
        }

        template<class T>
        static void* callableCloner(void* Object)
        {
            return new T(*static_cast<T*>(Object));
        }
    };
}

#endif
