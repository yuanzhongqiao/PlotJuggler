#include "toolbox_quaternion.h"
#include "ui_quaternion_to_rpy.h"

#include <QDialogButtonBox>
#include <QEvent>
#include <QMimeData>
#include <QDragEnterEvent>

#include "PlotJuggler/transform_function.h"

class QuaternionToRollPitchYaw: public PJ::TransformFunction
{
public:

  const char* name() const override {
    return "quaternion_to_RPY";
  }

  void reset() override {
    _prev_roll = 0;
    _prev_yaw = 0;
    _prev_pitch = 0;
    _roll_offset = 0;
    _pitch_offset = 0;
    _yaw_offset = 0;
    _last_timestamp = - std::numeric_limits<double>::max();
  }

  int numInputs() const override {
    return 4;
  }

  int numOutputs() const override {
    return 3;
  }

  void setScale(double scale)
  {
    _scale = scale;
  }

  void setWarp(bool wrap)
  {
    _wrap = wrap;
  }

  void calculate() override
  {
    auto& data_x = *_src_vector[0];
    auto& data_y = *_src_vector[1];
    auto& data_z = *_src_vector[2];
    auto& data_w = *_src_vector[3];

    auto& data_roll  = *_dst_vector[0];
    auto& data_pitch = *_dst_vector[1];
    auto& data_yaw   = *_dst_vector[2];

    data_roll.setMaximumRangeX( data_x.maximumRangeX() );
    data_pitch.setMaximumRangeX( data_x.maximumRangeX() );
    data_yaw.setMaximumRangeX( data_x.maximumRangeX() );

    data_roll.clear();
    data_pitch.clear();
    data_yaw.clear();

    if( data_x.size() == 0 ||
        data_x.size() != data_y.size() ||
        data_y.size() != data_z.size() ||
        data_z.size() != data_w.size() )
    {
      return;
    }

    int pos = data_x.getIndexFromX( _last_timestamp );
    size_t index = pos < 0 ? 0 : static_cast<size_t>(pos);

    while(index < data_x.size())
    {
      auto& point_x = data_x.at(index);
      double timestamp = point_x.x;
      double q_x = point_x.y;
      double q_y = data_y.at(index).y;
      double q_z = data_z.at(index).y;
      double q_w = data_w.at(index).y;

      if (timestamp >= _last_timestamp)
      {
        std::array<double,3> RPY;
        calculateNextPoint(index, {q_x, q_y, q_z, q_w}, RPY );

        data_roll.pushBack({ timestamp,  _scale * (RPY[0] + _roll_offset)});
        data_pitch.pushBack({ timestamp, _scale * (RPY[1] + _pitch_offset) });
        data_yaw.pushBack({ timestamp,   _scale * (RPY[2] + _yaw_offset)});

        _last_timestamp = timestamp;
      }
      index++;
    }
  }

 void calculateNextPoint(size_t index,
                         const std::array<double,4>& quat,
                         std::array<double,3>& rpy)
 {
   double q_x = quat[0];
   double q_y = quat[1];
   double q_z = quat[2];
   double q_w = quat[3];

   double quat_norm2 = (q_w * q_w) + (q_x * q_x) + (q_y * q_y) + (q_z * q_z);
   if (std::abs(quat_norm2 - 1.0) > std::numeric_limits<double>::epsilon())
   {
     double mult = 1.0 / std::sqrt(quat_norm2);
     q_x *= mult;
     q_y *= mult;
     q_z *= mult;
     q_w *= mult;
   }
   double roll, pitch, yaw;
   // roll (x-axis rotation)
   double sinr_cosp = 2 * (q_w * q_x + q_y * q_z);
   double cosr_cosp = 1 - 2 * (q_x * q_x + q_y * q_y);
   roll = std::atan2(sinr_cosp, cosr_cosp);

   // pitch (y-axis rotation)
   double sinp = 2 * (q_w * q_y - q_z * q_x);
   if (std::abs(sinp) >= 1)
   {
     pitch = std::copysign(M_PI_2, sinp);  // use 90 degrees if out of range
   }
   else
   {
     pitch = std::asin(sinp);
   }
   // yaw (z-axis rotation)
   double siny_cosp = 2 * (q_w * q_z + q_x * q_y);
   double cosy_cosp = 1 - 2 * (q_y * q_y + q_z * q_z);
   yaw = std::atan2(siny_cosp, cosy_cosp);

   const double WRAP_ANGLE = M_PI*2.0;
   const double WRAP_THRESHOLD = M_PI*1.95;

   //--------- wrap ------
   if( index != 0 && _wrap)
   {
     if( (roll -_prev_roll) > WRAP_THRESHOLD ) {
       _roll_offset -= WRAP_ANGLE;
     }
     else if( (_prev_roll - roll) > WRAP_THRESHOLD ) {
       _roll_offset += WRAP_ANGLE;
     }

     if( (pitch -_prev_pitch) > WRAP_THRESHOLD ) {
       _pitch_offset -= WRAP_ANGLE;
     }
     else if( (_prev_pitch - pitch) > WRAP_THRESHOLD ) {
       _pitch_offset += WRAP_ANGLE;
     }

     if( (yaw -_prev_yaw) > WRAP_THRESHOLD ) {
       _yaw_offset -= WRAP_ANGLE;
     }
     else if( (_prev_yaw - yaw) > WRAP_THRESHOLD ) {
       _yaw_offset += WRAP_ANGLE;
     }
   }

  _prev_pitch = pitch;
  _prev_roll = roll;
  _prev_yaw = yaw;

  rpy = { roll, pitch, yaw };
 }

private:
  double _prev_roll = 0;
  double _prev_yaw = 0;
  double _prev_pitch = 0;
  double _roll_offset = 0;
  double _pitch_offset = 0;
  double _yaw_offset = 0;
  double _scale = 1.0;
  bool _wrap = true;
  double _last_timestamp = - std::numeric_limits<double>::max();
};


