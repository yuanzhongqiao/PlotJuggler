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

  connect( ui->pushButtonSave, &QPushButton::clicked,
          this, &ToolboxFFT::onSaveCurve );
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
  const std::string& curve_id = _curve_name;

  if( _curve_name.empty() ) {
    return;
  }
  auto it = _plot_data->numeric.find( _curve_name );
  if( it == _plot_data->numeric.end() ) {
    return;
  }
  PlotData& curve_data = it->second;

  int min_index = curve_data.getIndexFromX( min );
  int max_index = curve_data.getIndexFromX( max );

  double min_t = curve_data.at(min_index).x;
  double max_t = curve_data.at(max_index).x;

  const double EPS = std::numeric_limits<double>::epsilon();
  if ( std::abs(min_t - _prev_range.min) <= EPS &&
       std::abs(max_t - _prev_range.max) <= EPS )
  {
    return;
  }

  _prev_range.min = min_t;
  _prev_range.max = max_t;

  int N = 1 + max_index - min_index;
  if( N < 4 || min_index < 0 || max_index < 0 ) {
    return;
  }

  if( N & 1 ) { // if not even, make it even
    N--;
    max_index--;
  }

  double dT = (curve_data.at(max_index).x -
               curve_data.at(min_index).x ) /
              double(N-1);

  std::vector<kiss_fft_scalar> input;
  input.reserve( curve_data.size() );

  double std_dev = 0.0;

  for(size_t i=0; i<N; i++ ) {
    const auto& p = curve_data[i];
    input.push_back( static_cast<kiss_fft_scalar>(p.y) );

    if( i != 0) {
      double dTi = ( p.x - curve_data[i-1].x );
      double diff = dTi - dT;
      std_dev += diff*diff;
    }
  }
  std_dev = sqrt(std_dev / double(N-1) );

  ui->lineEditDT->setText( QString::number( dT, 'f' ) );
  ui->lineEditStdDev->setText( QString::number( std_dev, 'f' ) );

  std::vector<kiss_fft_cpx> out( N/2+1 );

  auto config = kiss_fftr_alloc(N, false, nullptr, nullptr);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  kiss_fftr( config, input.data(), out.data() );
  QApplication::restoreOverrideCursor();

  auto& curver_fft = _local_data.getOrCreateNumeric( curve_id );
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

  _curve_name = curve_id;

  _zoom_range.min = curve_data.front().x;
  _zoom_range.max = curve_data.back().x;
  updateCurveFFT( _zoom_range.min, _zoom_range.max );

  ui->pushButtonSave->setEnabled( true );
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
  _zoom_range.min = rect.left();
  _zoom_range.max = rect.right();

  if( ui->radioZoomed->isChecked() ) {
    updateCurveFFT( rect.left(), rect.right() );
  }
}

void ToolboxFFT::onRadioZoomedToggled(bool checked)
{
  if( checked ) {
    updateCurveFFT( _zoom_range.min, _zoom_range.max );
  }
  else {
    auto MAX = std::numeric_limits<double>::max();
    updateCurveFFT( -MAX/2, MAX/2 );
  }
}

void ToolboxFFT::onSaveCurve()
{
  auto it = _local_data.numeric.find( _curve_name );
  if( it == _local_data.numeric.end() ) {
    return;
  }

  auto& out_data = _plot_data->getOrCreateNumeric( _curve_name + "_FFT" );
  out_data.clone( it->second );

  out_data.setAttribute( "disable_linked_zoom", "true" );

  emit plotCreated( _curve_name + "_FFT" );
  emit closed();
}
