/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  ==============================================================================*/

// Qt includes
#include <QDebug>

// SlicerQt includes
#include "qSlicerRTThermometryModuleWidget.h"
#include "ui_qSlicerRTThermometryModuleWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerRTThermometryModuleWidgetPrivate: public Ui_qSlicerRTThermometryModuleWidget
{
public:

  vtkMRMLSelectionNode* SelectionNode;
  vtkMRMLInteractionNode* InteractionNode;

  vtkMRMLIGTLConnectorNode* IGTLConnector;
  vtkMRMLMarkupsFiducialNode* SensorList;
  vtkImageData* Image1;
  vtkImageData* Image2;
  vtkImageData* TotalPhaseDifference;
  vtkMRMLScalarVolumeNode* OpenIGTLinkBuffer;
  vtkMRMLScalarVolumeNode* ViewerNode;
  std::vector<vtkImageData*> TemperatureImageList;
  int NumberOfMarkupSample;
  
  int    ImageDimension[3];
  double ImageOrigin[3];
  double ImageSpacing[3];
  int    ImageScalarType;
  vtkMatrix4x4* RASToIJK;

  // Thermometry Parameters
  double EchoTime;
  double MagneticField;
  double GyromagneticRatio;
  double ThermalCoefficient;
  double ScaleFactor;
  double BaseTemperature;

  qSlicerRTThermometryGraphWidget* TemperatureGraph;

public:
  qSlicerRTThermometryModuleWidgetPrivate();
  ~qSlicerRTThermometryModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerRTThermometryModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerRTThermometryModuleWidgetPrivate::qSlicerRTThermometryModuleWidgetPrivate()
{
  this->SelectionNode = NULL;
  this->InteractionNode = NULL;

  this->IGTLConnector = NULL;
  this->SensorList = NULL;
  this->Image1 = NULL;
  this->Image2 = NULL;
  this->TotalPhaseDifference = NULL;
  this->OpenIGTLinkBuffer = NULL;
  this->ViewerNode = NULL;
  this->NumberOfMarkupSample = 0;

  this->ImageScalarType = VTK_SHORT;
  this->RASToIJK = vtkMatrix4x4::New();

  this->TemperatureGraph = NULL;

  this->EchoTime = 0.0;
  this->MagneticField = 0.0;
  this->GyromagneticRatio = 0.0;
  this->ThermalCoefficient = 0.0;
  this->ScaleFactor = 0.0;
  this->BaseTemperature = 0.0;
}

//-----------------------------------------------------------------------------
qSlicerRTThermometryModuleWidgetPrivate::~qSlicerRTThermometryModuleWidgetPrivate()
{
  if (this->OpenIGTLinkBuffer && this->IGTLConnector)
    {
    this->IGTLConnector->UnregisterIncomingMRMLNode(this->OpenIGTLinkBuffer);
    this->IGTLConnector->Delete();
    }

  if (this->SensorList)
    {
    this->SensorList->Delete();
    }

  if (this->Image1)
    {
    this->Image1->Delete();
    }

  if (this->Image2)
    {
    this->Image2->Delete();
    }

  if (this->TotalPhaseDifference)
    {
    this->TotalPhaseDifference->Delete();
    }

  if (this->ViewerNode)
    {
    this->ViewerNode->Delete();
    }

  if (this->RASToIJK)
    {
    this->RASToIJK->Delete();
    }

  for (unsigned int i = 0; i < this->TemperatureImageList.size(); ++i)
    {
    vtkImageData* imData = this->TemperatureImageList[i];
    if (imData)
      {
      imData->Delete();
      }
    }
  this->TemperatureImageList.clear();

  if (this->TemperatureGraph)
    {
    delete this->TemperatureGraph;
    }
}

//-----------------------------------------------------------------------------
// qSlicerRTThermometryModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerRTThermometryModuleWidget::qSlicerRTThermometryModuleWidget(QWidget* _parent)
  : Superclass( _parent )
    , d_ptr( new qSlicerRTThermometryModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerRTThermometryModuleWidget::~qSlicerRTThermometryModuleWidget()
{
  this->qvtkDisconnectAll();
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::setup()
{
  Q_D(qSlicerRTThermometryModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Settings
  connect(d->ServerRadio, SIGNAL(toggled(bool)),
          this, SLOT(onServerRadioToggled(bool)));

  connect(d->ConnectButton, SIGNAL(clicked()),
          this, SLOT(onConnectClicked()));

  // Thermometry Parameters
  d->EchoTime = d->EchoTimeWidget->value();
  d->MagneticField = d->MagneticFieldWidget->value();
  d->GyromagneticRatio = d->GyromagneticRatioWidget->value();
  d->ThermalCoefficient = d->ThermalCoeffWidget->value();
  d->ScaleFactor = d->ScaleFactorWidget->value();
  d->BaseTemperature = d->BaseTemperatureWidget->value();

  connect(d->SetBaselineButton, SIGNAL(clicked()),
	  this, SLOT(onSetBaselineClicked()));

  // Sensors
  if (d->SensorTableWidget)
    {
    // Hide ID column
    d->SensorTableWidget->setColumnHidden(0,true);
    }

  connect(d->AddSensorButton, SIGNAL(toggled(bool)),
          this, SLOT(onAddSensorClicked(bool)));

  connect(d->RemoveSensorButton, SIGNAL(clicked()),
          this, SLOT(onRemoveSensorClicked()));

  connect(d->ShowGraphCheckbox, SIGNAL(stateChanged(int)),
          this, SLOT(onShowGraphChanged(int)));

  connect(d->SensorTableWidget, SIGNAL(cellChanged(int,int)),
          this, SLOT(onSensorChanged(int,int)));

  // Time Player
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onServerRadioToggled(bool checked)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->HostnameLine || !d->PortLine)
    {
    return;
    }

  if (checked)
    {
    d->HostnameLine->setEnabled(false);
    }
  else
    {
    d->HostnameLine->setEnabled(true);
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onConnectClicked()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!this->mrmlScene())
    {
    return;
    }

  // Create Markup node
  if (!d->SensorList)
    {
    d->SensorList = vtkMRMLMarkupsFiducialNode::New();
    d->SensorList->SetName("Sensors");
    this->mrmlScene()->AddNode(d->SensorList);
    vtkMRMLMarkupsDisplayNode* displayNode =
      vtkMRMLMarkupsDisplayNode::New();
    displayNode->SetGlyphType(vtkMRMLMarkupsDisplayNode::Cross2D);
    displayNode->SetGlyphScale(4.0);
    this->mrmlScene()->InsertBeforeNode(d->SensorList, displayNode);
    d->SensorList->DisableModifiedEventOn();
    d->SensorList->AddAndObserveDisplayNodeID(displayNode->GetID());
    d->SensorList->DisableModifiedEventOff();
    displayNode->Delete();

    this->qvtkConnect(d->SensorList, vtkMRMLMarkupsNode::MarkupAddedEvent,
                      this, SLOT(onMarkupNodeAdded()));
    this->qvtkConnect(d->SensorList, vtkMRMLMarkupsNode::PointModifiedEvent,
                      this, SLOT(onMarkupNodeModified(vtkObject*, vtkObject*)));
    this->qvtkConnect(d->SensorList, vtkMRMLMarkupsNode::NthMarkupModifiedEvent,
                      this, SLOT(onMarkupNodeModified(vtkObject*, vtkObject*)));
    this->qvtkConnect(d->SensorList, vtkMRMLMarkupsNode::MarkupRemovedEvent,
                      this, SLOT(onMarkupNodeRemoved()));
    }

  // Add OpenIGTLConnector node
  if (!d->IGTLConnector)
    {
    d->IGTLConnector = vtkMRMLIGTLConnectorNode::New();
    this->mrmlScene()->AddNode(d->IGTLConnector);

    this->qvtkConnect(d->IGTLConnector, vtkMRMLIGTLConnectorNode::ConnectedEvent,
                      this, SLOT(onStatusConnected()));
    this->qvtkConnect(d->IGTLConnector, vtkMRMLIGTLConnectorNode::DisconnectedEvent,
                      this, SLOT(onStatusDisconnected()));
    }

  // Connect
  if (d->ServerRadio->isChecked())
    {
    // Server type
    d->IGTLConnector->SetTypeServer(d->PortLine->text().toDouble());
    }
  else
    {
    // Client type
    d->IGTLConnector->SetTypeClient(d->HostnameLine->text().toStdString(),
                                    d->PortLine->text().toDouble());
    }
  d->IGTLConnector->Start();
  d->ConnectionFrame->setText("Connection - Waiting for connection...");
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onStatusConnected()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->ConnectionFrame || !d->IGTLConnector || !this->mrmlScene())
    {
    return;
    }

  d->ConnectionFrame->setCollapsed(true);

  // Phase Image Node
  vtkSmartPointer<vtkIGTLToMRMLImage> imageConverter =
    vtkSmartPointer<vtkIGTLToMRMLImage>::New();
  if (!d->OpenIGTLinkBuffer)
    {
    if (imageConverter)
      {
      d->OpenIGTLinkBuffer =
        vtkMRMLScalarVolumeNode::SafeDownCast(imageConverter->CreateNewNode(this->mrmlScene(), "ImagerClient"));
      if (d->OpenIGTLinkBuffer)
        {
        d->IGTLConnector->RegisterIncomingMRMLNode(d->OpenIGTLinkBuffer);
        }
      }
    }

  // Initialize graph
  if (!d->TemperatureGraph)
    {
    d->TemperatureGraph = new qSlicerRTThermometryGraphWidget();
    d->TemperatureGraph->setVisible(false);
    connect(d->TemperatureGraph, SIGNAL(graphHidden()),
	    this, SLOT(onGraphHidden()));
    }

  d->ConnectionFrame->setText("Connection - Connected");
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onSetBaselineClicked()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->OpenIGTLinkBuffer)
    {
    return;
    }

 this->qvtkDisconnect(d->OpenIGTLinkBuffer, vtkMRMLVolumeNode::ImageDataModifiedEvent,
		      this, SLOT(onPhaseImageModified()));

  if (!d->EchoTimeWidget || !d->MagneticFieldWidget ||
      !d->GyromagneticRatioWidget || !d->ThermalCoeffWidget ||
      !d->ScaleFactorWidget || !d->BaseTemperatureWidget)
    {
    return;
    }

  d->EchoTime = d->EchoTimeWidget->value();
  d->MagneticField = d->MagneticFieldWidget->value();
  d->GyromagneticRatio = d->GyromagneticRatioWidget->value();
  d->ThermalCoefficient = d->ThermalCoeffWidget->value();
  d->ScaleFactor = d->ScaleFactorWidget->value();
  d->BaseTemperature = d->BaseTemperatureWidget->value();
  
  if (d->Image1)
    {
    d->Image1->Delete();
    d->Image1 = NULL;
    }

  if (d->Image2)
    {
    d->Image2->Delete();
    d->Image2 = NULL;
    }

  if (d->TotalPhaseDifference)
    {
    d->TotalPhaseDifference->Delete();
    d->TotalPhaseDifference = NULL;
    }
  
  for (unsigned int i = 0; i < d->TemperatureImageList.size(); ++i)
    {
    vtkImageData* imData = d->TemperatureImageList[i];
    if (imData)
      {
      imData->Delete();
      }
    }
  d->TemperatureImageList.clear();
  
  if (d->TemperatureGraph)
    {
    d->TemperatureGraph->clearData();
    }

  d->NumberOfMarkupSample = 0;

  this->qvtkConnect(d->OpenIGTLinkBuffer, vtkMRMLVolumeNode::ImageDataModifiedEvent,
		    this, SLOT(onPhaseImageModified()));
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onStatusDisconnected()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (d->ConnectionFrame)
    {
    d->ConnectionFrame->setCollapsed(false);
    }
  d->ConnectionFrame->setText("Connection - Disconnected");
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onAddSensorClicked(bool pressed)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!this->mrmlScene() || !d->SensorList)
    {
    return;
    }

  if (!d->SelectionNode || !d->InteractionNode)
    {
    d->SelectionNode =
      vtkMRMLSelectionNode::SafeDownCast(this->mrmlScene()->GetNodeByID("vtkMRMLSelectionNodeSingleton"));
    d->InteractionNode =
      vtkMRMLInteractionNode::SafeDownCast(this->mrmlScene()->GetNodeByID("vtkMRMLInteractionNodeSingleton"));
    }

  if (!d->SelectionNode || !d->InteractionNode)
    {
    return;
    }

  d->SelectionNode->SetReferenceActivePlaceNodeClassName(d->SensorList->GetClassName());
  d->SelectionNode->SetActivePlaceNodeID(d->SensorList->GetID());

  if (pressed)
    {
    d->InteractionNode->SwitchToSinglePlaceMode();
    d->InteractionNode->SetCurrentInteractionMode(vtkMRMLInteractionNode::Place);
    }
  else
    {
    d->InteractionNode->SetCurrentInteractionMode(vtkMRMLInteractionNode::ViewTransform);
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onRemoveSensorClicked()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->SensorTableWidget || !d->SensorList)
    {
    return;
    }

  int selectedRow = d->SensorTableWidget->currentRow();
  int numberOfRow = d->SensorTableWidget->rowCount();
  if (selectedRow < numberOfRow &&
      selectedRow >= 0)
    {
    QTableWidgetItem* itemID = d->SensorTableWidget->item(selectedRow,0);
    if (itemID)
      {
      std::string currentID(itemID->text().toStdString());
      int markupIndex = this->getMarkupIndexByID(currentID.c_str());
      if (markupIndex >= 0)
        {
        d->SensorList->RemoveMarkup(markupIndex);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onShowGraphChanged(int state)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->TemperatureGraph)
    {
    return;
    }

  d->TemperatureGraph->setVisible(state == Qt::Checked);
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onMarkupNodeAdded()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->SensorList)
    {
    return;
    }

  d->AddSensorButton->setChecked(false);
  Markup* lastMarkup = d->SensorList->GetNthMarkup(d->SensorList->GetNumberOfMarkups()-1);
  if (lastMarkup)
    {
    if (!lastMarkup->Label.empty())
      {
      lastMarkup->Description.assign(lastMarkup->Label);
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onMarkupNodeModified(vtkObject* vtkNotUsed(caller),
                                                            vtkObject* callData)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!callData || !d->SensorList)
    {
    return;
    }

  int *nPtr = NULL;
  int n = -1;
  nPtr = reinterpret_cast<int*>(callData);
  if (nPtr)
    {
    n = *nPtr;
    }

  if (n >= 0 && n < d->SensorList->GetNumberOfMarkups())
    {
    Markup* modifiedMarkup = d->SensorList->GetNthMarkup(n);
    if (modifiedMarkup)
      {
      this->updateMarkupInWidget(modifiedMarkup);
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onMarkupNodeRemoved()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->SensorList || !d->SensorTableWidget)
    {
    return;
    }

  // Clear list
  d->SensorTableWidget->clearContents();
  d->SensorTableWidget->setRowCount(0);

  // Re-populate the widget
  int numberOfMarkups = d->SensorList->GetNumberOfMarkups();
  for (int i = 0; i < numberOfMarkups; ++i)
    {
    Markup* tmpMarkup = d->SensorList->GetNthMarkup(i);
    this->updateMarkupInWidget(tmpMarkup);
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onSensorChanged(int row, int column)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->SensorTableWidget || column != 1 || row >= d->SensorTableWidget->rowCount() ||
      !d->SensorList)
    {
    return;
    }

  std::string currentNodeID(d->SensorTableWidget->item(row,0)->text().toStdString());
  std::string newNodeName(d->SensorTableWidget->item(row,1)->text().toStdString());
  int markupIndex = this->getMarkupIndexByID(currentNodeID.c_str());
  if (markupIndex >= 0)
    {
    Markup* tmpMarkup = d->SensorList->GetNthMarkup(markupIndex);
    if (tmpMarkup)
      {
      if (tmpMarkup->Description.compare(newNodeName) != 0)
        {
        d->SensorList->SetNthMarkupDescription(markupIndex, newNodeName);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onPhaseImageModified()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!this->logic() || !d->OpenIGTLinkBuffer)
    {
    return;
    }

  vtkImageData* dataReceived = d->OpenIGTLinkBuffer->GetImageData();

  if (!d->Image1 && !d->TotalPhaseDifference)
    {
    d->OpenIGTLinkBuffer->GetOrigin(d->ImageOrigin);
    d->OpenIGTLinkBuffer->GetSpacing(d->ImageSpacing);
    d->OpenIGTLinkBuffer->GetRASToIJKMatrix(d->RASToIJK);
    dataReceived->GetDimensions(d->ImageDimension);
    d->ImageScalarType = dataReceived->GetScalarType();

    d->Image1 = vtkImageData::New();
    d->Image1->DeepCopy(dataReceived);

    d->TotalPhaseDifference = vtkImageData::New();
    d->TotalPhaseDifference->SetDimensions(d->ImageDimension);
    d->TotalPhaseDifference->SetSpacing(d->ImageSpacing);
    d->TotalPhaseDifference->SetOrigin(d->ImageOrigin);
    d->TotalPhaseDifference->SetScalarType(d->ImageScalarType);
    d->TotalPhaseDifference->SetNumberOfScalarComponents(1);
    d->TotalPhaseDifference->AllocateScalars();
    memset(d->TotalPhaseDifference->GetScalarPointer(), 0x00, d->TotalPhaseDifference->GetActualMemorySize()*1024);

    this->createViewerNode();
    return;
    }

  if (!d->Image2)
    {
    d->Image2 = vtkImageData::New();
    }
  d->Image2->DeepCopy(dataReceived);
  

  if (d->Image1 && d->Image2 && 
      d->TotalPhaseDifference && d->ViewerNode)
    {
    this->computePhaseDifference(d->Image1, d->Image2);
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::onGraphHidden()
{
  Q_D(qSlicerRTThermometryModuleWidget);
  
  if (!d->ShowGraphCheckbox)
    {
    return;
    }

  d->ShowGraphCheckbox->setChecked(false);
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::updateMarkupInWidget(Markup* modifiedMarkup)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->SensorTableWidget || !d->RASToIJK)
    {
    return;
    }

  // Find is markup has already been added
  int rowNumber = d->SensorTableWidget->rowCount();
  int itemIndex = -1;
  for (int i = 0; i < rowNumber; ++i)
    {
    QTableWidgetItem* currentItem = d->SensorTableWidget->item(i,0);
    if (modifiedMarkup->ID.compare(currentItem->text().toStdString()) == 0)
      {
      itemIndex = i;
      continue;
      }
    }

  if (itemIndex < 0)
    {
    // Markup is not in the list yet. Add it.
    QTableWidgetItem* idItem = new QTableWidgetItem();
    QTableWidgetItem* nameItem = new QTableWidgetItem();
    nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
    QTableWidgetItem* temperatureItem = new QTableWidgetItem();
    temperatureItem->setFlags(temperatureItem->flags() & ~Qt::ItemIsEditable);

    d->SensorTableWidget->insertRow(rowNumber);
    d->SensorTableWidget->setItem(rowNumber, 0, idItem);
    d->SensorTableWidget->setItem(rowNumber, 1, nameItem);
    d->SensorTableWidget->setItem(rowNumber, 2, temperatureItem);

    std::string markupID(modifiedMarkup->ID);
    d->SensorTableWidget->item(rowNumber,0)->setText(markupID.c_str());

    itemIndex = rowNumber;
    }

  // Update temperature
  double temp = 0.0;
  if (d->TemperatureImageList.size() > 0)
    {
    // Get Markup position
    double mPos[4] = { modifiedMarkup->points[0].X(),
                       modifiedMarkup->points[0].Y(),
                       modifiedMarkup->points[0].Z(),
                       1.0 };
    double mIJKPos[4];
    d->RASToIJK->MultiplyPoint(mPos, mIJKPos);

    vtkImageData* lastImage =
      d->ViewerNode->GetImageData();
    if (lastImage)
      {
      temp = lastImage->GetScalarComponentAsDouble(mIJKPos[0], mIJKPos[1], mIJKPos[2], 0);
      }
    }
  QString tempNumber = QString::number(temp,'f',1);
  d->SensorTableWidget->item(itemIndex, 2)->setText(tempNumber);

  // Update name
  std::stringstream markupName;
  markupName << modifiedMarkup->Description << " (" << tempNumber.toStdString() << ")";
  modifiedMarkup->Label = markupName.str();
  d->SensorList->Modified();
  d->SensorTableWidget->item(itemIndex,1)->setText(modifiedMarkup->Description.c_str());
}

//-----------------------------------------------------------------------------
int qSlicerRTThermometryModuleWidget::
getMarkupIndexByID(const char* markupID)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->SensorList)
    {
    return -1;
    }

  int numberOfMarkups = d->SensorList->GetNumberOfMarkups();
  for (int i = 0; i < numberOfMarkups; ++i)
    {
    Markup* compareMarkup = d->SensorList->GetNthMarkup(i);
    if (compareMarkup &&
        strcmp(compareMarkup->ID.c_str(), markupID) == 0)
      {
      return i;
      }
    }
  return -1;
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::
computePhaseDifference(vtkImageData* im1, vtkImageData* im2)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!im1 || !im2 || !d->TotalPhaseDifference)
    {
    return;
    }

  if (im1 == im2)
    {
    return;
    }

  vtkImageData* newImData = vtkImageData::New();
  newImData->SetDimensions(d->ImageDimension);
  newImData->SetSpacing(1.0, 1.0, 1.0); // Not sure why spacing should be 1.0, 1.0, 1.0, but not fitting otherwise
  newImData->SetScalarTypeToDouble();
  newImData->SetNumberOfScalarComponents(1);
  newImData->AllocateScalars();
  memset(newImData->GetScalarPointer(), 0x00, newImData->GetActualMemorySize()*1024);
  d->TemperatureImageList.push_back(newImData);

  short* im1BufferPointer = (short*) im1->GetPointData()->GetScalars()->GetVoidPointer(0);
  short* im2BufferPointer = (short*) im2->GetPointData()->GetScalars()->GetVoidPointer(0);
  short* totalPhaseDiffPointer = (short*) d->TotalPhaseDifference->GetPointData()->GetScalars()->GetVoidPointer(0);
  double* outputBufferPointer = (double*) newImData->GetPointData()->GetScalars()->GetVoidPointer(0);

  if (!im1BufferPointer || !im2BufferPointer ||
      !totalPhaseDiffPointer || !outputBufferPointer)
    {
    return;
    }

  int extent[6];
  d->TotalPhaseDifference->GetExtent(extent);

  vtkIdType inc[3];
  d->TotalPhaseDifference->GetIncrements(inc);

  for (int k=0; k<extent[5]+1; ++k)
    {
    for (int j=0; j<extent[3]+1; ++j)
      {
      for (int i=0; i<extent[1]+1; ++i)
        {
        // Compute phase difference
        short phaseDiff = im2BufferPointer[k*inc[2]+j*inc[1]+i] - im1BufferPointer[k*inc[2]+j*inc[1]+i];

        // Sum the phase difference to get the total
        totalPhaseDiffPointer[k*inc[2]+j*inc[1]+i] += phaseDiff;

        // Compute temperature using total phase difference
	double coefficient = 1 / (d->EchoTime * 2*M_PI*d->GyromagneticRatio * d->MagneticField * d->ThermalCoefficient);
        double temp = d->BaseTemperature + ((totalPhaseDiffPointer[k*inc[2]+j*inc[1]+i] * M_PI / d->ScaleFactor ) * coefficient);

        // Set the temperature
        outputBufferPointer[k*inc[2]+j*inc[1]+i] = temp;

        // Copy value from im2 to im1 to save im2 data for next iteration
        im1BufferPointer[k*inc[2]+j*inc[1]+i] = im2BufferPointer[k*inc[2]+j*inc[1]+i];
        }
      }
    }
 
  this->newImageAdded();
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::
newImageAdded()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (d->SensorList && d->SensorList->GetNumberOfMarkups() > 0)
    {
    d->NumberOfMarkupSample++;
    }

  if (d->ViewerNode)
    {
    vtkImageData* imData = d->TemperatureImageList[d->TemperatureImageList.size()-1];
    if (imData)
      {
      d->ViewerNode->SetAndObserveImageData(imData);
      this->updateAllMarkups();
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::
updateAllMarkups()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!d->SensorTableWidget || !d->SensorList)
    {
    return;
    }

  int numberOfMarkups = d->SensorTableWidget->rowCount();
  for (int i = 0; i < numberOfMarkups; ++i)
    {
    const char* sensorID = d->SensorTableWidget->item(i,0)->text().toStdString().c_str();
    int n = this->getMarkupIndexByID(sensorID);
    if (n >= 0 && n < d->SensorList->GetNumberOfMarkups())
      {
      Markup* updateMarkup = d->SensorList->GetNthMarkup(n);
      if (updateMarkup)
        {
        this->updateMarkupInWidget(updateMarkup);
        this->updateTemperatureGraph(i, updateMarkup);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::
updateTemperatureGraph(int position, Markup* sensor)
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (!sensor || !d->SensorTableWidget ||
      !d->TemperatureGraph)
    {
    return;
    }

  if (position < 0 || position >= d->SensorTableWidget->rowCount())
    {
    return;
    }

  double temperature = d->SensorTableWidget->item(position,2)->text().toDouble();
  std::string sensorID(d->SensorTableWidget->item(position,0)->text().toStdString());

  d->TemperatureGraph->recordNewData(sensorID, sensor->Description, temperature, d->NumberOfMarkupSample);
}

//-----------------------------------------------------------------------------
void qSlicerRTThermometryModuleWidget::createViewerNode()
{
  Q_D(qSlicerRTThermometryModuleWidget);

  if (d->ViewerNode)
    {
    return;
    }

  d->ViewerNode = vtkMRMLScalarVolumeNode::New();
  d->ViewerNode->SetOrigin(d->ImageOrigin);
  d->ViewerNode->SetSpacing(d->ImageSpacing);
  d->ViewerNode->SetRASToIJKMatrix(d->RASToIJK);
  d->ViewerNode->SetName("TemperatureViewer");
  this->mrmlScene()->AddNode(d->ViewerNode);
  
  // Create color table
  vtkSmartPointer<vtkMRMLColorTableNode> colorTable =
    vtkSmartPointer<vtkMRMLColorTableNode>::New();
  colorTable->SetName("RTThermometryColorMap");
  colorTable->SetTypeToUser();
  colorTable->SetNamesFromColors();
  colorTable->SetNumberOfColors(256);
  
  double temperatureRange[2] = {25.0, 90.0};
  
  vtkLookupTable* lut = colorTable->GetLookupTable();
  lut->SetHueRange(0.67, 0.0);
  lut->SetSaturationRange(1.0, 1.0);
  lut->SetValueRange(0.7, 1.0);
  lut->SetAlphaRange(0.8, 0.8);
  lut->SetRampToLinear();
  lut->SetScaleToLinear();
  lut->SetTableRange(temperatureRange[0], temperatureRange[1]);
  lut->Build();
  lut->SetTableValue(0, 0.0, 0.0, 0.0, 0.0);
  lut->SetTableValue(255, 0.0, 0.0, 0.0, 0.0);
  this->mrmlScene()->AddNode(colorTable.GetPointer());
  
  // Create display node
  vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> displayNode =
    vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
  displayNode->SetAndObserveColorNodeID(colorTable->GetID());
  displayNode->AutoWindowLevelOff();
  displayNode->SetLevel(temperatureRange[0] + (temperatureRange[1] - temperatureRange[0])/2);
  displayNode->SetWindow(temperatureRange[1] - temperatureRange[0]);
  this->mrmlScene()->AddNode(displayNode.GetPointer());
  
  d->ViewerNode->SetAndObserveDisplayNodeID(displayNode->GetID());
}
