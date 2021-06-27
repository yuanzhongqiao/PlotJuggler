#include "PlotJuggler/plotwidget_base.h"
#include <QBoxLayout>
#include <QMessageBox>

PJ::PlotWidgetBase::PlotWidgetBase(PJ::PlotDataMapRef &datamap, QWidget *parent)
  :_mapped_data ( datamap )
{

}

PJ::PlotWidgetBase::~PlotWidgetBase()
{

}

PJ::PlotWidgetBase::CurveInfo *PJ::PlotWidgetBase::addCurve(
    const std::string &name, QColor color)
{
 /* auto it = _mapped_data.numeric.find(name);
  if (it == _mapped_data.numeric.end())
  {
    return nullptr;
  }

  const auto qname = QString::fromStdString(name);

  // title is the same of src_name, unless a transform was applied
  auto curve_it = curveFromTitle(qname);
  if (curve_it)
  {
    return nullptr; //TODO FIXME
  }

  PlotData& data = it->second;

  auto curve = new QwtPlotCurve(qname);
  try
  {
    auto plot_qwt = createTimeSeries("", &data);

    curve->setPaintAttribute(QwtPlotCurve::ClipPolygons, true);
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsAggressive, true);
    curve->setData(plot_qwt);
  }
  catch (std::exception& ex)
  {
    QMessageBox::warning(this, "Exception!", ex.what());
    return nullptr;
  }

  if( color == Qt::transparent ){
    color = getColorHint(&data);
  }
  curve->setPen(color, (_curve_style == QwtPlotCurve::Dots) ? 4.0 : 1.3);
  curve->setStyle(_curve_style);

  curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);

  curve->attach(this);

  auto marker = new QwtPlotMarker;
  marker->attach(this);
  marker->setVisible(isXYPlot());

  QwtSymbol* sym = new QwtSymbol(QwtSymbol::Ellipse, Qt::red, QPen(Qt::black), QSize(8, 8));
  marker->setSymbol(sym);

  CurveInfo curve_info;
  curve_info.curve = curve;
  curve_info.marker = marker;
  curve_info.src_name = name;
  _curve_list.push_back( curve_info );
*/
  return &(_curve_list.back());
}
