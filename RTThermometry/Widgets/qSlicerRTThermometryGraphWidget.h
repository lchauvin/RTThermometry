/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __qSlicerRTThermometryGraphWidget_h
#define __qSlicerRTThermometryGraphWidget_h

// Qt includes
#include <QDialog>
#include <QWidget>

// VTK includes
#include <vtkAxis.h>
#include <vtkChartLegend.h>
#include <vtkChartXY.h>
#include <vtkDoubleArray.h>
#include <vtkPlot.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkVariant.h>

// FooBar Widgets includes
#include "qSlicerRTThermometryModuleWidgetsExport.h"

class qSlicerRTThermometryGraphWidgetPrivate;

/// \ingroup Slicer_QtModules_LoadableModuleTemplate
class Q_SLICER_MODULE_RTTHERMOMETRY_WIDGETS_EXPORT qSlicerRTThermometryGraphWidget
  : public QDialog
{
  Q_OBJECT
public:
  typedef QDialog Superclass;
  qSlicerRTThermometryGraphWidget(QWidget *parent=0);
  virtual ~qSlicerRTThermometryGraphWidget();

  void recordNewData(std::string sensorID, std::string sensorName, double sensorValue, int imageNumber);
  void clearData();

protected slots:

protected:
  QScopedPointer<qSlicerRTThermometryGraphWidgetPrivate> d_ptr;
  void closeEvent(QCloseEvent* event);

signals:
  void graphHidden();

private:
  Q_DECLARE_PRIVATE(qSlicerRTThermometryGraphWidget);
  Q_DISABLE_COPY(qSlicerRTThermometryGraphWidget);
};

#endif
