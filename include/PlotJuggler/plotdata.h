#ifndef PLOTDATA_H
#define PLOTDATA_H

#include "plotdatabase.h"
#include "timeseries.h"
#include "stringseries.h"

namespace PJ {

using PlotDataXY = PlotDataBase<double, double>;
using PlotData = TimeseriesBase<double>;
using PlotDataAny = TimeseriesBase<std::any>;

struct PlotDataMapRef
{
  std::unordered_map<std::string, PlotData> numeric;
  std::unordered_map<std::string, PlotDataAny> user_defined;
  std::unordered_map<std::string, StringSeries> strings;

  std::unordered_map<std::string, PlotData>::iterator addNumeric(const std::string& name)
  {
    return numeric.emplace(std::piecewise_construct,
                           std::forward_as_tuple(name),
                           std::forward_as_tuple(name)).first;
  }

  std::unordered_map<std::string, PlotDataAny>::iterator addUserDefined(const std::string& name)
  {
    return user_defined.emplace(std::piecewise_construct,
                                std::forward_as_tuple(name),
                                std::forward_as_tuple(name)).first;
  }

  std::unordered_map<std::string, StringSeries>::iterator addStringSeries(const std::string& name)
  {
    return strings.emplace(std::piecewise_construct,
                           std::forward_as_tuple(name),
                           std::forward_as_tuple(name)).first;
  }

  PlotData& findOrCreateNumberic(const std::string& name)
  {
    auto it = numeric.find( name );
    if( it == numeric.end() ) {
      it = addNumeric(name);
    }
    return it->second;
  }

  StringSeries& findOrCreateStringSeries(const std::string& name)
  {
    auto it = strings.find( name );
    if( it == strings.end() ) {
      it = addStringSeries(name);
    }
    return it->second;
  }

  PlotDataAny& findOrCreateUserDefined(const std::string& name)
  {
    auto it = user_defined.find( name );
    if( it == user_defined.end() ) {
      it = addUserDefined(name);
    }
    return it->second;
  }

  void clear()
  {
    numeric.clear();
    strings.clear();
    user_defined.clear();
  }

  void setMaximumRangeX( double range )
  {
    for (auto& it : numeric)
    {
      it.second.setMaximumRangeX( range );
    }
    for (auto& it : strings)
    {
      it.second.setMaximumRangeX( range );
    }
    for (auto& it : user_defined)
    {
      it.second.setMaximumRangeX( range );
    }
  }

  bool erase(const std::string& name )
  {
    bool erased = false;
    auto num_it = numeric.find(name);
    if (num_it != numeric.end())
    {
      numeric.erase( num_it );
      erased = true;
    }

    auto str_it = strings.find(name);
    if (str_it != strings.end())
    {
      strings.erase( str_it );
      erased = true;
    }

    auto any_it = user_defined.find(name);
    if (any_it != user_defined.end())
    {
      user_defined.erase( any_it );
      erased = true;
    }
    return erased;
  }

};

template <typename Value>
inline void AddPrefixToPlotData(const std::string& prefix,
                                std::unordered_map<std::string, Value>& data)
{
  if (prefix.empty()){
    return;
  }

  std::unordered_map<std::string, Value> temp;

  for (auto& it : data)
  {
    std::string key;
    key.reserve(prefix.size() + 2 + it.first.size());
    key =  (it.first.front() == '/') ? (prefix + it.first) : (prefix + "/" + it.first);

    auto new_plot = temp.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(key),
                                 std::forward_as_tuple(key)).first;

    new_plot->second = std::move(it.second);
  }
  data = std::move(temp);
}

}

#endif // PLOTDATA_H
