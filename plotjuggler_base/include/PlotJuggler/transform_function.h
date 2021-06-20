#pragma once

#include <QApplication>
#include <set>
#include <functional>
#include "PlotJuggler/plotdata.h"
#include "PlotJuggler/pj_plugin.h"

namespace PJ {

// Generic interface for a multi input - multi output transformation function.
class TransformFunction : public PlotJugglerPlugin
{
  Q_OBJECT

protected:
  std::vector<const PlotData*> _src_vector;

public:
  using Ptr = std::shared_ptr<TransformFunction>;

  TransformFunction()
  {
    reset();
  }

  virtual ~TransformFunction() = default;

  virtual const char* name() const = 0;

  virtual void reset() {}

  virtual void setDataSource(const std::vector<const PlotData*>& src_data)
  {
    _src_vector = src_data;
  }

  virtual void calculate(std::vector<PlotData*>& dst_data) = 0;

signals:
  void parametersChanged();

};

// Simplified version with Single input and single output
class TransformFunction_SISO : public TransformFunction
{
  Q_OBJECT
public:

  TransformFunction_SISO() = default;

  virtual void reset() override
  {
    _last_timestamp = - std::numeric_limits<double>::max();
  }

  virtual void setDataSource(const std::vector<const PlotData*>& src_data) override
  {
    if( src_data.size() != 1 )
    {
      throw std::runtime_error("Wrong number of input data sources");
    }
    _src_vector = src_data;
  }

  virtual void calculate(std::vector<PlotData*>& dst_vector) override
  {
    if( dst_vector.size() != 1 )
    {
      throw std::runtime_error("Wrong number of output data sources");
    }

    PlotData* dst_data = dst_vector.front();
    if (dataSource()->size() == 0)
    {
      return;
    }
    dst_data->setMaximumRangeX( dataSource()->maximumRangeX() );
    if (dst_data->size() != 0)
    {
      _last_timestamp = dst_data->back().x;
    }

    int pos = dataSource()->getIndexFromX( _last_timestamp );
    size_t index = pos < 0 ? 0 : static_cast<size_t>(pos);

    while(index < dataSource()->size())
    {
      const auto& in_point = dataSource()->at(index);

      if (in_point.x >= _last_timestamp)
      {
        auto out_point = calculateNextPoint(index);
        if (out_point)
        {
          dst_data->pushBack( std::move(out_point.value()) );
        }
        _last_timestamp = in_point.x;
      }
      index++;
    }
  }

protected:

  const PlotData* dataSource()
  {
    if( _src_vector.empty() )
    {
      return nullptr;
    }
    return _src_vector.front();
  }

  virtual std::optional<PlotData::Point> calculateNextPoint(size_t index) = 0;

  double _last_timestamp;
};

///------ The factory to create instances of a SeriesTransform -------------

class TransformFactory: public QObject
{
public:
  TransformFactory() {}

private:
  TransformFactory(const TransformFactory&) = delete;
  TransformFactory& operator=(const TransformFactory&) = delete;

  std::map<std::string, std::function<TransformFunction::Ptr()>> creators_;
  std::set<std::string> names_;

  static TransformFactory* instance();

public:

  static const std::set<std::string>& registeredTransforms() {
    return instance()->names_;
  }

  template <typename T> static void registerTransform()
  {
    T temp;
    std::string name = temp.name();
    instance()->names_.insert(name);
    instance()->creators_[name] = [](){ return std::make_shared<T>(); };
  }

  static TransformFunction::Ptr create(const std::string& name)
  {
    auto it = instance()->creators_.find(name);
    if( it == instance()->creators_.end())
    {
      return {};
    }
    return it->second();
  }
};

} // end namespace

Q_DECLARE_OPAQUE_POINTER(PJ::TransformFactory *)
Q_DECLARE_METATYPE(PJ::TransformFactory *)
Q_GLOBAL_STATIC(PJ::TransformFactory, _transform_factory_ptr_from_macro)

inline PJ::TransformFactory* PJ::TransformFactory::instance()
{
  static TransformFactory * _ptr(nullptr);
  if (!qApp->property("TransformFactory").isValid() && !_ptr) {
    _ptr = _transform_factory_ptr_from_macro;
    qApp->setProperty("TransformFactory", QVariant::fromValue(_ptr));
  }
  else if (!_ptr) {
    _ptr = qvariant_cast<TransformFactory *>(qApp->property("TransformFactory"));
  }
  else if (!qApp->property("TransformFactory").isValid()) {
    qApp->setProperty("TransformFactory", QVariant::fromValue(_ptr));
  }
  return _ptr;
}


QT_BEGIN_NAMESPACE

#define TransformFunction_iid "facontidavide.PlotJuggler3.TransformFunction"
Q_DECLARE_INTERFACE(PJ::TransformFunction, TransformFunction_iid)

#define TransformFunctionSISO_iid "facontidavide.PlotJuggler3.TransformFunctionSISO"
Q_DECLARE_INTERFACE(PJ::TransformFunction_SISO, TransformFunctionSISO_iid)

QT_END_NAMESPACE

