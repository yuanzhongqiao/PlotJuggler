#include "toolbox_FFT.h"
#include "ui_toolbox_FFT.h"

#include <QDialogButtonBox>
#include <QEvent>
#include <QMimeData>
#include <QDebug>
#include <QDragEnterEvent>

#include "PlotJuggler/transform_function.h"
#include "KissFFT/kiss_fftr.h"

ToolboxFFT::ToolboxFFT()
{
  _widget = new QWidget(nullptr);
  ui = new Ui::toolbox_fft;

  ui->setupUi(_widget);

  connect( ui->buttonBox, &QDialogButtonBox::rejected,
           this, &ToolboxPlugin::closed );

  connect( ui->radioZoomed, &QRadioButton::toggled,
          this, &ToolboxFFT::onRadioZoomedToggled );
}

ToolboxFFT::~ToolboxFFT()
{
  delete ui;
}

void ToolboxFFT::init(PJ::PlotDataMapRef &src_data,
                      PJ::TransformsMap &transform_map)
{
  _plot_data = &src_data;
  _transforms = &transform_map;

  _plot_widget_A = new PJ::PlotWidgetBase(ui->frameA);
  _plot_widget_B = new PJ::PlotWidgetBase(ui->frameB);

  auto preview_layout_A = new QHBoxLayout( ui->frameA );
  preview_layout_A->setMargin(6);
  preview_layout_A->addWidget( _plot_widget_A->widget() );

  auto preview_layout_B = new QHBoxLayout( ui->frameB );
  preview_layout_B->setMargin(6);
  preview_layout_B->addWidget( _plot_widget_B->widget() );

  _plot_widget_A->setAcceptDrops(true);

  connect( _plot_widget_A, &PlotWidgetBase::dragEnterSignal,
          this, &ToolboxFFT::onDragEnterEvent);

  connect( _plot_widget_A, &PlotWidgetBase::dropSignal,
          this, &ToolboxFFT::onDropEvent);

  connect( _plot_widget_A, &PlotWidgetBase::viewResized,
          this, &ToolboxFFT::onViewResized );
}

std::pair<QWidget*, PJ::ToolboxPlugin::WidgetType>
ToolboxFFT::providedWidget() const
{
  return { _widget, PJ::ToolboxPlugin::FIXED };
}

bool ToolboxFFT::onShowWidget()
{
  return true;
}

void ToolboxFFT::updateCurveFFT(double min, double max)
{
  const std::string& curve_id = _original_curve;

  if( _original_curve.empty() ) {
    return;
  }
  auto it = _plot_data->numeric.find( _original_curve );
  if( it == _plot_data->numeric.end() ) {
    return;
  }
  PlotData& curve_data = it->second;

  int min_index = curve_data.getIndexFromX( min );
  int max_index = curve_data.getIndexFromX( max );

  int N = 1 + max_index - min_index;
  if( N < 4 || min_index < 0 || max_index < 0 ) {
    return;
  }

  if( N & 1 ) { // if not even, make it even
    N--;
    max_index--;
  }

  float dT = float(curve_data.at(max_index).x -
                   curve_data.at(min_index).x ) /
             float(N-1);

  std::vector<kiss_fft_scalar> input;
  input.reserve( curve_data.size() );

  double std_dev = 0.0;

  for(size_t i=0; i<N; i++ ) {
    const auto& p = curve_data[i];
    input.push_back( static_cast<kiss_fft_scalar>(p.y) );

    if( i != 0) {
      double dt = ( p.x - curve_data[i-1].x );
      std_dev += dt*dt;
    }
  }
  std_dev = sqrt(std_dev) / double(N-1);

  ui->spinBoxPeriod->setValue( dT );
  ui->spinBoxStdDev->setValue( std_dev );

  std::vector<kiss_fft_cpx> out( N/2+1 );

  auto config = kiss_fftr_alloc(N, false, nullptr, nullptr);
  kiss_fftr( config, input.data(), out.data() );

  auto& curver_fft = _local_data.getOrCreateNumeric( curve_id + "/fft_freq" );
  curver_fft.clear();
  for ( int i=0; i<N/2; i++)
  {
    kiss_fft_scalar Hz = i * ( 1.0 / dT) / double(N);
    kiss_fft_scalar amplitude = std::hypot( out[i].r, out[i].i ) / N ;
    curver_fft.pushBack( {Hz, amplitude} );
  }

  _plot_widget_B->removeAllCurves();
  _plot_widget_B->addCurve( curve_id + "/fft_freq", curver_fft, Qt::blue );
  _plot_widget_B->changeCurvesStyle( PlotWidgetBase::HISTOGRAM );
  _plot_widget_B->resetZoom();

  free(config);
}

void ToolboxFFT::addCurve(std::string curve_id)
{
  PlotData& curve_data = _plot_data->getOrCreateNumeric(curve_id);
  const int N = curve_data.size();
  if( N < 2 ) {
    return;
  }
  _plot_widget_A->removeAllCurves();
  _plot_widget_A->addCurve( curve_id, curve_data );
  _plot_widget_A->resetZoom();

  _original_curve = curve_id;

  auto MAX = std::numeric_limits<double>::max();
  _view_rectangle.setLeft( -MAX/2 );
  _view_rectangle.setWidth( MAX );
  updateCurveFFT( -MAX/2, MAX/2 );
}

void ToolboxFFT::onDragEnterEvent(QDragEnterEvent *event)
{
  const QMimeData* mimeData = event->mimeData();
  QStringList mimeFormats = mimeData->formats();

  for(const QString& format : mimeFormats)
  {
    QByteArray encoded = mimeData->data(format);
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    if (format != "curveslist/add_curve") {
      return;
    }

    QStringList curves;
    while (!stream.atEnd())
    {
      QString curve_name;
      stream >> curve_name;
      if (!curve_name.isEmpty())
      {
        curves.push_back(curve_name);
      }
    }
    if( curves.size() != 1 )
    {
      return;
    }

    _dragging_curve = curves.front();
    event->acceptProposedAction();
  }
}

void ToolboxFFT::onDropEvent(QDropEvent *)
{
  std::string curve_id = _dragging_curve.toStdString();
  addCurve(curve_id);
  _dragging_curve.clear();
}

void ToolboxFFT::onViewResized(const QRectF &rect)
{
  _view_rectangle = rect;

  if( ui->radioZoomed->isChecked() ) {
    updateCurveFFT( rect.left(), rect.right() );
  }
}

void ToolboxFFT::onRadioZoomedToggled(bool checked)
{
  if( checked ) {
    updateCurveFFT( _view_rectangle.left(), _view_rectangle.right() );
  }
  else {
    auto MAX = std::numeric_limits<double>::max();
    updateCurveFFT( -MAX/2, MAX/2 );
  }
}
