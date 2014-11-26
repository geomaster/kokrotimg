#ifndef __KOKROTVIZ_VISUALIZATIONWINDOW_H__
#define __KOKROTVIZ_VISUALIZATIONWINDOW_H__
#include <gtkmm/builder.h>
#include <gtkmm/window.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/box.h>
#include <cairomm/context.h>
#include <gtkmm/scale.h>
#include "layerManager.h"

namespace kokrotviz
{
    class VisualizationWindow 
    {
    public:
        VisualizationWindow(Gtk::Window* Window, Glib::RefPtr<Gtk::Builder> Builder, LayerManager *LayerMan);
        void redraw();
        void notifyLayersChange();

    protected:
        Gtk::Box *mCanvasContainer;
        Gtk::Scale *mZoomScale;
        std::map<uint64_t, Glib::RefPtr<Gdk::Pixbuf> > mPixbufCache;

        double mPositionX, mPositionY, mDragStartPositionX, mDragStartPositionY, mDragStartX, mDragStartY, mZoom;
        bool mDragging;
        LayerManager *mLayers;

        bool onDraw(const Cairo::RefPtr<Cairo::Context>& cr);

        class VisualizationWidget : public Gtk::DrawingArea
        {
        public:
            VisualizationWidget(VisualizationWindow* Parent) : mParent(Parent) { ; }
            virtual ~VisualizationWidget() { ; }

            virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override { return mParent->onDraw(cr); }

        protected:
            VisualizationWindow* mParent;

        } mWidget;

        void drawLayer(const Cairo::RefPtr<Cairo::Context>& cr, const Layer& l);
        void drawElement(const Cairo::RefPtr<Cairo::Context>& cr, const LayerElement& el);

        void onZoomChanged();
        bool onMouseMove(GdkEventMotion*);
        bool onMouseDown(GdkEventButton*);
        bool onMouseUp(GdkEventButton*);
    };
}

#endif
