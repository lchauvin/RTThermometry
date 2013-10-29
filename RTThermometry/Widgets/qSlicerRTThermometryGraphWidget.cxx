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

// FooBar Widgets includes
#include "qSlicerRTThermometryGraphWidget.h"
#include "ui_qSlicerRTThermometryGraphWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_LoadableModuleTemplate
class qSlicerRTThermometryGraphWidgetPrivate
  : public Ui_qSlicerRTThermometryGraphWidget
{
  Q_DECLARE_PUBLIC(qSlicerRTThermometryGraphWidget);
protected:
  qSlicerRTThermometryGraphWidget* const q_ptr;

public:
  std::map<std::string, vtkTable*>  TemperatureMap;
  typedef std::map<std::string, vtkTable*>::iterator TemperatureMapIter;

public:
  qSlicerRTThermometryGraphWidgetPrivate(
    qSlicerRTThermometryGraphWidget& object);
  virtual void setupUi(qSlicerRTThermometryGraphWidget*);
};

// --------------------------------------------------------------------------
qSlicerRTThermometryGraphWidgetPrivate
::qSlicerRTThermometryGraphWidgetPrivate(
  qSlicerRTThermometryGraphWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerRTThermometryGraphWidgetPrivate
::setupUi(qSlicerRTThermometryGraphWidget* widget)
{
  this->Ui_qSlicerRTThermometryGraphWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qSlicerRTThermometryGraphWidget methods

//-----------------------------------------------------------------------------
qSlicerRTThermometryGraphWidget
::qSlicerRTThermometryGraphWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerRTThermometryGraphWidgetPrivate(*this) )
{
  Q_D(qSlicerRTThermometryGraphWidget);
  d->setupUi(this);

  if (d->ChartView && d->ChartView->chart())
    {
    vtkChartXY* chartXY = d->ChartView->chart();
    chartXY->GetAxis(1)->SetTitle("Images");
    chartXY->GetAxis(0)->SetTitle("Temperature (C)");
    chartXY->SetShowLegend(true);
    chartXY->GetLegend()->SetDragEnabled(true);
    }
}

//-----------------------------------------------------------------------------
qSlicerRTThermometryGraphWidget
::~qSlicerRTThermometryGraphWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryGraphWidget
::recordNewData(std::string sensorID, std::string sensorName, double sensorValue, int imageNumber)
{
  Q_D(qSlicerRTThermometryGraphWidget);

  if (!d->ChartView || !d->ChartView->chart())
    {
    return;
    }

  // Check if sensor table already exists
  vtkTable* currentSensorTable = NULL;
  qSlicerRTThermometryGraphWidgetPrivate::TemperatureMapIter iter
    = d->TemperatureMap.find(sensorID);
  if (iter == d->TemperatureMap.end())
    {
    // Sensor table not found. Add new table.
    vtkSmartPointer<vtkTable> newTable = vtkSmartPointer<vtkTable>::New();
    currentSensorTable = newTable.GetPointer();
    currentSensorTable->Initialize();

    // Initialize X Axis array
    vtkSmartPointer<vtkDoubleArray> xAxis = vtkSmartPointer<vtkDoubleArray>::New();
    xAxis->SetName("Image");
    xAxis->SetNumberOfValues(imageNumber-1);
    for (int i = 0; i < xAxis->GetNumberOfTuples(); ++i)
      {
      xAxis->SetValue(i, i);
      }
    currentSensorTable->AddColumn(xAxis);

    // Initialize Y Axis array
    vtkSmartPointer<vtkDoubleArray> temperature = vtkSmartPointer<vtkDoubleArray>::New();
    temperature->SetName(sensorName.c_str());
    temperature->SetNumberOfValues(imageNumber-1);
    for (int i = 0; i < temperature->GetNumberOfTuples(); ++i)
      {
      temperature->SetValue(i, 0);
      }
    currentSensorTable->AddColumn(temperature);

    // Add new line
    vtkPlot* newLine = d->ChartView->chart()->AddPlot(vtkChart::LINE);
    if (newLine)
      {
      int array = d->ChartView->chart()->GetNumberOfPlots();
      newLine->SetInput(currentSensorTable, 0, 1);
      newLine->SetColor((array*76)%255, ((array+1)*76)%255, ((array+2)*76)%255);
      }
    
    // Add new table to the map
    d->TemperatureMap.insert( std::pair<std::string, vtkTable*>(sensorID, currentSensorTable));
    }
  else
    {
    // Sensor table found
    currentSensorTable = (*iter).second;    
    }


  // Update table
  if (currentSensorTable)
    {
    // TODO: Update Name
    // Difficult because in vtkTable, data array should have a unique name

    // Update value
    int nOfRows = currentSensorTable->GetNumberOfRows();
    currentSensorTable->InsertNextBlankRow();
    currentSensorTable->SetValue(nOfRows, 0, vtkVariant(imageNumber));
    currentSensorTable->SetValue(nOfRows, 1, vtkVariant(sensorValue));
    currentSensorTable->Modified();
    d->ChartView->chart()->RecalculateBounds();
    this->repaint();
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryGraphWidget
::closeEvent(QCloseEvent*)
{
  emit this->graphHidden();
}
