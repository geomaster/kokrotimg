#ifndef __KOKROTVIZ_EVENTSOURCE_H__
#define __KOKROTVIZ_EVENTSOURCE_H__

#include "delegate.h"
#include <vector>
#include <map>
#include "exceptions.h"

namespace kokrotviz {
    template<typename... Args>
    class EventSource {
    public:
        typedef uint Handle;

        class NoSuchEventHandlerException : public Exception
        {
        public:
            NoSuchEventHandlerException(Handle H) : 
                Exception("There is no such event handler"), mHandler(H)
            {

            }

            Handle getHandler() const {
                return mHandler;
            }

        protected:
            Handle mHandler;
        };

        Handle addHandler(const Delegate<void(Args...)>& Handler)
        {
            Handle nh = mNextHandle++;
            mHandlers.push_back(Handler);
            mHandleMap[nh] = mHandlers.size() - 1;

            return nh;
        }

        void removeHandler(Handle H)
        {
            typename HandleMap::iterator it = mHandleMap.find(H);
            if (it == mHandleMap.end())
                throw NoSuchEventHandlerException(H);

            typename HandlerList::size_type i = it->second, last = mHandlers.size() - 1;
            std::swap(mHandlers.at(i), mHandlers.back());
            mHandlers.pop_back();

            for (auto& p : mHandleMap)
                if (p.second == last) {
                    p.second = i;
                    break;
                }
        }

        void clearHandlers()
        {
            mHandlers.clear();
        }

        void fire(Args... Arguments)
        {
            for (auto& d : mHandlers)
                d(Arguments...);
        }

    protected:
        typedef std::vector<Delegate<void (Args...)>> HandlerList;
        typedef std::map<Handle, typename HandlerList::size_type> HandleMap;

        HandlerList mHandlers;
        HandleMap mHandleMap;
        Handle mNextHandle;
    };
}

#endif
