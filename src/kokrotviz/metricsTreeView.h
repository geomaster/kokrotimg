#ifndef __KOKROTVIZ_METRICSTREEVIEW_H__
#define __KOKROTVIZ_METRICSTREEVIEW_H__
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <string>
#include <kokrotimg/kokrotimg.h>

namespace kokrotviz 
{
    class MetricsTreeView
    {
    public:
        MetricsTreeView();

        void linkTreeView(Glib::RefPtr<Gtk::TreeView> tvw);
        void clear();

        void initializeView(double MacroTime, int MacroMem, double MicroTime, int MicroMem);
        void addMetric(kok_metric_type Type, double Time, int Mem);

    protected:
        class ModelColumnRecord : public Gtk::TreeModelColumnRecord
        {
        public:
            ModelColumnRecord();

            Gtk::TreeModelColumn<Glib::ustring> mNameColumn, mTimeColumn, mMemoryColumn;
        } mColumnRecord;

        Glib::RefPtr<Gtk::TreeStore> mTreeStore;
        double mTotalMacroTime, mTotalMicroTime;
        int mTotalMacroMemory, mTotalMicroMemory;

        Gtk::TreeModel::iterator mRootItem, mMacrocosmItem, mMicrocosmItem;
        std::string formatTime(double time, double percent);
        std::string formatMemory(int memory, double percent);
    };
}


#endif 
