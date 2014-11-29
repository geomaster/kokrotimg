#include "visualizationWindow.h"
#include "utils.h"
#include <iostream>
#include <gdkmm/general.h>
using namespace kokrotviz;

VisualizationWindow::VisualizationWindow(Gtk::Window* Window, Glib::RefPtr<Gtk::Builder> Builder, LayerManager *LayerMan) : 
    mWidget(this), mLayers(LayerMan), mPositionX(0), mPositionY(0), mZoom(1.0), mDragging(false),
    mDragStartX(0), mDragStartY(0), mDragStartPositionX(0), mDragStartPositionY(0.0)
{
    mCanvasContainer = Utils::tryFindWidget<Gtk::Box>("visReferencePoint", Builder);

    mZoomScale = Utils::tryFindWidget<Gtk::Scale>("zoomControl", Builder);
    mZoomScale->signal_value_changed().connect(sigc::mem_fun(*this, &VisualizationWindow::onZoomChanged));

    mCanvasContainer->pack_start(mWidget);

    mWidget.signal_button_press_event().connect(sigc::mem_fun(*this, &VisualizationWindow::onMouseDown));
    mWidget.signal_button_release_event().connect(sigc::mem_fun(*this, &VisualizationWindow::onMouseUp));
    mWidget.signal_motion_notify_event().connect(sigc::mem_fun(*this, &VisualizationWindow::onMouseMove));

    mWidget.add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);
    mWidget.show();
}

void VisualizationWindow::notifyLayersChange()
{
    mPixbufCache.clear();
    mPositionX = mPositionY = 0;
    mZoom = 1.0;
    mZoomScale->set_value(100.0);
}

void VisualizationWindow::drawElement(const Cairo::RefPtr<Cairo::Context>& cr, const LayerElement& el)
{
    LayerElementType typ = el.Type;
    if (typ == LET_Bitmap) {
        Glib::RefPtr<Gdk::Pixbuf> pixbuf;
        auto it = mPixbufCache.find(el.Data.B.GUID);

        if (it != mPixbufCache.end()) {
            pixbuf = it->second;
        } else {
            pixbuf = Gdk::Pixbuf::create_from_data((guint8*)el.Data.B.Data, 
                Gdk::COLORSPACE_RGB, false, 8, el.Data.B.Width, el.Data.B.Height, 3 * el.Data.B.Width);
            mPixbufCache[el.Data.B.GUID] = pixbuf;
        }

        Gdk::Cairo::set_source_pixbuf(cr, pixbuf);
        Cairo::RefPtr<Cairo::SurfacePattern>::cast_dynamic<Cairo::Pattern>(cr->get_source())->set_filter(Cairo::FILTER_NEAREST);
        cr->rectangle(0, 0, el.Data.B.Width, el.Data.B.Height);
        cr->fill();
    } else if (typ == LET_Polygon) {
        cr->move_to(el.Data.Poly.Data[0].X, el.Data.Poly.Data[0].Y);

        for (int i = 0; i < el.Data.Poly.Size; ++i) {
            int j = (i + 1) % el.Data.Poly.Size;
            cr->line_to(el.Data.Poly.Data[j].X, el.Data.Poly.Data[j].Y);
        }
        cr->stroke();
    } else if (typ == LET_Point) {
        cr->move_to(el.Data.P.X, el.Data.P.Y);
        cr->line_to(el.Data.P.X, el.Data.P.Y);
        cr->stroke();
    } else if (typ == LET_Line) {
        cr->move_to(el.Data.L.A.X, el.Data.L.A.Y);
        cr->line_to(el.Data.L.B.X, el.Data.L.B.Y);
        cr->stroke();
    }


}

void VisualizationWindow::onZoomChanged()
{
    mZoom = mZoomScale->get_value() / 100.0;
    mWidget.queue_draw();
}

void VisualizationWindow::drawLayer(const Cairo::RefPtr<Cairo::Context>& cr, const Layer& l)
{
    const std::vector<LayerElement>& elems = l.getElements();
    for (auto& elem : elems) {
        Color c = l.getColor();
        cr->set_source_rgba(c.R / 255.0, c.G / 255.0, c.B / 255.0, l.getOpacity() / 255.0);
        /* cr->set_source_rgba(1, 0, 0, 1); //l.getOpacity() / 255.0); */
        cr->set_line_cap(Cairo::LINE_CAP_ROUND); 
        cr->set_line_width(l.getLineWidth() / mZoom);

        drawElement(cr, elem);
    }
}

bool VisualizationWindow::onDraw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    std::pair<dimension, dimension> dims = mLayers->getCanvasDimensions();
    Gtk::Allocation alloc = mWidget.get_allocation();
    const int ww = alloc.get_width(), wh = alloc.get_height();

    cr->set_identity_matrix();
    cr->translate(mPositionX - mZoom * dims.first / 2 + ww / 2, mPositionY - mZoom * dims.second / 2 + wh / 2);
    cr->scale(mZoom, mZoom);

    mLayers->visitLayers(Delegate<void(kok_debug_class, Layer&)>::fromCallable(
            [this, cr] (kok_debug_class, Layer& layer) {
                if (layer.getVisible()) {
                    std::lock_guard<std::recursive_mutex> ll(layer.Mutex);
                    drawLayer(cr, layer);
                }
            }));
}

void VisualizationWindow::redraw()
{
    mWidget.queue_draw();
}

bool VisualizationWindow::onMouseMove(GdkEventMotion* evt)
{
    if (mDragging) {
        double dx = - mDragStartX + evt->x, dy = - mDragStartY + evt->y;
        mPositionX = mDragStartPositionX + dx;
        mPositionY = mDragStartPositionY + dy;
        
        mWidget.queue_draw();
    }
}

bool VisualizationWindow::onMouseDown(GdkEventButton* evt)
{
    if (evt->type == GDK_BUTTON_PRESS && evt->button == 1) {
        mDragging = true;
        mDragStartX = evt->x;
        mDragStartY = evt->y;
        mDragStartPositionX = mPositionX;
        mDragStartPositionY = mPositionY;
    }
}

bool VisualizationWindow::onMouseUp(GdkEventButton* evt)
{
    if (evt->type == GDK_BUTTON_RELEASE && evt->button == 1) {
        mDragging = false;
    }
}

