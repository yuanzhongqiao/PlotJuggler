#ifndef PLOTWIDGET_BASE_H
#define PLOTWIDGET_BASE_H

#include <QWidget>
#include "PlotJuggler/plotdata.h"
#include "PlotJuggler/plotmagnifier.h"
#include "PlotJuggler/plotzoomer.h"

#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_symbol.h"
#include "qwt_legend.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_rescaler.h"
#include "qwt_plot_panner.h"
#include "qwt_plot_legenditem.h"
#include "qwt_plot_marker.h"

namespace PJ
{

class PlotWidgetBase: public QwtPlot
{
    Q_OBJECT

public:

    struct CurveInfo
    {
        std::string src_name;
        QwtPlotCurve* curve;
        QwtPlotMarker* marker;
    };

    PlotWidgetBase(PlotDataMapRef& datamap, QWidget* parent);

    ~PlotWidgetBase();

    CurveInfo* addCurve(const std::string& name,
                        QColor color = Qt::transparent);

    bool isEmpty() const;

    void removeCurve(const QString& title);

    const std::list<CurveInfo> &curveList() const;

    QColor getColorHint(PlotData* data);

    const CurveInfo* curveFromTitle(const QString &title) const;

private:
    PlotDataMapRef& _mapped_data;

    std::list<CurveInfo> _curve_list;

     QwtPlotCurve::CurveStyle _curve_style;

};

}



#endif // PLOTWIDGET_PROXY_H
