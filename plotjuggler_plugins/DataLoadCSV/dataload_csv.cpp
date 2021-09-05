#include "dataload_csv.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QProgressDialog>
#include <QDateTime>
#include <QInputDialog>

const int TIME_INDEX_NOT_DEFINED = -2;
const int TIME_INDEX_GENERATED = -1;

DataLoadCSV::DataLoadCSV()
{
  _extensions.push_back("csv");
  _separator = QRegExp("(\\,)");
  // setup the dialog

  _dialog = new QDialog();
  _ui = new Ui::DialogCSV();
  _ui->setupUi(_dialog);

  _ui->splitter->setStretchFactor(0, 1);
  _ui->splitter->setStretchFactor(1, 2);
}


DataLoadCSV::~DataLoadCSV()
{
  delete _ui;
  delete _dialog;
}

const std::vector<const char*>& DataLoadCSV::compatibleFileExtensions() const
{
  return _extensions;
}

void DataLoadCSV::parseHeader(QFile& file,
                              std::vector<std::string>* column_names )
{
  file.open(QFile::ReadOnly);

  column_names->clear();
  _ui->listWidgetSeries->clear();

  QTextStream inA(&file);
  // The first line should contain the header. If it contains a number, we will
  // apply a name ourselves
  QString first_line = inA.readLine();

  QString preview_lines = first_line + "\n";

  QStringList firstline_items = first_line.split( _separator );

  int is_number_count = 0;

  // check if all the elements in first row are numbers
  for (int i = 0; i < firstline_items.size(); i++)
  {
    bool isNum;
    firstline_items[i].trimmed().toDouble(&isNum);
    if( isNum )
    {
      is_number_count++;
    }
  }

  if( is_number_count == firstline_items.size() )
  {
    for (int i = 0; i < firstline_items.size(); i++)
    {
      auto field_name = QString("_Column_%1").arg(i);
      _ui->listWidgetSeries->addItem( field_name );
      column_names->push_back(field_name.toStdString());
    }
  }
  else
  {
    for (int i = 0; i < firstline_items.size(); i++)
    {
      // remove annoying prefix
      QString field_name(firstline_items[i].trimmed());

      if (field_name.isEmpty())
      {
        field_name = QString("_Column_%1").arg(i);
      }
      _ui->listWidgetSeries->addItem( field_name );
      column_names->push_back(field_name.toStdString());
    }
  }

  int linecount = 1;
  while (!inA.atEnd())
  {
    auto line = inA.readLine();
    if( linecount++ < 100 )
    {
      preview_lines += line + "\n";
    }
    else{
      break;
    }
  }
  _ui->rawText->setPlainText(preview_lines);

  file.close();
}

int DataLoadCSV::launchDialog(QFile &file, std::vector<std::string> *column_names)
{
  column_names->clear();

  QSettings settings;
  _dialog->restoreGeometry(settings.value("DataLoadCSV.geometry").toByteArray());

  // suggest separator
  {
    file.open(QFile::ReadOnly);
    QTextStream in(&file);

    QString first_line = in.readLine();
    int comma_count = first_line.count(QLatin1Char(','));
    int semicolon_count = first_line.count(QLatin1Char(';'));
    int space_count = first_line.count(QLatin1Char(' '));

    if( comma_count > 3 && comma_count > semicolon_count )
    {
      _ui->comboBox->setCurrentIndex( 0 );
      _separator = QRegExp("(\\,)");
    }
    if( semicolon_count > 3 && semicolon_count > comma_count )
    {
      _ui->comboBox->setCurrentIndex( 1 );
      _separator = QRegExp("(\\;)");
    }
    if( space_count > 3 && comma_count == 0 && semicolon_count == 0 )
    {
      _ui->comboBox->setCurrentIndex( 2 );
      _separator = QRegExp("(\\ )");
    }
    file.close();
  }

  // connections
  connect ( _ui->comboBox, qOverload<int>( &QComboBox::currentIndexChanged ),
          this, [&]( int index ) {
            switch( index )
            {
            case 0: _separator = QRegExp("(\\,)"); break;
            case 1: _separator = QRegExp("(\\;)"); break;
            case 2: _separator = QRegExp("(\\ )"); break;
            }
            parseHeader( file, column_names );
          });

  connect ( _ui->radioButtonSelect, &QRadioButton::toggled,
          this, [this]( bool checked ) {
            _ui->listWidgetSeries->setEnabled( checked );
          });
  connect ( _ui->listWidgetSeries, &QListWidget::itemSelectionChanged,
          this, [this]() {
            QModelIndexList indexes = _ui->listWidgetSeries->selectionModel()->selectedIndexes();
            _ui->buttonBox->setEnabled( indexes.size() == 1 );
          });

  connect ( _ui->listWidgetSeries, &QListWidget::itemDoubleClicked,
          this, [this]() {
            emit _ui->buttonBox->accepted();
          });

  // parse the header once and launch the dialog
  parseHeader(file, column_names);

  int res = _dialog->exec();
  settings.setValue("DataLoadCSV.geometry", _dialog->saveGeometry());

  if (res == QDialog::Rejected)
  {
    return TIME_INDEX_NOT_DEFINED;
  }

  if( _ui->radioButtonIndex->isChecked() )
  {
    return TIME_INDEX_GENERATED;
  }

  QModelIndexList indexes = _ui->listWidgetSeries->selectionModel()->selectedIndexes();
  if( indexes.size() == 1)
  {
    return indexes.front().row();
  }

  return TIME_INDEX_NOT_DEFINED;
}

