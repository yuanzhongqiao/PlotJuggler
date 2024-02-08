#include "samples_count.h"
#include "ui_samples_count.h"

#include <QSpinBox>

SamplesCountFilter::SamplesCountFilter()
    : ui(new Ui::SamplesCount)
    , _widget(new QWidget())
{
    ui->setupUi(_widget);

    connect(ui->spinBoxMilliseconds,
            qOverload<int>(&QSpinBox::valueChanged), this,
            [=](int) { emit parametersChanged(); });
}

SamplesCountFilter::~SamplesCountFilter()
{
    delete ui;
    delete _widget;
}

QWidget* SamplesCountFilter::optionsWidget()
{
    return _widget;
}

bool SamplesCountFilter::xmlSaveState(QDomDocument& doc,
                                        QDomElement& parent_element) const
{
    QDomElement widget_el = doc.createElement("options");
    widget_el.setAttribute("milliseconds", ui->spinBoxMilliseconds->value());
    parent_element.appendChild(widget_el);
    return true;
}

bool SamplesCountFilter::xmlLoadState(const QDomElement& parent_element)
{
    QDomElement widget_el = parent_element.firstChildElement("options");
    if(widget_el.isNull())
    {
        return false;
    }
    int ms = widget_el.attribute("milliseconds", "1000").toInt();
    ui->spinBoxMilliseconds->setValue(ms);
    return true;
}

std::optional<PJ::PlotData::Point> SamplesCountFilter::calculateNextPoint(size_t index)
{
    if(dataSource()->size() == 0) {
        return std::nullopt;
    }
    const double delta = 0.001 * double(ui->spinBoxMilliseconds->value());
    const auto& point = dataSource()->at(index);
    if(index == 0) {
        interval_end_ = point.x + delta;
        count_ = 0;
    }
    count_++;

    if(point.x > interval_end_ || index == (dataSource()->size() - 1))
    {
        auto out = PJ::PlotData::Point{ interval_end_, double(count_) };
        while(point.x > interval_end_) {
            interval_end_ += delta;
        }
        count_ = 0;
        return out;
    }
    return std::nullopt;
}
