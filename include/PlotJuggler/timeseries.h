#ifndef TIMESERIES_RAW_H
#define TIMESERIES_RAW_H

#include "plotdata.h"
#include <algorithm>

namespace PJ {

template <typename Value>
class TimeseriesBase: public PlotDataBase<double, Value>
{
protected:
    double _max_range_x;
    using PlotDataBase<double, Value>::_points;

public:
    using Point = typename PlotDataBase<double, Value>::Point;

    TimeseriesBase(const std::string& name):
        PlotDataBase<double, Value>(name),
        _max_range_x( std::numeric_limits<double>::max() )
    { }

    void setMaximumRangeX(double max_range)
    {
        _max_range_x = max_range;
        trimRange();
    }

    double maximumRangeX() const
    {
        return _max_range_x;
    }

    int getIndexFromX(double x) const;

    std::optional<Value> getYfromX(double x) const
    {
      int index = getIndexFromX(x);
      return ( index < 0 ) ? std::nullopt : std::optional(index);
    }

    void pushBack(const Point &p)
    {
        auto temp = p;
        pushBack(std::move(temp));
    }

    void pushBack(Point&& p) override
    {
        bool need_sorting = ( !_points.empty() && p.x < this->back().x );

        if( need_sorting ) {
            auto it = std::upper_bound(_points.begin(), _points.end(), p, TimeCompare );
            PlotDataBase<double, Value>::insert( it, std::move(p) );
        }
        else{
            PlotDataBase<double, Value>::pushBack( std::move(p) );
        }
        trimRange();
    }

private:
    void trimRange()
    {
        while (_points.size() > 2 && (_points.back().x - _points.front().x) > _max_range_x)
        {
            this->popFront();
        }
    }

    static bool TimeCompare(const Point& a, const Point& b) { return a.x < b.x; }
};

//--------------------

template <typename Value>
inline int TimeseriesBase<Value>::getIndexFromX(double x) const
{
  if (_points.size() == 0)
  {
    return -1;
  }
  auto lower = std::lower_bound(_points.begin(), _points.end(), Point(x, 0), TimeCompare );
  auto index = std::distance(_points.begin(), lower);

  if (index >= _points.size())
  {
    return _points.size() - 1;
  }
  if (index < 0)
  {
    return 0;
  }

  if (index > 0)
  {
    if (std::abs(_points[index - 1].x - x) < std::abs(_points[index].x - x))
    {
      return index - 1;
    }
    else
    {
      return index;
    }
  }
  return index;
}


} // end namespace

#endif
