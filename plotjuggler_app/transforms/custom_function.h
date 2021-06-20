#pragma once

#include <memory>
#include <string>
#include <vector>
#include <QWidget>
#include <QString>
#include <QDomDocument>
#include <QString>
#include "PlotJuggler/plotdata.h"
#include "PlotJuggler/transform_function.h"

using namespace PJ;

class CustomFunction;

typedef std::shared_ptr<CustomFunction> CustomPlotPtr;
typedef std::unordered_map<std::string, CustomPlotPtr> CustomPlotMap;

struct SnippetData
{
  QString alias_name;
  QString global_vars;
  QString function;
  QString linked_source;
  QStringList additional_sources;
};

typedef std::map<QString, SnippetData> SnippetsMap;

SnippetData GetSnippetFromXML(const QDomElement& snippets_element);

SnippetsMap GetSnippetsFromXML(const QString& xml_text);

SnippetsMap GetSnippetsFromXML(const QDomElement& snippets_element);

QDomElement ExportSnippetToXML(const SnippetData& snippet, QDomDocument& destination_doc);

QDomElement ExportSnippets(const SnippetsMap& snippets, QDomDocument& destination_doc);

class CustomFunction : public PJ::TransformFunction
{
public:

  CustomFunction(const SnippetData& snippet);

  void reset() override;

  int numInputs() const override {
    return int(_used_channels.size()) + 1;
  }

  int numOutputs() const override {
    return 1;
  }

  void calculate(std::vector<PlotData*>& dst_data) override;

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;

  const SnippetData& snippet() const;

  virtual QString language() const = 0;

  virtual void initEngine() = 0;

  virtual void calculatePoints(const PlotData& src_data,
                               const std::vector<const PlotData*>& channels_data,
                               size_t point_index,
                               std::vector<PlotData::Point> &new_points) = 0;

protected:
  const SnippetData _snippet;
  const std::string _linked_plot_name;
  const std::string _plot_name;

  std::vector<std::string> _used_channels;
};