bool DataLoadCSV::readDataFromFile(FileLoadInfo* info, PlotDataMapRef& plot_data)
{
  bool use_provided_configuration = false;
  _default_time_axis.clear();

  if (info->plugin_config.hasChildNodes())
  {
    use_provided_configuration = true;
    xmlLoadState(info->plugin_config.firstChildElement());
  }

  QFile file( info->filename);
  std::vector<std::string> column_names;

  int time_index = TIME_INDEX_NOT_DEFINED;

  if( !use_provided_configuration )
  {
    time_index = launchDialog( file, &column_names );
  }
  else
  {
    parseHeader(file, &column_names );
    if (_default_time_axis == "__TIME_INDEX_GENERATED__")
    {
      time_index = TIME_INDEX_GENERATED;
    }
    else{
      for( size_t i=0; i<column_names.size(); i++ )
      {
        if ( column_names[i] == _default_time_axis )
        {
          time_index = i;
          break;
        }
      }
    }
  }

  if( time_index == TIME_INDEX_NOT_DEFINED )
  {
    return false;
  }

  //-----------------------------------

  file.open(QFile::ReadOnly);
  QTextStream in(&file);

  std::vector<PlotData*> plots_vector;

  bool interrupted = false;

  int linecount = 0;

  // count the number of lines first
  int tot_lines = 0;
  {
    QFile file(info->filename);
    QTextStream in(&file);
    while (!in.atEnd())
    {
      in.readLine();
      tot_lines++;
    }
    file.close();
  }

  QProgressDialog progress_dialog;
  progress_dialog.setLabelText("Loading... please wait");
  progress_dialog.setWindowModality(Qt::ApplicationModal);
  progress_dialog.setRange(0, tot_lines - 1);
  progress_dialog.setAutoClose(true);
  progress_dialog.setAutoReset(true);
  progress_dialog.show();

  // remove first line (header)
  in.readLine();

  //---- build plots_vector from header  ------
  for (unsigned i = 0; i < column_names.size(); i++)
  {
    const std::string& field_name = (column_names[i]);

    auto it = plot_data.addNumeric(field_name);
    plots_vector.push_back(&(it->second));
  }

  //-----------------
  double prev_time = -std::numeric_limits<double>::max();
  bool to_string_parse = false;
  QString format_string = "";

  while (!in.atEnd())
  {
    QString line = in.readLine();

    QStringList string_items = line.split( _separator );
    if (string_items.size() != column_names.size())
    {
      auto err_msg = QString("The number of values at line %1 is %2,\n"
                             "but the expected number of columns is %3.\n"
                             "Aborting...")
          .arg(linecount+1)
          .arg(string_items.size())
          .arg(column_names.size());

      QMessageBox::warning(nullptr, "Error reading file", err_msg );
      return false;
    }
    double t = linecount;

    if (time_index >= 0)
    {
      bool is_number = false;
      QString str = string_items[time_index].trimmed();
      if (!to_string_parse)
      {
          t = str.toDouble(&is_number);
          if (!is_number)
          {
              to_string_parse = true;
          }
      }

      if (!is_number || to_string_parse)
      {
          if (format_string.isEmpty())
          {
              bool ok;
              format_string = QInputDialog::getText(nullptr, tr("Error reading file"),
                      QString("One of the timestamps is not a valid number.\n"
                              "Failing string: \"%1\".\n"
                              "Please enter a Format String (see QDateTime::fromString).\n").arg(str),
                      QLineEdit::Normal, "yyyy-MM-dd hh:mm:ss", &ok);

              if (!ok || format_string.isEmpty())
              {
                  return false;
              }
          }

          QDateTime ts = QDateTime::fromString(str, format_string);
          is_number = ts.isValid();
          if (!is_number)
          {
              QMessageBox::StandardButton reply;
              reply = QMessageBox::warning(nullptr, tr("Error reading file"),
                      tr("Couldn't parse timestamp. Aborting.\n"));

              return false;
          }
          t = ts.toMSecsSinceEpoch()/1000.0;
      }

      if (prev_time > t)
      {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(nullptr, tr("Error reading file"),
                                     tr("Selected time in not strictly monotonic. "
                                        "Loading will be aborted\n"));
        return false;
      }

      prev_time = t;
    }

    int index = 0;
    for (int i = 0; i < string_items.size(); i++)
    {
      bool is_number = false;
      double y = string_items[i].toDouble(&is_number);
      if (is_number)
      {
        PlotData::Point point(t, y);
        plots_vector[index]->pushBack(point);
      }
      index++;
    }

    if (linecount++ % 100 == 0)
    {
      progress_dialog.setValue(linecount);
      QApplication::processEvents();
      if (progress_dialog.wasCanceled())
      {
        interrupted = true;
        break;
      }
    }
  }
  file.close();

  if (interrupted)
  {
    progress_dialog.cancel();
    plot_data.clear();
  }

  _default_time_axis = column_names[ time_index ];
  return true;
}


bool DataLoadCSV::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  QDomElement elem = doc.createElement("default");
  elem.setAttribute("time_axis", _default_time_axis.c_str());

  parent_element.appendChild(elem);
  return true;
}

bool DataLoadCSV::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement elem = parent_element.firstChildElement("default");
  if (!elem.isNull())
  {
    if (elem.hasAttribute("time_axis"))
    {
      _default_time_axis = elem.attribute("time_axis").toStdString();
      return true;
    }
  }
  return false;
}
