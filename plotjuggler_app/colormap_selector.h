#ifndef COLORMAP_SELECTOR_H
#define COLORMAP_SELECTOR_H

#include <QDialog>

namespace Ui {
class colormap_selector;
}

class ColormapSelectorDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ColormapSelectorDialog(QString series, QString default_colormap, QWidget *parent = nullptr);
  ~ColormapSelectorDialog();

  QString selectedColorMap() const;

private slots:
  void on_buttonEditor_clicked();

  void on_comboBox_currentTextChanged(const QString &arg1);

private:
  Ui::colormap_selector *ui;
  QString _selected;
};

#endif // COLORMAP_SELECTOR_H
