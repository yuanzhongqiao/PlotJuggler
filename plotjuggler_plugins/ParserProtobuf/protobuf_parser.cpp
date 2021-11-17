#include <QSettings>
#include <QMessageBox>
#include "protobuf_parser.h"
#include "PlotJuggler/fmt/format.h"
#include "PlotJuggler/svg_util.h"


using namespace google::protobuf;
using namespace google::protobuf::io;
using namespace google::protobuf::compiler;

bool ProtobufParser::parseMessage(const MessageRef serialized_msg,
                                  double &timestamp)
{
  const google::protobuf::Message* prototype_msg =
      _msg_factory.GetPrototype(_msg_descriptor);

  google::protobuf::Message* mutable_msg = prototype_msg->New();
  if (!mutable_msg->ParseFromArray(serialized_msg.data(),
                                   serialized_msg.size()))
  {
    return false;
  }

  std::function<void(const google::protobuf::Message&, const std::string&)> ParseImpl;

  ParseImpl = [&](const google::protobuf::Message& msg, const std::string& prefix)
  {
    const Reflection* reflection = msg.GetReflection();
    std::vector<const FieldDescriptor*> fields;
    reflection->ListFields(msg, &fields);

    for (auto field: fields)
    {
      std::string key = prefix.empty() ?
                            field->name():
                            fmt::format("{}/{}", prefix, field->name() );
      std::string suffix;

      if (!field)
      {
        continue;
      }

      int count = 1;
      bool repeated = false;
      if (field->is_repeated())
      {
        count = reflection->FieldSize(msg, field);
        repeated = true;
      }

      for(int index = 0; index < count ; index++)
      {
        if(repeated)
        {
          suffix = fmt::format("[{}]", index);
        }

        bool is_double = true;
        double value = 0;
        switch(field->cpp_type())
        {
          case FieldDescriptor::CPPTYPE_DOUBLE:{
            value = !repeated ? reflection->GetDouble(msg, field) :
                                reflection->GetRepeatedDouble(msg, field, index);
          }break;
          case FieldDescriptor::CPPTYPE_FLOAT:{
            auto tmp = !repeated ? reflection->GetFloat(msg, field) :
                                   reflection->GetRepeatedFloat(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_UINT32:{
            auto tmp = !repeated ? reflection->GetUInt32(msg, field) :
                                   reflection->GetRepeatedUInt32(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_UINT64:{
            auto tmp = !repeated ? reflection->GetUInt64(msg, field) :
                                   reflection->GetRepeatedUInt64(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_BOOL:{
            auto tmp = !repeated ? reflection->GetBool(msg, field) :
                                   reflection->GetRepeatedBool(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_INT32:{
            auto tmp = !repeated ? reflection->GetInt32(msg, field) :
                                   reflection->GetRepeatedInt32(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_INT64:{
            auto tmp = !repeated ? reflection->GetInt64(msg, field) :
                                   reflection->GetRepeatedInt64(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_ENUM:{
            auto tmp = !repeated ? reflection->GetEnum(msg, field) :
                                   reflection->GetRepeatedEnum(msg, field, index);

            auto& series = this->getStringSeries(key + suffix);
            series.pushBack({timestamp, tmp->name()});
            is_double = false;
          }break;
          case FieldDescriptor::CPPTYPE_STRING:{
            auto tmp = !repeated ? reflection->GetString(msg, field) :
                                   reflection->GetRepeatedString(msg, field, index);

            auto& series = this->getStringSeries(key + suffix);
            series.pushBack({timestamp, tmp});
            is_double = false;
          }break;
          case FieldDescriptor::CPPTYPE_MESSAGE:
          {
// Fix macro issue in Windows
#pragma push_macro("GetMessage")
#undef GetMessage
            const auto& new_msg = reflection->GetMessage(msg, field, nullptr);
#pragma pop_macro("GetMessage")

            ParseImpl(new_msg, key + suffix);
            is_double = false;
          }break;
        }

        if( is_double )
        {
          auto& series = this->getSeries(key + suffix);
          series.pushBack({timestamp, value});
        }
      }
    }
  };

  // start recursion
  ParseImpl(*mutable_msg, _topic_name);

  return true;
}

ProtobufParserCreator::ProtobufParserCreator()
{
  _widget = new QWidget(nullptr);
  ui = new Ui::ProtobufLoader;
  ui->setupUi(_widget);

  _source_tree.MapPath("/", "/");
  _source_tree.MapPath("", "");

  loadSettings();

  QSettings settings;
  QString theme = settings.value("Preferences::theme", "light").toString();
  ui->pushButtonRemove->setIcon(LoadSvg(":/resources/svg/trash.svg", theme));

  connect( ui->pushButtonLoad, &QPushButton::clicked, this, &ProtobufParserCreator::onLoadFile);
  connect( ui->pushButtonRemove, &QPushButton::clicked, this, &ProtobufParserCreator::onRemoveFile);

  connect( ui->listWidget, &QListWidget::currentRowChanged,
          this, &ProtobufParserCreator::onSelectionChanged );

  QString last_proto = settings.value("ProtobufParserCreator.lastProtoSelection").toString();

  auto proto_items = ui->listWidget->findItems(last_proto, Qt::MatchExactly);
  if( !last_proto.isEmpty() && proto_items.size() == 1)
  {
    ui->listWidget->setCurrentItem(proto_items.front());
  }
  _selected_file = last_proto;

  QString last_type = settings.value("ProtobufParserCreator.lastType").toString();
  int combo_index = ui->comboBox->findText(last_type, Qt::MatchExactly);
  if( !last_type.isEmpty() && combo_index != -1)
  {
    ui->comboBox->setCurrentIndex(combo_index);
    onComboChanged(last_type);
  }

  connect( ui->comboBox, qOverload<const QString&>(&QComboBox::currentIndexChanged),
          this, &ProtobufParserCreator::onComboChanged );
}

void ProtobufParserCreator::importFile(QString filename)
{
  QFile file(filename);
  if( !file.exists() )
  {
    QMessageBox::warning(nullptr, tr("Error loading file"),
                         tr("File %1 does not exist").arg(filename),
                         QMessageBox::Cancel);
    return;
  }
  file.open(QIODevice::ReadOnly);
  Info info;
  QFileInfo fileinfo(filename);
  QString file_basename = fileinfo.fileName();
  info.file_path = filename;
  info.proto_text = file.readAll();

//  _source_tree.MapPath(fname, fname);
  _source_tree.MapPath("", filename.toStdString());
  _source_tree.MapPath("", file_basename.toStdString());
  _source_tree.MapPath("", fileinfo.absolutePath().toStdString());

 // auto res = _source_tree.DiskFileToVirtualFile(fname, &virtual_file, &shadowing_disk_file);
  info.file_descriptor = _importer->Import(filename.toStdString());

  if( !info.file_descriptor )
  {
    QMessageBox::warning(nullptr, tr("Error loading file"),
                         tr("Error parsing the file:\n\n %1").arg(filename),
                         QMessageBox::Cancel);
    return;
  }
  for(int i=0; i < info.file_descriptor->message_type_count(); i++)
  {
    const std::string& type_name = info.file_descriptor->message_type(i)->name();
    auto descriptor = info.file_descriptor->FindMessageTypeByName(type_name);
    info.descriptors.insert({QString::fromStdString(type_name), descriptor });
  }
  _files.insert( {file_basename, info} );

  if( ui->listWidget->findItems(file_basename, Qt::MatchExactly).empty() )
  {
    ui->listWidget->addItem( file_basename );
    ui->listWidget->sortItems();
  }
}

void ProtobufParserCreator::loadSettings()
{
  _importer.reset( new Importer(&_source_tree, &_error_collector) );
  _files.clear();

  QSettings settings;
  auto file_list = settings.value("ProtobufParserCreator.filenames").toStringList();

  for( const auto& filename: file_list)
  {
    importFile(filename);
  }
}

void ProtobufParserCreator::saveSettings()
{
  QSettings settings;
  QStringList file_list;
  for(auto it : _files)
  {
    file_list.push_back(it.second.file_path);
  }
  settings.setValue("ProtobufParserCreator.filenames", file_list);
}

ProtobufParserCreator::~ProtobufParserCreator()
{
  saveSettings();
  delete ui;
}

MessageParserPtr ProtobufParserCreator::createInstance(
    const std::string &topic_name, PlotDataMapRef &data)
{
  onComboChanged(ui->comboBox->currentText());
  return std::make_shared<ProtobufParser>(topic_name, data, _selected_descriptor);
}

void ProtobufParserCreator::onLoadFile()
{
  QSettings settings;

  QString directory_path =
      settings.value("ProtobufParserCreator.loadDirectory", QDir::currentPath()).toString();

  QString filename = QFileDialog::getOpenFileName(_widget, tr("Load StyleSheet"),
                                                   directory_path, tr("(*.proto)"));

  if (filename.isEmpty())
  {
    return;
  }

  importFile(filename);

  directory_path = QFileInfo(filename).absolutePath();
  settings.setValue("ProtobufParserCreator.loadDirectory", directory_path);

  saveSettings();
}

void ProtobufParserCreator::onRemoveFile()
{
  auto selected = ui->listWidget->selectedItems();

  while(!selected.isEmpty())
  {
    auto item = selected.front();
    _files.erase(item->text());
    delete ui->listWidget->takeItem(ui->listWidget->row(item));
    selected.pop_front();
  }
  saveSettings();
  loadSettings();
}

void ProtobufParserCreator::onSelectionChanged(int row)
{
  if( row == -1 )
  {
    ui->comboBox->clear();
    ui->comboBox->setEnabled(false);
    ui->protoPreview->setText("");
    return;
  }
  QString filename = ui->listWidget->item(row)->text();
  const auto& info = _files[filename];

  ui->protoPreview->setText( info.proto_text );

  ui->comboBox->clear();
  ui->comboBox->setEnabled(true);

  for(const auto& it: info.descriptors)
  {
    ui->comboBox->addItem( it.first );
  }

  _selected_file = filename;

  QSettings settings;
  settings.setValue("ProtobufParserCreator.lastProtoSelection", filename);
}

void ProtobufParserCreator::onComboChanged(const QString& text)
{
  auto info_it = _files.find(_selected_file);
  if( info_it != _files.end())
  {
    auto descr_it = info_it->second.descriptors.find(text);
    if( descr_it != info_it->second.descriptors.end())
    {
      _selected_descriptor = descr_it->second;
      QSettings settings;
      settings.setValue("ProtobufParserCreator.lastType", text);
    }
  }
}
/*
bool ProtobufParserCreator::updateUI()
{
  Info info;
  info.proto_text = proto;

  ArrayInputStream proto_input_stream(proto.data(), proto.size());
  Tokenizer tokenizer(&proto_input_stream, nullptr);
  FileDescriptorProto file_desc_proto;

  DiskSourceTree source_tree;
  Importer importer(&source_tree, nullptr);

  if (!importer.Import(&tokenizer, &file_desc_proto))
  {
    QMessageBox::warning(nullptr, tr("Error loading file"),
                         tr("Error parsing the file %1").arg(filename),
                         QMessageBox::Cancel);
    return false;
  }

  if (!file_desc_proto.has_name())
  {
    file_desc_proto.set_name(filename.toStdString());
  }

  info.file_descriptor = _pool->BuildFile(file_desc_proto);
  if (info.file_descriptor == nullptr)
  {
    QMessageBox::warning(nullptr, tr("Error loading file"),
                         tr("Error getting file descriptor."),
                         QMessageBox::Cancel);
    return false;
  }

  for(int i=0; i < info.file_descriptor->message_type_count(); i++)
  {
    const std::string& type_name = info.file_descriptor->message_type(i)->name();
    auto descriptor = info.file_descriptor->FindMessageTypeByName(type_name);
    info.descriptors.insert({QString::fromStdString(type_name), descriptor });
  }

  _files.insert( filename, info );

  // add to the list if not present
  if( ui->listWidget->findItems(filename, Qt::MatchExactly).empty() )
  {
    ui->listWidget->addItem( filename );
    ui->listWidget->sortItems();
  }
  return true;
}*/