ToolboxQuaternion::ToolboxQuaternion()
{
  _widget = new QWidget(nullptr);
  ui = new Ui::quaternion_to_RPY;

  ui->setupUi(_widget);

  ui->lineEditX->installEventFilter(this);
  ui->lineEditY->installEventFilter(this);
  ui->lineEditZ->installEventFilter(this);
  ui->lineEditW->installEventFilter(this);

  connect( ui->buttonBox, &QDialogButtonBox::rejected,
           this, &ToolboxPlugin::closed );

  connect( ui->checkBoxUnwrap, &QCheckBox::toggled,
           this, &ToolboxQuaternion::onParametersChanged );

  connect( ui->radioButtonDegrees, &QRadioButton::toggled,
           this, &ToolboxQuaternion::onParametersChanged );

  connect( ui->pushButtonSave, &QPushButton::clicked,
           this, &ToolboxQuaternion::on_pushButtonSave_clicked );
}

ToolboxQuaternion::~ToolboxQuaternion()
{

}

void ToolboxQuaternion::init(PJ::PlotDataMapRef &src_data,
                             PJ::TransformsMap &transform_map)
{
  _plot_data = &src_data;
  _transforms = &transform_map;

  _plot_widget = new PJ::PlotWidgetBase(ui->frame);

  auto preview_layout = new QHBoxLayout( ui->framePlotPreview );
  preview_layout->setMargin(6);
  preview_layout->addWidget(_plot_widget->widget());

}

std::pair<QWidget*, PJ::ToolboxPlugin::WidgetType>
ToolboxQuaternion::providedWidget() const
{
  return { _widget, PJ::ToolboxPlugin::FIXED };
}

bool ToolboxQuaternion::onShowWidget()
{
  return true;
}

bool ToolboxQuaternion::eventFilter(QObject *obj, QEvent *ev)
{
  if( ev->type() == QEvent::DragEnter )
  {
    auto event = static_cast<QDragEnterEvent*>(ev);
    const QMimeData* mimeData = event->mimeData();
    QStringList mimeFormats = mimeData->formats();

    for(const QString& format : mimeFormats)
    {
      QByteArray encoded = mimeData->data(format);
      QDataStream stream(&encoded, QIODevice::ReadOnly);

      if (format != "curveslist/add_curve") {
        return false;
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
        return false;
      }

      _dragging_curve = curves.front();

      if( obj == ui->lineEditX ||
          obj == ui->lineEditY ||
          obj == ui->lineEditZ ||
          obj == ui->lineEditW )
      {
        event->acceptProposedAction();
        return true;
      }
    }
  }
  else if ( ev->type() == QEvent::Drop ) {
    auto lineEdit = qobject_cast<QLineEdit*>( obj );

    if ( !lineEdit )
    {
      return false;
    }
    lineEdit->setText( _dragging_curve );

    if( (obj == ui->lineEditX && _dragging_curve.endsWith("x") ) ||
        (obj == ui->lineEditY && _dragging_curve.endsWith("y") ) ||
        (obj == ui->lineEditZ && _dragging_curve.endsWith("z") ) ||
        (obj == ui->lineEditW && _dragging_curve.endsWith("w") ) )
    {
      autoFill( _dragging_curve.left( _dragging_curve.size() - 1 ) );
    }
  }

  return false;
}

