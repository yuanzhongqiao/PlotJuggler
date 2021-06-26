#ifndef PJ_TOOLBOX_BASE_H
#define PJ_TOOLBOX_BASE_H

#include <QAction>

#include <functional>
#include "PlotJuggler/plotdata.h"
#include "PlotJuggler/pj_plugin.h"
#include "PlotJuggler/transform_function.h"

namespace PJ {

class ToolboxPlugin : public PlotJugglerPlugin
{
  Q_OBJECT

public:

  ToolboxPlugin() = default;

  virtual void setDataSource( PlotDataMapRef& src_data, TransformsMap& transform_map)
  {
    _src_data = &src_data;
    _transform_map = &transform_map;
  }

  virtual ~ToolboxPlugin() = default;

  enum WidgetType {
    FLOATING,
    FIXED
  };

  std::pair<QWidget*, WidgetType> providedWidget() const;

public slots:

  virtual bool onShowWidget(PlotDataMapRef& data) = 0;

signals:

  void plotCreated(QString plot_name, bool is_custom);

protected:
  PlotDataMapRef* _src_data = nullptr;
  TransformsMap* _transform_map = nullptr;
};

using ToolboxPluginPtr = std::shared_ptr<ToolboxPlugin>;

}

QT_BEGIN_NAMESPACE
#define Toolbox_iid "facontidavide.PlotJuggler3.Toolbox"
Q_DECLARE_INTERFACE(PJ::ToolboxPlugin, Toolbox_iid)
QT_END_NAMESPACE

#endif
