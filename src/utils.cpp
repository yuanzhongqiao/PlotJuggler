#include "utils.h"
#include <QDebug>

std::pair<std::vector<std::string>, bool>
MoveData(PlotDataMapRef &source,
         PlotDataMapRef &destination,
         bool remove_older )
{

  std::vector<std::string> added_curves;

  auto moveDataImpl = [&](auto& source_group, auto& destination_group)
  {
    bool need_update = false;
    for (auto& it : source_group)
    {
      const std::string& name = it.first;
      auto& source_plot = it.second;

      auto dest_plot_it = destination_group.find(name);
      if (dest_plot_it == destination_group.end())
      {
        added_curves.push_back(name);

        dest_plot_it =
            destination_group
            .emplace(std::piecewise_construct,
                     std::forward_as_tuple(name),
                     std::forward_as_tuple(name))
            .first;
        need_update = true;
      }
      auto& destination_plot = dest_plot_it->second;

      // copy the attributes, at the series and group level
      for (const auto& [name, attr]: source_plot.attributes() )
      {
        if( destination_plot.attribute(name) != attr ){
          destination_plot.setAttribute( name, attr );
          need_update = true;
        }
      }
      if( source_plot.group() && !destination_plot.group() )
      {
        destination_plot.setGroup( source_plot.group() );
        need_update = true;
      }

      if( remove_older ) {
        destination_plot.clear();
      }

      for (size_t i = 0; i < source_plot.size(); i++)
      {
        destination_plot.pushBack( source_plot.at(i) );
      }

      double max_range_x = source_plot.maximumRangeX();
      destination_plot.setMaximumRangeX(max_range_x);

      source_plot.clear();
    }
    return need_update;
  };

  //--------------------------------------------
  bool num_updated = moveDataImpl( source.numeric, destination.numeric );
  bool str_updated = moveDataImpl( source.strings, destination.strings );

  moveDataImpl( source.user_defined, destination.user_defined );

  return { added_curves, num_updated || str_updated };
}
