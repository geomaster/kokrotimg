#include "metricsTreeView.h"
#include <sstream>
#include <iomanip>
using namespace kokrotviz;


MetricsTreeView::ModelColumnRecord::ModelColumnRecord() 
{
    add(mNameColumn);
    add(mTimeColumn);
    add(mMemoryColumn);
}

MetricsTreeView::MetricsTreeView() : mColumnRecord()
{
    mTreeStore = Gtk::TreeStore::create(mColumnRecord);
}

void MetricsTreeView::clear()
{
    mTreeStore->clear();
}

void MetricsTreeView::addMetric(kok_metric_type Type, double Time, int Mem)
{
    bool isMicro = Type > KOKROTDBG_METRIC_MICRO_BASE;
    Gtk::TreeModel::Row parent = (isMicro ? *mMicrocosmItem : *mMacrocosmItem);
    double refTime = (isMicro ? mTotalMicroTime : mTotalMacroTime);
    int refMem = (isMicro ? mTotalMicroMemory : mTotalMacroMemory);

    double timePercent = 100.0 * Time / refTime, memPercent = 100.0 * Mem / refMem;

#define DEFINE_CATEGORY(cat, str) \
    case cat: { \
        category = (str); \
    } break

    std::string category = "Unknown";
    switch (Type) {
        DEFINE_CATEGORY(
                KOKROTDBG_METRIC_MACRO_CUMULATIVE_TABLE_COMPUTATION, "Cumulative table computation");
        DEFINE_CATEGORY(
                KOKROTDBG_METRIC_MACRO_BINARIZATION, "Image binarization");
        DEFINE_CATEGORY(
                KOKROTDBG_METRIC_MACRO_FINDER_CANDIDATE_SEARCH, "Finder candidate search");
        DEFINE_CATEGORY(
                KOKROTDBG_METRIC_MACRO_FINDER_SQUARE_PROCESSING, "Finder pattern processing");
        DEFINE_CATEGORY(
                KOKROTDBG_METRIC_MACRO_CODE_LOCATION_HYPOTHESIZATION, "Hypothesizing of code locations") ;
        DEFINE_CATEGORY(
                KOKROTDBG_METRIC_MACRO_ALLOCATION_OVERHEAD, "Memory allocation");
        default: break;
    }

    Gtk::TreeModel::iterator prow = mTreeStore->append(parent.children());
    Gtk::TreeModel::Row row = *prow;

    row[mColumnRecord.mNameColumn] = category;
    row[mColumnRecord.mMemoryColumn] = Glib::ustring(formatMemory(Mem, memPercent));
    row[mColumnRecord.mTimeColumn] = Glib::ustring(formatTime(Time, timePercent));

}

void MetricsTreeView::linkTreeView(Glib::RefPtr<Gtk::TreeView> tvw)
{
    tvw->set_model(mTreeStore);
    tvw->append_column("Category", mColumnRecord.mNameColumn);
    tvw->append_column("Time (ms)", mColumnRecord.mTimeColumn);
    tvw->append_column("Memory (MB)", mColumnRecord.mMemoryColumn);
}

std::string MetricsTreeView::formatTime(double time, double percent)
{
    std::ostringstream timestr;
    timestr << std::setiosflags(std::ios::fixed) << std::setprecision(2) << (time) << "ms ("
        << std::setprecision(0) << percent << "%)";

    return timestr.str();
}

std::string MetricsTreeView::formatMemory(int memory, double percent)
{
    std::ostringstream memstr;
    memstr << std::setiosflags(std::ios::fixed) << std::setprecision(2) << (memory / (1024.0 * 1024.0)) << "MB ("
        << std::setprecision(0) << percent << "%)";

    return memstr.str();
}


void MetricsTreeView::initializeView(double MacroTime, int MacroMem, double MicroTime, int MicroMem)
{
    mTotalMicroTime = MicroTime; mTotalMicroMemory = MicroMem;
    mTotalMacroTime = MacroTime; mTotalMacroMemory = MacroMem;

    clear();
    mRootItem = mTreeStore->append();

    Gtk::TreeModel::Row rootrow = *mRootItem;
    rootrow[mColumnRecord.mNameColumn] = "Total";

    int totalmem = mTotalMicroMemory + mTotalMacroMemory;
    double totaltime = mTotalMicroTime + mTotalMacroTime;

    rootrow[mColumnRecord.mMemoryColumn] = Glib::ustring(formatMemory(totalmem, 100));
    rootrow[mColumnRecord.mTimeColumn] = Glib::ustring(formatTime(totaltime, 100));

    mMacrocosmItem = mTreeStore->append(rootrow.children());
    Gtk::TreeModel::Row macrow = *mMacrocosmItem;
    macrow[mColumnRecord.mNameColumn] = "Macrocosm";
    macrow[mColumnRecord.mMemoryColumn] = Glib::ustring(formatMemory(MacroMem, 100.0 * MacroMem / totalmem));
    macrow[mColumnRecord.mTimeColumn] = Glib::ustring(formatTime(MacroTime, 100.0 * MacroTime / totaltime));

    mMicrocosmItem = mTreeStore->append(rootrow.children());
    Gtk::TreeModel::Row microw = *mMicrocosmItem;
    microw[mColumnRecord.mNameColumn] = "Microcosm";
    microw[mColumnRecord.mMemoryColumn] = Glib::ustring(formatMemory(MicroMem, 100.0 * MicroMem / totalmem));
    microw[mColumnRecord.mTimeColumn] = Glib::ustring(formatTime(MicroTime, 100.0 * MicroTime / totaltime));
}
