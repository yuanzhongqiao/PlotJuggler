#include "PlotJuggler/plotwidget_proxy.h"
#include <QBoxLayout>
#include "plotwidget.h"

namespace PJ
{

struct PlotWidgetProxy::Pimpl
{
  QHBoxLayout* layout;
  PlotWidget* plot_widget;

  Pimpl(PlotDataMapRef &datamap, QWidget *parent)
  {
    layout = new QHBoxLayout;
    plot_widget = new PlotWidget(datamap, nullptr);
    parent->setLayout( layout );
    layout->addWidget( plot_widget );
  }

};

PlotWidgetProxy::PlotWidgetProxy(PlotDataMapRef &datamap, QWidget *parent):
  pimpl( new Pimpl(datamap, parent) )
{

}

PlotWidgetProxy::~PlotWidgetProxy()
{
  delete pimpl;
}

std::vector<PlotDataMap::iterator> PlotWidgetProxy::curves()
{
  return {};
}

void PlotWidgetProxy::addCurve(const std::string &plot_name)
{

}

void PlotWidgetProxy::removeCurve(const std::string &plot_name)
{

}

}
