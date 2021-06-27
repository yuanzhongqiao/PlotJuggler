#include "toolbox_quaternion.h"
#include "ui_quaternion_to_rpy.h"

#include <QDialogButtonBox>
#include <QEvent>
#include <QMimeData>
#include <QDragEnterEvent>

ToolboxQuaternion::ToolboxQuaternion()
{
  _widget = new QWidget(nullptr);
  ui = new Ui::quaternion_to_RPY;

  ui->setupUi(_widget);

  ui->lineEditX->installEventFilter(this);
  ui->lineEditY->installEventFilter(this);
  ui->lineEditZ->installEventFilter(this);
  ui->lineEditW->installEventFilter(this);

  connect ( ui->buttonBox, &QDialogButtonBox::rejected,
            this, &ToolboxQuaternion::closed );
}

void ToolboxQuaternion::init(PJ::PlotDataMapRef &src_data,
                             PJ::TransformsMap &transform_map)
{
  _plot_data = &src_data;
  _transforms = &transform_map;

 // _plot_widget = new PJ::PlotWidgetProxy(src_data, ui->frame);
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
  }
}



