#include "utils.h"

std::pair<std::vector<QString>, bool> MoveData(PlotDataMapRef &source,
                                               PlotDataMapRef &destination)
{
  bool destination_updated = false;
  std::vector<QString> added_curves;

  auto moveDataImpl = [&](auto& source_group, auto& destination_group)
  {
    for (auto& it : source_group)
    {
      const std::string& name = it.first;
      auto& source_plot = it.second;
      if(source_plot.size() == 0)
      {
        continue;
      }

      auto plot_with_same_name = destination_group.find(name);

      if (plot_with_same_name == destination_group.end())
      {
        added_curves.push_back(QString::fromStdString(name));

        plot_with_same_name =
            destination_group
            .emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name))
            .first;
      }
      auto& destination_plot = plot_with_same_name->second;

      // copy the attributes,
      for (const auto& attr_it: source_plot.attributes() )
      {
        destination_plot.setAttribute(attr_it.first, attr_it.second );
      }

      for (size_t i = 0; i < source_plot.size(); i++)
      {
        destination_plot.pushBack(source_plot.at(i));
        destination_updated = true;
      }
      source_plot.clear();
    }
  };

  //--------------------------------------------
  moveDataImpl( source.numeric, destination.numeric );
  moveDataImpl( source.strings, destination.strings );
  moveDataImpl( source.user_defined, destination.user_defined );

  return { added_curves, destination_updated };
}
