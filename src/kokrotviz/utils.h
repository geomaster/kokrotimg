#ifndef __KOKROTVIZ_UTILS_H__
#define __KOKROTVIZ_UTILS_H__
#include <gtkmm/builder.h>
#include "exceptions.h"

namespace kokrotviz {
    class Utils {
    public:
        template<typename T>
        static T* tryFindWidget(const char* Name, Glib::RefPtr<Gtk::Builder> Builder)
        {
            T* ptr = NULL;
            Builder->get_widget(Name, ptr);
            if (!ptr) {
                throw UnexpectedException(std::string("Widget '") + Name + "' not found!");
            }

            return ptr;
        }
    };
}
#endif
