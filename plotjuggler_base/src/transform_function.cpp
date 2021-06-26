#include "PlotJuggler/transform_function.h"

namespace PJ {

TransformFunction::TransformFunction():
  _data(nullptr)
{
  reset();
}

void TransformFunction::setDataSource(PlotDataMapRef *data)
{
  if( numInputs() > 0 )
  {
    throw std::runtime_error("When numInputs() > 0, the method "
                             "setDataSource(const std::vector<const PlotData*>&) "
                             "should be used.");
  }
  _data = data;
}

void TransformFunction::setDataSource(const std::vector<const PlotData *> &src_data)
{
  if( src_data.size() != numInputs() )
  {
    throw std::runtime_error("Wrong number of input data sources "
                             "in setDataSource");
  }
  _src_vector = src_data;
}

void TransformFunction_SISO::reset() {
  _last_timestamp = - std::numeric_limits<double>::max();
}

void TransformFunction_SISO::calculate(std::vector<PlotData *> &dst_vector)
{
  if( dst_vector.size() != numOutputs() )
  {
    throw std::runtime_error("Wrong number of output data destinations");
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

const PlotData *TransformFunction_SISO::dataSource()
{
  if( _src_vector.empty() )
  {
    return nullptr;
  }
  return _src_vector.front();
}

TransformFunction::Ptr TransformFactory::create(const std::string &name)
{
  auto it = instance()->creators_.find(name);
  if( it == instance()->creators_.end())
  {
    return {};
  }
  return it->second();
}

TransformFactory *PJ::TransformFactory::instance()
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

const std::set<std::string> &TransformFactory::registeredTransforms() {
  return instance()->names_;
}


}