void ToolboxQuaternion::autoFill(QString prefix)
{
  QStringList suffix = {"x", "y", "z", "w"};
  std::array<QLineEdit*, 4> lineEdits = {ui->lineEditX,
                                         ui->lineEditY,
                                         ui->lineEditZ,
                                         ui->lineEditW };
  QStringList names;
  for( int i=0; i<4; i++ )
  {
    QString name = prefix + suffix[i];
    auto it = _plot_data->numeric.find( name.toStdString() );
    if( it != _plot_data->numeric.end() )
    {
      names.push_back( name );
    }
  }

  if( names.size() == 4 )
  {
    for( int i=0; i<4; i++ )
    {
      lineEdits[i]->setText( names[i] );
    }
    ui->lineEditOut->setText( prefix );
    ui->pushButtonSave->setEnabled( true );

    generateRPY();
  }
}

void ToolboxQuaternion::generateRPY()
{
  using namespace PJ;

  bool wrap = ui->checkBoxUnwrap->isChecked();
  double unit_scale = ui->radioButtonDegrees->isChecked() ? (180.0 / M_PI) : 1.0;

  QuaternionToRollPitchYaw transform;

  PlotData& data_x = _plot_data->getOrCreateNumeric(
        ui->lineEditX->text().toStdString() );
  PlotData& data_y = _plot_data->getOrCreateNumeric(
        ui->lineEditY->text().toStdString() );
  PlotData& data_z = _plot_data->getOrCreateNumeric(
        ui->lineEditZ->text().toStdString() );
  PlotData& data_w = _plot_data->getOrCreateNumeric(
        ui->lineEditW->text().toStdString() );

  std::string prefix = ui->lineEditOut->text().toStdString();
  PlotData data_roll ( prefix + "/roll", {});
  PlotData data_pitch( prefix + "/pitch", {});
  PlotData data_yaw( prefix + "/yaw", {});

  std::vector<PlotData*> dst_vector = { &data_roll, &data_pitch, &data_yaw };
  transform.setData( _plot_data, {&data_x, &data_y, &data_z, &data_w }, dst_vector );
  transform.setWarp( wrap );
  transform.setScale( unit_scale );

  transform.calculate();

  _plot_widget->removeAllCurves();
  _plot_widget->addCurve( data_roll.plotName(), data_roll );
  _plot_widget->addCurve( data_pitch.plotName(), data_pitch );
  _plot_widget->addCurve( data_yaw.plotName(), data_yaw );
  _plot_widget->resetZoom();
}


void ToolboxQuaternion::on_pushButtonSave_clicked()
{
  QString qprefix = ui->lineEditOut->text();
  if( !qprefix.endsWith('/') )
  {
    qprefix.append('/');
  }
  std::string prefix = qprefix.toStdString();
  PlotData& data_roll  = _plot_data->getOrCreateNumeric( prefix + "roll", {});
  PlotData& data_pitch = _plot_data->getOrCreateNumeric( prefix + "pitch", {});
  PlotData& data_yaw   = _plot_data->getOrCreateNumeric( prefix + "yaw", {});

  auto transform = std::make_shared<QuaternionToRollPitchYaw>();

  PlotData& data_x = _plot_data->getOrCreateNumeric(
        ui->lineEditX->text().toStdString() );
  PlotData& data_y = _plot_data->getOrCreateNumeric(
        ui->lineEditY->text().toStdString() );
  PlotData& data_z = _plot_data->getOrCreateNumeric(
        ui->lineEditZ->text().toStdString() );
  PlotData& data_w = _plot_data->getOrCreateNumeric(
        ui->lineEditW->text().toStdString() );

  std::vector<PlotData*> dst_vector = { &data_roll, &data_pitch, &data_yaw };
  transform->setData( _plot_data, {&data_x, &data_y, &data_z, &data_w }, dst_vector );

  bool wrap = ui->checkBoxUnwrap->isChecked();
  double unit_scale = ui->radioButtonDegrees->isChecked() ? (180.0 / M_PI) : 1.0;

  transform->setWarp( wrap );
  transform->setScale( unit_scale );
  transform->calculate();

  _transforms->insert( { (qprefix + "RPY").toStdString(), transform} );

  emit plotCreated( qprefix + "roll", false);
  emit plotCreated( qprefix + "pitch", false);
  emit plotCreated( qprefix + "yaw", false);

  emit this->closed();
}


void ToolboxQuaternion::onParametersChanged()
{
    if( ui->lineEditX->text().isEmpty() ||
        ui->lineEditY->text().isEmpty() ||
        ui->lineEditZ->text().isEmpty() ||
        ui->lineEditW->text().isEmpty() ||
        ui->lineEditOut->text().isEmpty() )
    {
      return;
    }
    generateRPY();
}
