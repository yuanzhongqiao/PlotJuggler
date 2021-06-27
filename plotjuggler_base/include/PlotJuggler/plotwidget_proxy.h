#ifndef PLOTWIDGET_PROXY_H
#define PLOTWIDGET_PROXY_H


#include <QWidget>
#include "PlotJuggler/plotdata.h"

namespace PJ
{

class PlotWidgetProxy: public QWidget
{
public:
  PlotWidgetProxy(PlotDataMapRef& datamap, QWidget* parent);

  ~PlotWidgetProxy();

  std::vector<PlotDataMap::iterator> curves();

  void addCurve(const std::string& plot_name);

  void removeCurve(const std::string& plot_name);

  //void getCurves(std::vector<PlotDataMap::iterator>& curves);

private:
  struct Pimpl;
  Pimpl* pimpl;

};

}


#endif // PLOTWIDGET_PROXY_H
